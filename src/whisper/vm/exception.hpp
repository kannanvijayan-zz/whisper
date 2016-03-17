#ifndef WHISPER__VM__EXCEPTION_HPP
#define WHISPER__VM__EXCEPTION_HPP


#include "vm/core.hpp"
#include "vm/predeclare.hpp"

namespace Whisper {
namespace VM {


//
// Base class for exceptions.
//
class Exception
{
  protected:
    Exception() {}
};

//
// An internal has a string message, and zero or more ValBox arguments
// indicating the exception data.
//
class InternalException : public Exception
{
  friend class TraceTraits<InternalException>;
  private:
    char const* message_;
    uint32_t numArguments_;
    HeapField<ValBox> arguments_[0];

  public:
    InternalException(char const* message,
                      uint32_t numArguments,
                      ValBox const* arguments)
      : message_(message),
        numArguments_(numArguments)
    {
        for (uint32_t i = 0; i < numArguments; i++) {
            arguments_[i].init(arguments[i], this);
        }
    }
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::Exception>
  : public UntracedTraceTraits<VM::Exception>
{};

template <>
struct TraceTraits<VM::InternalException>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::InternalException const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Exception>::Scan<Scanner>(scanner, obj, start, end);
        for (uint32_t i = 0; i < obj.numArguments_; i++) {
            obj.arguments_[i].scan(scanner, start, end);
        }
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::InternalException& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Exception>::Update<Updater>(updater, obj, start, end);
        for (uint32_t i = 0; i < obj.numArguments_; i++) {
            obj.arguments_[i].update(updater, start, end);
        }
    }
};


} // namespace Whisper


#endif // WHISPER__VM__EXCEPTION_HPP
