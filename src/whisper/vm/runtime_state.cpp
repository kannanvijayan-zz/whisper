
#include "runtime_inlines.hpp"
#include "vm/packed_syntax_tree.hpp"
#include "vm/runtime_state.hpp"
#include "vm/global_scope.hpp"
#include "vm/wobject.hpp"
#include "interp/syntax_behaviour.hpp"
#include "interp/object_behaviour.hpp"

namespace Whisper {
namespace VM {


/* static */ Result<RuntimeState*>
RuntimeState::Create(AllocationContext acx)
{
    // Allocate the array.
    Local<String*> vmstr(acx, nullptr);
    Local<Array<String*>*> namePool(acx);
    if (!namePool.setResult(
            Array<String*>::CreateFill(acx, NamePool::Size(), vmstr.handle())))
    {
        return ErrorVal();
    }

#define ALLOC_STRING_(name, str) \
    if (!vmstr.setResult(VM::String::Create(acx, str))) \
        return ErrorVal(); \
    namePool->set(NamePool::IndexOfId(NamePool::Id::name), vmstr.get());
        WHISPER_DEFN_NAME_POOL(ALLOC_STRING_)
#undef ALLOC_STRING_

    return acx.create<RuntimeState>(namePool.get());
}

String*
RuntimeState::syntaxHandlerName(SyntaxNode const* syntaxNode) const
{
    return syntaxHandlerName(syntaxNode->nodeType());
}

String*
RuntimeState::syntaxHandlerName(AST::NodeType nodeType) const
{
    switch (nodeType) {
      case AST::File:               return nm_AtFile();
      case AST::Block:              return nm_AtBlock();
      case AST::EmptyStmt:          return nm_AtEmptyStmt();
      case AST::ExprStmt:           return nm_AtExprStmt();
      case AST::ReturnStmt:         return nm_AtReturnStmt();
      case AST::IfStmt:             return nm_AtIfStmt();
      case AST::DefStmt:            return nm_AtDefStmt();
      case AST::ConstStmt:          return nm_AtConstStmt();
      case AST::VarStmt:            return nm_AtVarStmt();
      case AST::LoopStmt:           return nm_AtLoopStmt();
      case AST::CallExpr:           return nm_AtCallExpr();
      case AST::DotExpr:            return nm_AtDotExpr();
      case AST::ArrowExpr:          return nm_AtArrowExpr();
      case AST::PosExpr:            return nm_AtPosExpr();
      case AST::NegExpr:            return nm_AtNegExpr();
      case AST::AddExpr:            return nm_AtAddExpr();
      case AST::SubExpr:            return nm_AtSubExpr();
      case AST::MulExpr:            return nm_AtMulExpr();
      case AST::DivExpr:            return nm_AtDivExpr();
      case AST::ParenExpr:          return nm_AtParenExpr();
      case AST::NameExpr:           return nm_AtNameExpr();
      case AST::IntegerExpr:        return nm_AtIntegerExpr();
      default:                      return nullptr;
    }
}


/* static */ Result<ThreadState*>
ThreadState::Create(AllocationContext acx)
{
    // Initialize the global.
    Local<VM::GlobalScope*> glob(acx);
    if (!glob.setResult(VM::GlobalScope::Create(acx)))
        return ErrorVal();

    if (!Interp::BindSyntaxHandlers(acx, glob))
        return ErrorVal();

    // Initialize the root delegate.
    Local<VM::Wobject*> rootDelegate(acx);
    if (!rootDelegate.setResult(Interp::CreateRootDelegate(acx)))
        return ErrorVal();

    // Initialize the immediate integer delegate.
    Local<VM::Wobject*> immIntDelegate(acx);
    if (!immIntDelegate.setResult(
            Interp::CreateImmIntDelegate(acx, rootDelegate)))
    {
        return ErrorVal();
    }

    // Initialize the immediate boolean delegate.
    Local<VM::Wobject*> immBoolDelegate(acx);
    if (!immBoolDelegate.setResult(
            Interp::CreateImmBoolDelegate(acx, rootDelegate)))
    {
        return ErrorVal();
    }

    return acx.create<ThreadState>(glob.handle(), rootDelegate.handle(),
                                   immIntDelegate.handle(),
                                   immBoolDelegate.handle());
}


} // namespace VM
} // namespace Whisper
