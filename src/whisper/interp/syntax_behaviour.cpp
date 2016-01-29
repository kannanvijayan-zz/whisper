
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


// Declare a lift function for each syntax node type.
#define DECLARE_SYNTAX_FN_(name) \
    static VM::CallResult Syntax_##name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxTreeFragment*> args);

    DECLARE_SYNTAX_FN_(File)

  /*
    DECLARE_SYNTAX_FN_(EmptyStmt)
  */

    DECLARE_SYNTAX_FN_(ExprStmt)

  /*
    DECLARE_SYNTAX_FN_(ReturnStmt)
    DECLARE_SYNTAX_FN_(DefStmt)
    DECLARE_SYNTAX_FN_(ConstStmt)
    DECLARE_SYNTAX_FN_(VarStmt)
  */

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
  /*
    DECLARE_SYNTAX_FN_(IntegerExpr)
 */

#undef DECLARE_SYNTAX_FN_

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
    Local<VM::PropertyDescriptor> desc(acx, VM::PropertyDescriptor(natF.get()));

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

/*
    BIND_GLOBAL_METHOD_(EmptyStmt);
*/
    BIND_GLOBAL_METHOD_(ExprStmt);
/*
    BIND_GLOBAL_METHOD_(ReturnStmt);
    BIND_GLOBAL_METHOD_(DefStmt);
    BIND_GLOBAL_METHOD_(ConstStmt);
    BIND_GLOBAL_METHOD_(VarStmt);
*/

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
/*
    BIND_GLOBAL_METHOD_(IntegerExpr);

*/

#undef BIND_GLOBAL_METHOD_

    return OkVal();
}

#define IMPL_SYNTAX_FN_(name) \
    static VM::CallResult Syntax_##name( \
        ThreadContext* cx, \
        Handle<VM::NativeCallInfo> callInfo, \
        ArrayHandle<VM::SyntaxTreeFragment*> args)

IMPL_SYNTAX_FN_(File)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@File called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::File);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedFileNode> fileNode(cx,
        AST::PackedFileNode(pst->data(), stRef->offset()));

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

IMPL_SYNTAX_FN_(ExprStmt)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ExprStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::ExprStmt);

    Local<VM::SyntaxNodeRef> stRef(cx, args.get(0)->toNode());
    Local<VM::PackedSyntaxTree*> pst(cx, stRef->pst());
    Local<AST::PackedExprStmtNode> exprStmtNode(cx,
        AST::PackedExprStmtNode(pst->data(), stRef->offset()));

    SpewInterpNote("Syntax_ExprStmt: Interpreting");

    // Create a new entry frame for evaluating the ExprStmt's child.
    Local<AST::PackedBaseNode> exprBaseNode(cx, exprStmtNode->expression());
    Local<VM::SyntaxTreeFragment*> exprStRef(cx);
    if (!exprStRef.setResult(VM::SyntaxNode::Create(
            cx->inHatchery(), pst, exprBaseNode->offset())))
    {
        WH_ASSERT(cx->hasError());
        return ErrorVal();
    }

    Local<VM::EntryFrame*> exprEntryFrame(cx);
    if (!exprEntryFrame.setResult(VM::EntryFrame::Create(
            cx->inHatchery(), callInfo->frame(), exprStRef,
            callInfo->callerScope())))
    {
        WH_ASSERT(cx->hasError());
        return ErrorVal();
    }

    return VM::CallResult::Continue(exprEntryFrame);
}

IMPL_SYNTAX_FN_(CallExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ExprStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::CallExpr);

    Local<VM::SyntaxNode*> stNode(cx, args.get(0)->toNode());

    SpewInterpNote("Syntax_CallExpr: Interpreting");

    // Set up a CallExprSyntaxFrame
    Local<VM::EntryFrame*> entryFrame(cx,
        callInfo->frame()->ancestorEntryFrame());

    Local<VM::CallExprSyntaxFrame*> callExprSyntaxFrame(cx);
    if (!callExprSyntaxFrame.setResult(VM::CallExprSyntaxFrame::Create(
            cx->inHatchery(), callInfo->frame(), entryFrame, stNode.handle())))
    {
        return ErrorVal();
    }

    return VM::CallResult::Continue(callExprSyntaxFrame.get());
}

IMPL_SYNTAX_FN_(NameExpr)
{
    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@ExprStmt called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0)->isNode());
    WH_ASSERT(args.get(0)->toNode()->nodeType() == AST::NameExpr);
    Local<VM::PackedSyntaxTree*> pst(cx, args.get(0)->pst());
    Local<AST::PackedNameExprNode> nameExprNode(cx,
        AST::PackedNameExprNode(pst->data(), args.get(0)->offset()));

    Local<VM::String*> name(cx,
        pst->getConstantString(nameExprNode->nameCid()));

    SpewInterpNote("Syntax_NameExpr: Interpreting");

    Local<VM::Frame*> frame(cx, callInfo->frame());
    Local<VM::Frame*> parent(cx, frame->parent());
    Local<VM::ScopeObject*> scope(cx, frame->ancestorEntryFrame()->scope());

    PropertyLookupResult lookupResult = GetObjectProperty(cx,
        scope.handle(), name);

    if (lookupResult.isError()) {
        SpewInterpNote("Syntax_NameExpr - lookupResult returned error!");
        return VM::CallResult::Error();
    }

    if (lookupResult.isNotFound()) {
        SpewInterpNote("Syntax_NameExpr -"
                       " lookupResult returned notFound -"
                       " raising exception!");
        cx->setExceptionRaised("Name binding not found.", name.get());
        return VM::CallResult::Exception(frame.get());
    }

    if (lookupResult.isFound()) {
        SpewInterpNote("Syntax_NameExpr - lookupResult returned found");
        Local<VM::PropertyDescriptor> descriptor(cx, lookupResult.descriptor());
        Local<VM::LookupState*> lookupState(cx, lookupResult.lookupState());

        // Handle a value binding by returning the value.
        if (descriptor->isValue())
            return VM::CallResult::Value(descriptor->valBox());

        // Handle a method binding by creating a bound FunctionObject
        // from the method.
        if (descriptor->isMethod()) {
            // Create a new function object bound to the scope, which
            // is the receiver object.
            Local<VM::ValBox> scopeVal(cx, VM::ValBox::Object(scope.get()));
            Local<VM::Function*> func(cx, descriptor->method());
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


} // namespace Interp
} // namespace Whisper
