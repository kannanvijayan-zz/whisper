#ifndef WHISPER__VM__EXCEPTION_HPP
#define WHISPER__VM__EXCEPTION_HPP


#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace VM {


//
// Base class for exceptions.
//
class Exception
{
  friend class TraceTraits<Exception>;
  protected:
    Exception() {}

  public:
    bool isInternalException() const {
        return HeapThing::From(this)->isInternalException();
    }

    InternalException* toInternalException() {
        WH_ASSERT(isInternalException());
        return reinterpret_cast<InternalException*>(this);
    }

    size_t snprint(char* buf, size_t n);
};

//
// An internal has a string message, and zero or more Box arguments
// indicating the exception data.
//
class InternalException : public Exception
{
  friend class TraceTraits<InternalException>;
  private:
    char const* message_;
    uint32_t numArguments_;
    HeapField<Box> arguments_[0];

  public:
    InternalException(char const* message,
                      uint32_t numArguments,
                      Box const* arguments)
      : Exception(),
        message_(message),
        numArguments_(numArguments)
    {
        for (uint32_t i = 0; i < numArguments; i++) {
            arguments_[i].init(arguments[i], this);
        }
    }

    static uint32_t CalculateSize(uint32_t numArguments) {
        return sizeof(InternalException) +
                (numArguments * sizeof(HeapField<Box>));
    }

    static Result<InternalException*> Create(AllocationContext acx,
                                             char const* message,
                                             ArrayHandle<Box> const& args);

    static Result<InternalException*> Create(AllocationContext acx,
                                             char const* message)
    {
        return Create(acx, message, ArrayHandle<Box>::Empty());
    }

    template <typename T>
    static Result<InternalException*> Create(AllocationContext acx,
                                             char const* message,
                                             Handle<T*> arg)
    {
        Local<Box> args(acx, Box::Pointer(arg.get()));
        return Create(acx, message, ArrayHandle<Box>(args));
    }

    static Result<InternalException*> Create(AllocationContext acx,
                                             char const* message,
                                             Handle<Box> arg)
    {
        return Create(acx, message, ArrayHandle<Box>(arg));
    }

    static Result<InternalException*> Create(AllocationContext acx,
                                             char const* message,
                                             Handle<ValBox> arg)
    {
        Local<Box> args(acx, arg.get());
        return Create(acx, message, ArrayHandle<Box>(args));
    }

    char const* message() const {
        return message_;
    }

    uint32_t numArguments() const {
        return numArguments_;
    }

    Box const& argument(uint32_t argNo) const {
        WH_ASSERT(argNo < numArguments_);
        return arguments_[argNo];
    }

    size_t snprint(char* buf, size_t n);
};

//
// Exception raised when a lexical name lookup fails.
//
class NameLookupFailedException : public Exception
{
  friend class TraceTraits<NameLookupFailedException>;
  private:
    // Object name was looked up on.
    HeapField<Wobject*> object_;

    // Name that failed to be looked up.
    HeapField<String*> name_;

  public:
    NameLookupFailedException(Wobject* object, String* name)
      : Exception(),
        object_(object),
        name_(name)
    {
    }

    static Result<NameLookupFailedException*> Create(
            AllocationContext acx,
            Handle<Wobject*> object,
            Handle<String*> name);

    Wobject* object() const {
        return object_;
    }

    String* name() const {
        return name_;
    }

    size_t snprint(char* buf, size_t n);
};

//
// Exception raised when a non-operative function is used
// in an operative context.
//
class FunctionNotOperativeException : public Exception
{
  friend class TraceTraits<FunctionNotOperativeException>;
  private:
    // The function object in question.
    HeapField<FunctionObject*> func_;

  public:
    FunctionNotOperativeException(FunctionObject* func)
      : Exception(),
        func_(func)
    {}

    static Result<FunctionNotOperativeException*> Create(
            AllocationContext acx,
            Handle<FunctionObject*> func);

    FunctionObject* func() const {
        return func_;
    }

    size_t snprint(char* buf, size_t n);
};


} // namespace VM


//
// GC Specializations
//

template <>
struct TraceTraits<VM::Exception>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::Exception const& obj,
                     void const* start, void const* end)
    {
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::Exception& obj,
                       void const* start, void const* end)
    {
    }
};

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

template <>
struct TraceTraits<VM::NameLookupFailedException>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::NameLookupFailedException const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Exception>::Scan<Scanner>(scanner, obj, start, end);
        obj.object_.scan(scanner, start, end);
        obj.name_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::NameLookupFailedException& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Exception>::Update<Updater>(updater, obj, start, end);
        obj.object_.update(updater, start, end);
        obj.name_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::FunctionNotOperativeException>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::FunctionNotOperativeException const& obj,
                     void const* start, void const* end)
    {
        TraceTraits<VM::Exception>::Scan<Scanner>(scanner, obj, start, end);
        obj.func_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::FunctionNotOperativeException& obj,
                       void const* start, void const* end)
    {
        TraceTraits<VM::Exception>::Update<Updater>(updater, obj, start, end);
        obj.func_.update(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__EXCEPTION_HPP
