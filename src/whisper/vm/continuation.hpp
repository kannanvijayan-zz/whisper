#ifndef WHISPER__VM__CONTINUATION_HPP
#define WHISPER__VM__CONTINUATION_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"
#include "vm/control_flow.hpp"
#include "vm/wobject.hpp"
#include "vm/hash_object.hpp"

namespace Whisper {
namespace VM {


class Continuation
{
  friend class TraceTraits<Continuation>;
  private:
    HeapField<Frame*> frame_;

  public:
    Continuation(Frame* frame)
      : frame_(frame)
    {}

    Frame* frame() const {
        return frame_;
    }

    static Result<Continuation*> Create(AllocationContext cx,
                                        Handle<Frame*> frame);

    StepResult continueWith(ThreadContext* cx, Handle<ValBox> value) const;
};

class ContObject : public HashObject
{
  friend class TraceTraits<ContObject>;
  private:
    HeapField<Continuation*> cont_;

  public:
    ContObject(Array<Wobject*>* delegates,
               PropertyDict* dict,
               Continuation* cont)
      : HashObject(delegates, dict),
        cont_(cont)
    {}

    static Result<ContObject*> Create(AllocationContext acx,
                                      Handle<Continuation*> cont);

    WobjectHooks const* getContObjectHooks() const;

    Continuation* cont() const {
        return cont_;
    }
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::Continuation>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::Continuation const& obj,
                     void const* start, void const* end)
    {
        obj.frame_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::Continuation& obj,
                       void const* start, void const* end)
    {
        obj.frame_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::ContObject>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::ContObject const& obj,
                     void const* start, void const* end)
    {
        obj.cont_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::ContObject& obj,
                       void const* start, void const* end)
    {
        obj.cont_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__CONTINUATION_HPP
