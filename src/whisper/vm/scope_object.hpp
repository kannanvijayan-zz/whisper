#ifndef WHISPER__VM__SCOPE_OBJECT_HPP
#define WHISPER__VM__SCOPE_OBJECT_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/wobject.hpp"
#include "vm/hash_object.hpp"

namespace Whisper {
namespace VM {


class ScopeObject : public HashObject
{
  friend class TraceTraits<ScopeObject>;
  public:
    ScopeObject(Array<Wobject*>* delegates,
                PropertyDict* dict)
      : HashObject(delegates, dict)
    {}
};

class CallScope : public ScopeObject
{
  friend class TraceTraits<CallScope>;
  private:
    HeapField<Function*> function_;

  public:
    CallScope(Array<Wobject*>* delegates,
              PropertyDict* dict,
              Function* function)
      : ScopeObject(delegates, dict),
        function_(function)
    {}

    static Result<CallScope*> Create(AllocationContext acx,
                                     Handle<ScopeObject*> enclosingScope,
                                     Handle<Function*> function);

    WobjectHooks const* getCallScopeHooks() const;

    Function* function() const {
        return function_;
    }
};

class BlockScope : public ScopeObject
{
  friend class TraceTraits<BlockScope>;
  public:
    BlockScope(Array<Wobject*>* delegates,
               PropertyDict* dict)
      : ScopeObject(delegates, dict)
    {}

    static Result<BlockScope*> Create(AllocationContext acx,
                                      Handle<ScopeObject*> enclosingScope);

    WobjectHooks const* getBlockScopeHooks() const;
};

class ModuleScope : public ScopeObject
{
  friend class TraceTraits<ModuleScope>;
  public:
    ModuleScope(Array<Wobject*>* delegates,
                PropertyDict* dict)
      : ScopeObject(delegates, dict)
    {}

    static Result<ModuleScope*> Create(AllocationContext acx,
                                       Handle<GlobalScope*> global);

    WobjectHooks const* getModuleScopeHooks() const;
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::CallScope>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::CallScope const& scope,
                     void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, scope, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::CallScope& scope,
                       void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Update(updater, scope, start, end);
    }
};

template <>
struct TraceTraits<VM::BlockScope>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::BlockScope const& scope,
                     void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, scope, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::BlockScope& scope,
                       void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Update(updater, scope, start, end);
    }
};

template <>
struct TraceTraits<VM::ModuleScope>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::ModuleScope const& scope,
                     void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Scan(scanner, scope, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::ModuleScope& scope,
                       void const* start, void const* end)
    {
        TraceTraits<VM::HashObject>::Update(updater, scope, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__SCOPE_OBJECT_HPP
