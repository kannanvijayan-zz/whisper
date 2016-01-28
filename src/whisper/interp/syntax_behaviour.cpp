
#include <iostream>

#include "parser/syntax_defn.hpp"
#include "parser/packed_syntax.hpp"
#include "runtime_inlines.hpp"
#include "vm/global_scope.hpp"
#include "vm/runtime_state.hpp"
#include "vm/function.hpp"
#include "vm/frame.hpp"
#include "vm/packed_syntax_tree.hpp"
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
    DECLARE_SYNTAX_FN_(ExprStmt)
    DECLARE_SYNTAX_FN_(ReturnStmt)
    DECLARE_SYNTAX_FN_(DefStmt)
    DECLARE_SYNTAX_FN_(ConstStmt)
    DECLARE_SYNTAX_FN_(VarStmt)

    DECLARE_SYNTAX_FN_(CallExpr)
    DECLARE_SYNTAX_FN_(DotExpr)
    DECLARE_SYNTAX_FN_(ArrowExpr)
    DECLARE_SYNTAX_FN_(PosExpr)
    DECLARE_SYNTAX_FN_(NegExpr)
    DECLARE_SYNTAX_FN_(AddExpr)
    DECLARE_SYNTAX_FN_(SubExpr)
    DECLARE_SYNTAX_FN_(MulExpr)
    DECLARE_SYNTAX_FN_(DivExpr)
    DECLARE_SYNTAX_FN_(ParenExpr)
    DECLARE_SYNTAX_FN_(NameExpr)
    DECLARE_SYNTAX_FN_(IntegerExpr)
    //WHISPER_DEFN_SYNTAX_NODES(DECLARE_SYNTAX_FN_)
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
    BIND_GLOBAL_METHOD_(ExprStmt);
    BIND_GLOBAL_METHOD_(ReturnStmt);
    BIND_GLOBAL_METHOD_(DefStmt);
    BIND_GLOBAL_METHOD_(ConstStmt);
    BIND_GLOBAL_METHOD_(VarStmt);

    BIND_GLOBAL_METHOD_(CallExpr);
    BIND_GLOBAL_METHOD_(DotExpr);
    BIND_GLOBAL_METHOD_(ArrowExpr);
    BIND_GLOBAL_METHOD_(PosExpr);
    BIND_GLOBAL_METHOD_(NegExpr);
    BIND_GLOBAL_METHOD_(AddExpr);
    BIND_GLOBAL_METHOD_(SubExpr);
    BIND_GLOBAL_METHOD_(MulExpr);
    BIND_GLOBAL_METHOD_(DivExpr);
    BIND_GLOBAL_METHOD_(ParenExpr);
    BIND_GLOBAL_METHOD_(NameExpr);
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
    // return cx->setExceptionRaised("File syntax handler not implemented.");

    if (args.length() != 1) {
        return cx->setExceptionRaised(
            "@File called with wrong number of arguments.");
    }

    WH_ASSERT(args.get(0)->isNode() && args.get(0)->toNode()->nodeType() == AST::File);

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


} // namespace Interp
} // namespace Whisper
