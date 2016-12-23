
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "vm/frame.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "interp/heap_interpreter.hpp"
#include "interp/syntax_behaviour.hpp"

namespace Whisper {
namespace Interp {


// Helper macro to declare native functions for syntax lifting.
#define IMPL_SYNTAX_FN_(name) \
    static VM::CallResult Syntax_##name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxTreeFragment*> args)

#define DECLARE_SYNTAX_FN_(name) IMPL_SYNTAX_FN_(name);

DECLARE_SYNTAX_FN_(File)
DECLARE_SYNTAX_FN_(Block)
DECLARE_SYNTAX_FN_(EmptyStmt)
DECLARE_SYNTAX_FN_(ExprStmt)

/*
DECLARE_SYNTAX_FN_(ReturnStmt)
*/
DECLARE_SYNTAX_FN_(DefStmt)
/*
DECLARE_SYNTAX_FN_(ConstStmt)
*/
DECLARE_SYNTAX_FN_(VarStmt)

DECLARE_SYNTAX_FN_(CallExpr)

/*
DECLARE_SYNTAX_FN_(DotExpr)
DECLARE_SYNTAX_FN_(ArrowExpr)
DECLARE_SYNTAX_FN_(PosExpr)
DECLARE_SYNTAX_FN_(NegExpr)
DECLARE_SYNTAX_FN_(AddExpr)
DECLARE_SYNTAX_FN_(SubExpr)
DECLARE_SYNTAX_FN_(MulExpr)
DECLARE_SYNTAX_FN_(DivExpr)
DECLARE_SYNTAX_FN_(ParenExpr)
*/
DECLARE_SYNTAX_FN_(NameExpr)
DECLARE_SYNTAX_FN_(IntegerExpr)

static OkResult
BindGlobalMethod(AllocationContext acx,
                 Handle<VM::GlobalScope*> obj,
                 VM::String* name,
                 VM::NativeOperativeFuncPtr opFunc)
{
    Local<VM::String*> rootedName(acx, name);

    // Allocate NativeFunction object.
    Local<VM::NativeFunction*> natF(acx);
    if (!natF.setResult(VM::NativeFunction::Create(acx, opFunc)))
        return ErrorVal();
    Local<VM::PropertyDescriptor> desc(acx,
        VM::PropertyDescriptor::MakeMethod(natF.get()));

    // Bind method on global.
    if (!VM::Wobject::DefineProperty(acx, obj.convertTo<VM::Wobject*>(),
                                     rootedName, desc))
    {
        return ErrorVal();
    }

    return OkVal();
}

OkResult
BindSyntaxHandlers(AllocationContext acx, VM::GlobalScope* scope)
{
    Local<VM::GlobalScope*> rootedScope(acx, scope);

    ThreadContext* cx = acx.threadContext();
    Local<VM::RuntimeState*> rtState(acx, cx->runtimeState());

#define BIND_GLOBAL_METHOD_(name) \
    do { \
        if (!BindGlobalMethod(acx, rootedScope, rtState->nm_At##name(), \
                              &Syntax_##name)) \
        { \
            return ErrorVal(); \
        } \
    } while(false)

    BIND_GLOBAL_METHOD_(File);
    BIND_GLOBAL_METHOD_(Block);
    BIND_GLOBAL_METHOD_(EmptyStmt);
    BIND_GLOBAL_METHOD_(ExprStmt);
/*
    BIND_GLOBAL_METHOD_(ReturnStmt);
*/
    BIND_GLOBAL_METHOD_(DefStmt);
/*
    BIND_GLOBAL_METHOD_(ConstStmt);
*/
    BIND_GLOBAL_METHOD_(VarStmt);

    BIND_GLOBAL_METHOD_(CallExpr);

/*
    BIND_GLOBAL_METHOD_(DotExpr);
    BIND_GLOBAL_METHOD_(ArrowExpr);
    BIND_GLOBAL_METHOD_(PosExpr);
    BIND_GLOBAL_METHOD_(NegExpr);
    BIND_GLOBAL_METHOD_(AddExpr);
    BIND_GLOBAL_METHOD_(SubExpr);
    BIND_GLOBAL_METHOD_(MulExpr);
    BIND_GLOBAL_METHOD_(DivExpr);
    BIND_GLOBAL_METHOD_(ParenExpr);
*/
    BIND_GLOBAL_METHOD_(NameExpr);
    BIND_GLOBAL_METHOD_(IntegerExpr);

#undef BIND_GLOBAL_METHOD_

    return OkVal();
}

IMPL_SYNTAX_FN_(File)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                           "@File called with wrong number of arguments")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::File);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<AST::PackedFileNode> fileNode(cx, stRef->astFile());

    SpewInterpNote("Syntax_File: Interpreting %u statements",
                   unsigned(fileNode->numStatements()));

