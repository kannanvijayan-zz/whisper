#ifndef WHISPER__VM__CONTINUATION_HPP
#define WHISPER__VM__CONTINUATION_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/frame.hpp"

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


} // namespace Whisper


#endif // WHISPER__VM__CONTINUATION_HPP
