#ifndef WHISPER__VM__CONTROL_FLOW_HPP
#define WHISPER__VM__CONTROL_FLOW_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"
#include "vm/exception.hpp"

namespace Whisper {
namespace VM {

class Frame;

// Result of an interpretation of a code.
class EvalResult
{
  friend class TraceTraits<EvalResult>;
  public:
    enum class Outcome {
        Error,
        Exc,
        Value,
        Void
    };

  private:
    Outcome outcome_;
    StackField<ValBox> value_;
    StackField<Frame*> frame_;
    StackField<Exception*> exception_;

    EvalResult(Outcome outcome)
      : outcome_(outcome),
        value_()
    {}

    EvalResult(Outcome outcome, ValBox const& value)
      : outcome_(outcome),
        value_(value),
        frame_(nullptr)
    {}

    EvalResult(Outcome outcome, Frame* frame, Exception* exception)
      : outcome_(outcome),
        value_(),
        frame_(frame),
        exception_(exception)
    {}

  public:
    EvalResult(ErrorT_ const& error)
      : EvalResult(Outcome::Error)
    {}

    static EvalResult Error() {
        return EvalResult(Outcome::Error);
    }
    static EvalResult Exc(Frame* frame, Exception* exception) {
        return EvalResult(Outcome::Exc, frame, exception);
    }
    static EvalResult Value(ValBox const& value) {
        return EvalResult(Outcome::Value, value);
    }
    static EvalResult Void() {
        return EvalResult(Outcome::Void);
    }

    Outcome outcome() const {
        return outcome_;
    }
    char const* outcomeString() const {
        return OutcomeString(outcome());
    }
    static char const* OutcomeString(Outcome outcome) {
        switch (outcome) {
          case Outcome::Error:       return "Error";
          case Outcome::Exc:         return "Exc";
          case Outcome::Value:       return "Value";
          case Outcome::Void:        return "Void";
          default:
            WH_UNREACHABLE("Unknown outcome");
            return "UNKNOWN";
        }
    }

    bool isError() const {
        return outcome() == Outcome::Error;
    }
    bool isExc() const {
        return outcome() == Outcome::Exc;
    }
    bool isValue() const {
        return outcome() == Outcome::Value;
    }
    bool isVoid() const {
        return outcome() == Outcome::Void;
    }

    Handle<ValBox> value() const {
        WH_ASSERT(isValue());
        return value_;
    }
    Handle<Frame*> throwingFrame() const {
        WH_ASSERT(isExc());
        return frame_;
    }
    Handle<Exception*> exception() const {
        WH_ASSERT(isExc());
        return exception_;
    }
};

// Intermediate result produced when calling a function.
class CallResult
{
  friend class TraceTraits<CallResult>;
  public:
    enum class Outcome {
        Error,
        Exc,
        Value,
        Void,
        Continue
    };

  private:
    Outcome outcome_;
    StackField<ValBox> value_;
    StackField<Frame*> frame_;
    StackField<Exception*> exception_;

    CallResult(Outcome outcome)
      : outcome_(outcome),
        value_()
    {}

    CallResult(Outcome outcome, ValBox const& value)
      : outcome_(outcome),
        value_(value),
        frame_(nullptr)
    {}

    CallResult(Outcome outcome, Frame* frame)
      : outcome_(outcome),
        value_(),
        frame_(frame),
        exception_()
    {}

    CallResult(Outcome outcome, Frame* frame, Exception* exception)
      : outcome_(outcome),
        value_(),
        frame_(frame),
        exception_(exception)
    {}

  public:
    CallResult(ErrorT_ const& error)
      : CallResult(Outcome::Error)
    {}

    static CallResult Error() {
        return CallResult(Outcome::Error);
    }
    static CallResult Exc(Frame* frame, Exception* exception) {
        return CallResult(Outcome::Exc, frame, exception);
    }
    static CallResult Value(ValBox const& value) {
        return CallResult(Outcome::Value, value);
    }
    static CallResult Void() {
        return CallResult(Outcome::Void);
    }
    static CallResult Continue(Frame* frame) {
        return CallResult(Outcome::Continue, frame);
    }

    Outcome outcome() const {
        return outcome_;
    }
    char const* outcomeString() const {
        return OutcomeString(outcome());
    }
    static char const* OutcomeString(Outcome outcome) {
        switch (outcome) {
          case Outcome::Error:       return "Error";
          case Outcome::Exc:         return "Exc";
          case Outcome::Value:       return "Value";
          case Outcome::Void:        return "Void";
          case Outcome::Continue:    return "Continue";
          default:
            WH_UNREACHABLE("Unknown outcome");
            return "UNKNOWN";
        }
    }