    Local<VM::Frame*> frame(cx, callInfo->frame());
    Local<VM::EntryFrame*> entryFrame(cx, frame->ancestorEntryFrame());
    Local<VM::SyntaxNode*> stFrag(cx);
    if (!stFrag.setResult(VM::SyntaxNode::Create(cx->inHatchery(), stRef))) {
        return ErrorVal();
    }

    Local<VM::Frame*> fileSyntaxFrame(cx);
    if (!fileSyntaxFrame.setResult(VM::FileSyntaxFrame::Create(
            cx->inHatchery(), frame, entryFrame,
            stFrag.handle().convertTo<VM::SyntaxTreeFragment*>(),
            0)))
    {
        return ErrorVal();
    }

    return VM::CallResult::Continue(fileSyntaxFrame);
}

IMPL_SYNTAX_FN_(Block)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                           "@Block called with wrong number of arguments")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isBlock());

    Local<VM::SyntaxBlockRef> stRef(cx, args.get(0)->toBlock());
    Local<AST::PackedBlock> astBlock(cx, stRef->astBlock());

    SpewInterpNote("Syntax_Block: Interpreting %u statements",
                   unsigned(astBlock->numStatements()));

    // An empty block resolves immediately with a void result.
    if (astBlock->numStatements() == 0) {
        return VM::CallResult::Void();
    }

    Local<VM::Frame*> frame(cx, callInfo->frame());
    Local<VM::EntryFrame*> entryFrame(cx, frame->ancestorEntryFrame());
    Local<VM::SyntaxTreeFragment*> stFrag(cx);
    if (!stFrag.setResult(VM::SyntaxBlock::Create(cx->inHatchery(), stRef))) {
        return ErrorVal();
    }

    Local<VM::Frame*> fileSyntaxFrame(cx);
    if (!fileSyntaxFrame.setResult(VM::BlockSyntaxFrame::Create(
            cx->inHatchery(), frame, entryFrame,
            stFrag.handle().convertTo<VM::SyntaxTreeFragment*>(),
            0)))
    {
        return ErrorVal();
    }

    return VM::CallResult::Continue(fileSyntaxFrame);
}

IMPL_SYNTAX_FN_(EmptyStmt)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@EmptyStmt called with wrong number of arguments.")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::EmptyStmt);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedEmptyStmtNode> exprStmtNode(cx, stRef->astEmptyStmt());

    SpewInterpNote("Syntax_EmptyStmt: Interpreting");

    return VM::CallResult::Void();
}

IMPL_SYNTAX_FN_(ExprStmt)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@ExprStmt called with wrong number of arguments.")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::ExprStmt);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedExprStmtNode> exprStmtNode(cx, stRef->astExprStmt());

    SpewInterpNote("Syntax_ExprStmt: Interpreting");

    // Create a new syntax frame for evaluating the ExprStmt's child.
    Local<AST::PackedBaseNode> exprBaseNode(cx, exprStmtNode->expression());
    Local<VM::SyntaxTreeFragment*> exprStRef(cx);
    if (!exprStRef.setResult(VM::SyntaxNode::Create(
            cx->inHatchery(), pst, exprBaseNode->offset())))
    {
        WH_ASSERT(cx->hasError());
        return ErrorVal();
    }

    Local<VM::Frame*> frame(cx, callInfo->frame());
    Local<VM::EntryFrame*> entryFrame(cx, frame->ancestorEntryFrame());
    Local<VM::InvokeSyntaxNodeFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(VM::InvokeSyntaxNodeFrame::Create(
            cx->inHatchery(), frame, entryFrame, exprStRef)))
    {
        WH_ASSERT(cx->hasError());
        return ErrorVal();
    }

    return VM::CallResult::Continue(syntaxFrame);
}

IMPL_SYNTAX_FN_(DefStmt)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@DefStmt called with wrong number of arguments.")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::DefStmt);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedDefStmtNode> defStmtNode(cx, stRef->astDefStmt());

    Local<VM::String*> name(cx,
        pst->getConstantString(defStmtNode->nameCid()));

    SpewInterpNote("Syntax_DefStmt: Interpreting");

    Local<VM::Function*> func(cx);
    if (!func.setResult(VM::ScriptedFunction::Create(
                            cx->inHatchery(), stRef, callInfo->callerScope(),
                            /* isOperative = */ false)))
    {
        WH_ASSERT(cx->hasError());
        return ErrorVal();
    }
    Local<VM::PropertyDescriptor> descr(cx,
        VM::PropertyDescriptor::MakeMethod(func));

    // Bind the function to the receiver.
    Local<VM::ValBox> receiver(cx, callInfo->receiver());
    if (!DefValueProperty(cx, receiver, name, descr)) {
        WH_ASSERT(cx->hasError());
        return ErrorVal();
    }

    // Def statements always yield void.
    return VM::CallResult::Void();
}

IMPL_SYNTAX_FN_(VarStmt)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@VarStmt called with wrong number of arguments.")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::VarStmt);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedVarStmtNode> varStmtNode(cx, stRef->astVarStmt());

    WH_ASSERT(varStmtNode->numBindings() > 0);

    SpewInterpNote("Syntax_VarStmt: Interpreting");

    // Create a VarSyntaxFrame for it.
    Local<VM::Frame*> frame(cx, callInfo->frame());
    Local<VM::EntryFrame*> entryFrame(cx, frame->ancestorEntryFrame());
    Local<VM::SyntaxTreeFragment*> stFrag(cx, args.get(0));
    Local<VM::VarSyntaxFrame*> syntaxFrame(cx);
    if (!syntaxFrame.setResult(VM::VarSyntaxFrame::Create(
            cx->inHatchery(), frame, entryFrame, stFrag, 0)))
    {
        return ErrorVal();
    }

    // Def statements always yield void.
    return VM::CallResult::Continue(syntaxFrame.get());
}

IMPL_SYNTAX_FN_(CallExpr)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@CallStmt called with wrong number of arguments.")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::CallExpr);

    Local<VM::SyntaxNode*> stNode(cx, args.get(0)->toNode());

    SpewInterpNote("Syntax_CallExpr: Interpreting");

    // Set up a CallExprSyntaxFrame
    Local<VM::EntryFrame*> entryFrame(cx,
        callInfo->frame()->ancestorEntryFrame());

    Local<VM::CallExprSyntaxFrame*> callExprSyntaxFrame(cx);
    if (!callExprSyntaxFrame.setResult(VM::CallExprSyntaxFrame::CreateCallee(
            cx->inHatchery(), callInfo->frame(), entryFrame, stNode.handle())))
    {
        return ErrorVal();
    }

    return VM::CallResult::Continue(callExprSyntaxFrame.get());
}

IMPL_SYNTAX_FN_(NameExpr)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@NameExpr called with wrong number of arguments.")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::NameExpr);
    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedNameExprNode> nameExprNode(cx, stRef->astNameExpr());

    Local<VM::String*> name(cx,
        pst->getConstantString(nameExprNode->nameCid()));

    SpewInterpNote("Syntax_NameExpr: Interpreting");

    Local<VM::Frame*> frame(cx, callInfo->frame());
    Local<VM::Frame*> parent(cx, frame->parent());
    Local<VM::ScopeObject*> scope(cx, frame->ancestorEntryFrame()->scope());

    Local<VM::PropertyLookupResult> lookupResult(cx,
        GetObjectProperty(cx, scope.handle(), name));

    if (lookupResult->isError()) {
        SpewInterpNote("Syntax_NameExpr - lookupResult returned error!");
        return VM::CallResult::Error();
    }

    if (lookupResult->isNotFound()) {
        SpewInterpNote("Syntax_NameExpr -"
                       " lookupResult returned notFound -"
                       " raising exception!");

        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                           "Name binding not found",
                           name.handle())))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(frame, exc);
    }

    if (lookupResult->isFound()) {
        SpewInterpNote("Syntax_NameExpr - lookupResult returned found");
        Local<VM::PropertyDescriptor> descriptor(cx,
                                        lookupResult->descriptor());
        Local<VM::LookupState*> lookupState(cx, lookupResult->lookupState());

        // Handle a value binding by returning the value.
        if (descriptor->isSlot())
            return VM::CallResult::Value(descriptor->slotValue());

        // Handle a method binding by creating a bound FunctionObject
        // from the method.
        if (descriptor->isMethod()) {
            // Create a new function object bound to the scope, which
            // is the receiver object.
            Local<VM::ValBox> scopeVal(cx, VM::ValBox::Object(scope.get()));
            Local<VM::Function*> func(cx, descriptor->methodFunction());
            Local<VM::FunctionObject*> funcObj(cx);
            if (!funcObj.setResult(VM::FunctionObject::Create(
                    cx->inHatchery(), func, scopeVal, lookupState)))
            {
                return ErrorVal();
            }

            return VM::CallResult::Value(VM::ValBox::Object(funcObj.get()));
        }

        WH_UNREACHABLE("PropertyDescriptor not one of Value, Method.");
        return ErrorVal();
    }

    return cx->setError(RuntimeError::InternalError,
                        "Invalid property lookup result");
}

IMPL_SYNTAX_FN_(IntegerExpr)
{
    if (args.length() != 1) {
        Local<VM::Exception*> exc(cx);
        if (!exc.setResult(VM::InternalException::Create(cx->inHatchery(),
                       "@IntegerExpr called with wrong number of arguments.")))
        {
            return ErrorVal();
        }
        return VM::CallResult::Exc(callInfo->frame(), exc);
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::IntegerExpr);
    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<AST::PackedIntegerExprNode> integerExprNode(cx,
                                                  stRef->astIntegerExpr());

    int32_t intVal = integerExprNode->value();

    return VM::CallResult::Value(VM::ValBox::Integer(intVal));
}


} // namespace Interp
} // namespace Whisper