    bool isError() const {
        return outcome() == Outcome::Error;
    }
    bool isExc() const {
        return outcome() == Outcome::Exc;
    }
    bool isValue() const {
        return outcome() == Outcome::Value;
    }
    bool isVoid() const {
        return outcome() == Outcome::Void;
    }
    bool isContinue() const {
        return outcome() == Outcome::Continue;
    }

    Handle<ValBox> value() const {
        WH_ASSERT(isValue());
        return value_;
    }
    Handle<Frame*> throwingFrame() const {
        WH_ASSERT(isExc());
        return frame_;
    }
    Handle<Frame*> continueFrame() const {
        WH_ASSERT(isContinue());
        return frame_;
    }
    Handle<Exception*> exception() const {
        WH_ASSERT(isExc());
        return exception_;
    }

    EvalResult errorAsEvalResult() const {
        WH_ASSERT(isError());
        return EvalResult::Error();
    }
    EvalResult valueAsEvalResult() const {
        WH_ASSERT(isValue());
        return EvalResult::Value(value());
    }
    EvalResult excAsEvalResult() const {
        WH_ASSERT(isExc());
        return EvalResult::Exc(throwingFrame(), exception());
    }
    EvalResult voidAsEvalResult() const {
        WH_ASSERT(isVoid());
        return EvalResult::Void();
    }

    EvalResult asEvalResult() const {
        WH_ASSERT(isError() || isExc() || isValue() || isVoid());

        if (isError()) {
            return errorAsEvalResult();

        } else if (isExc()) {
            return excAsEvalResult();

        } else if (isValue()) {
            return valueAsEvalResult();

        } else if (isVoid()) {
            return voidAsEvalResult();

        } else if (isContinue()) {
            WH_UNREACHABLE("Invalid EvalResult outcome: Continue");

        }
        WH_UNREACHABLE("Invalid EvalResult outcome");
        return EvalResult::Void();
    }
};

// Result of a step function.
class StepResult
{
  friend class TraceTraits<StepResult>;
  public:
    enum class Outcome {
        Error,
        Continue
    };

  private:
    Outcome outcome_;
    StackField<Frame*> frame_;

    StepResult(Outcome outcome)
      : outcome_(outcome)
    {}

    StepResult(Outcome outcome, Frame* frame)
      : outcome_(outcome),
        frame_(frame)
    {}

  public:
    StepResult(ErrorT_ const& error)
      : StepResult(Outcome::Error)
    {}

    static StepResult Error() {
        return StepResult(Outcome::Error);
    }
    static StepResult Continue(Frame* frame) {
        return StepResult(Outcome::Continue, frame);
    }

    Outcome outcome() const {
        return outcome_;
    }
    char const* outcomeString() const {
        return OutcomeString(outcome());
    }
    static char const* OutcomeString(Outcome outcome) {
        switch (outcome) {
          case Outcome::Error:       return "Error";
          case Outcome::Continue:    return "Continue";
          default:
            WH_UNREACHABLE("Unknown outcome");
            return "UNKNOWN";
        }
    }

    bool isError() const {
        return outcome() == Outcome::Error;
    }
    bool isContinue() const {
        return outcome() == Outcome::Continue;
    }

    Handle<Frame*> continueFrame() const {
        WH_ASSERT(isContinue());
        return frame_;
    }
};


} // namespace VM


//
// GC-Specializations.
//

template <>
struct TraceTraits<VM::EvalResult>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::EvalResult const& er,
                     void const* start, void const* end)
    {
        er.value_.scan(scanner, start, end);
        er.frame_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::EvalResult& er,
                       void const* start, void const* end)
    {
        er.value_.update(updater, start, end);
        er.frame_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::CallResult>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::CallResult const& cr,
                     void const* start, void const* end)
    {
        cr.value_.scan(scanner, start, end);
        cr.frame_.scan(scanner, start, end);
        cr.exception_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::CallResult& cr,
                       void const* start, void const* end)
    {
        cr.value_.update(updater, start, end);
        cr.frame_.update(updater, start, end);
        cr.exception_.update(updater, start, end);
    }
};

template <>
struct TraceTraits<VM::StepResult>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::StepResult const& sr,
                     void const* start, void const* end)
    {
        sr.frame_.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::StepResult& sr,
                       void const* start, void const* end)
    {
        sr.frame_.update(updater, start, end);
    }
};


} // namespace Whisper

#endif // WHISPER__VM__CONTROL_FLOW_HPP
