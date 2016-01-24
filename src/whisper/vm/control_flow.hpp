#ifndef WHISPER__VM__CONTROL_FLOW_HPP
#define WHISPER__VM__CONTROL_FLOW_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"

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
        Exception,
        Value,
        Void
    };

  private:
    Outcome outcome_;
    StackField<VM::ValBox> value_;
    StackField<VM::Frame*> thrower_;

    EvalResult(Outcome outcome)
      : outcome_(outcome),
        value_()
    {}

    EvalResult(Outcome outcome, VM::ValBox const& value)
      : outcome_(outcome),
        value_(value),
        thrower_(nullptr)
    {}

    EvalResult(Outcome outcome, VM::Frame* frame)
      : outcome_(outcome),
        value_(),
        thrower_(frame)
    {}

  public:
    EvalResult(ErrorT_ const& error)
      : EvalResult(Outcome::Error)
    {}

    static EvalResult Error() {
        return EvalResult(Outcome::Error);
    }
    static EvalResult Exception(VM::Frame* thrower) {
        return EvalResult(Outcome::Exception, thrower);
    }
    static EvalResult Value(VM::ValBox const& value) {
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
          case Outcome::Exception:   return "Exception";
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
    bool isException() const {
        return outcome() == Outcome::Exception;
    }
    bool isValue() const {
        return outcome() == Outcome::Value;
    }
    bool isVoid() const {
        return outcome() == Outcome::Void;
    }

    VM::ValBox const& value() const {
        WH_ASSERT(isValue());
        return value_;
    }
    VM::Frame* thrower() const {
        WH_ASSERT(isException());
        return thrower_;
    }
};

class ControlFlow
{
    friend class TraceTraits<ControlFlow>;
  public:
    enum class Kind {
        Error,
        Exception,
        Value,
        Void,
        Return,
        Continue
    };

  private:
    static constexpr size_t ValBoxSize = sizeof(ValBox);
    static constexpr size_t ValBoxAlign = alignof(ValBox);

    static constexpr size_t FramePointerSize = sizeof(Frame*);
    static constexpr size_t FramePointerAlign = alignof(Frame*);

    static constexpr size_t PayloadSize = ConstExprMax<size_t, ValBoxSize, FramePointerSize>();
    static constexpr size_t PayloadAlign = ConstExprMax<size_t, ValBoxAlign, FramePointerAlign>();

    Kind kind_;
    alignas(PayloadAlign)
    uint8_t payload_[PayloadSize];

    ValBox* valBoxPayload() {
        return reinterpret_cast<ValBox*>(payload_);
    }
    ValBox const* valBoxPayload() const {
        return reinterpret_cast<ValBox const*>(payload_);
    }
    void initValBoxPayload(ValBox const& val) {
        new (valBoxPayload()) ValBox(val);
    }

    Frame** framePointerPayload() {
        return reinterpret_cast<Frame**>(payload_);
    }
    Frame* const* framePointerPayload() const {
        return reinterpret_cast<Frame* const*>(payload_);
    }
    void initFramePointerPayload(Frame* frame) {
        *framePointerPayload() = frame;
    }

  private:
    ControlFlow(Kind kind) : kind_(kind) {}

    ControlFlow(Kind kind, ValBox const& val)
      : kind_(kind)
    {
        initValBoxPayload(val);
    }

    ControlFlow(Kind kind, Frame* frame)
      : kind_(kind)
    {
        initFramePointerPayload(frame);
    }

  public:
    ControlFlow(ControlFlow const& other)
      : kind_(other.kind_)
    {
        switch (kind_) {
          case Kind::Void:
          case Kind::Error:
          case Kind::Exception:
            break;

          case Kind::Value:
          case Kind::Return:
            initValBoxPayload(*other.valBoxPayload());
            break;

          case Kind::Continue:
            initFramePointerPayload(*other.framePointerPayload());
            break;

          default:
            WH_UNREACHABLE("Bad ControlFlow Kind.");
        }
    }
    ControlFlow(ErrorT_ const& err)
      : kind_(Kind::Error)
    {}
    ControlFlow(OkT_ const& ok)
      : kind_(Kind::Value)
    {
        initValBoxPayload(ValBox::Undefined());
    }
    ControlFlow(OkValT_<ValBox> const& okVal)
      : kind_(Kind::Value)
    {
        initValBoxPayload(okVal.val());
    }
    ControlFlow(OkValT_<Wobject*> const& okVal)
      : kind_(Kind::Value)
    {
        initValBoxPayload(ValBox::Pointer(okVal.val()));
    }

    ~ControlFlow()
    {
        switch (kind_) {
          case Kind::Void:
          case Kind::Error:
          case Kind::Exception:
          case Kind::Continue:
            break;

          case Kind::Value:
          case Kind::Return:
            valBoxPayload()->~ValBox();
            break;

          default:
            WH_UNREACHABLE("Bad ControlFlow Kind.");
        }
    }

    static ControlFlow Void() {
        return ControlFlow(Kind::Void);
    }
    static ControlFlow Error() {
        return ControlFlow(Kind::Error);
    }
    static ControlFlow Value(ValBox const& val) {
        return ControlFlow(Kind::Value, val);
    }
    static ControlFlow Value(Wobject* obj) {
        return ControlFlow(Kind::Value, ValBox::Pointer(obj));
    }
    static ControlFlow Return(ValBox const& val) {
        return ControlFlow(Kind::Return, val);
    }
    static ControlFlow Continue(Frame* frame) {
        return ControlFlow(Kind::Continue, frame);
    }
    static ControlFlow Exception() {
        return ControlFlow(Kind::Exception);
    }

    Kind kind() const {
        return kind_;
    }
    char const* kindString() const {
        switch (kind()) {
          case Kind::Void:        return "Void";
          case Kind::Error:       return "Error";
          case Kind::Value:       return "Value";
          case Kind::Return:      return "Return";
          case Kind::Continue:    return "Continue";
          case Kind::Exception:   return "Exception";
          default:
            WH_UNREACHABLE("Unknown kind");
            return "UNKNOWN";
        }
    }
    bool isVoid() const {
        return kind() == Kind::Void;
    }
    bool isError() const {
        return kind() == Kind::Error;
    }
    bool isValue() const {
        return kind() == Kind::Value;
    }
    bool isReturn() const {
        return kind() == Kind::Return;
    }
    bool isContinue() const {
        return kind() == Kind::Continue;
    }
    bool isException() const {
        return kind() == Kind::Exception;
    }

    ValBox const& value() const {
        WH_ASSERT(isValue());
        return *valBoxPayload();
    }
    ValBox const& returnValue() const {
        WH_ASSERT(isReturn());
        return *valBoxPayload();
    }
    Frame* continueFrame() const {
        WH_ASSERT(isContinue());
        return *framePointerPayload();
    }
};


} // namespace VM


//
// GC-Specializations.
//

template <>
struct TraceTraits<VM::ControlFlow>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::ControlFlow const& cf,
                     void const* start, void const* end)
    {
        switch (cf.kind()) {
          case VM::ControlFlow::Kind::Void:
          case VM::ControlFlow::Kind::Error:
          case VM::ControlFlow::Kind::Exception:
            return;
          case VM::ControlFlow::Kind::Value:
          case VM::ControlFlow::Kind::Return:
            TraceTraits<VM::ValBox>::Scan(scanner, *cf.valBoxPayload(),
                                          start, end);
            return;
          case VM::ControlFlow::Kind::Continue:
            TraceTraits<VM::Frame*>::Scan(scanner, *cf.framePointerPayload(),
                                          start, end);
            return;
          default:
            WH_UNREACHABLE("Unknown outcome");
        }
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::ControlFlow& cf,
                       void const* start, void const* end)
    {
        switch (cf.kind()) {
          case VM::ControlFlow::Kind::Void:
          case VM::ControlFlow::Kind::Error:
          case VM::ControlFlow::Kind::Exception:
            return;
          case VM::ControlFlow::Kind::Value:
          case VM::ControlFlow::Kind::Return:
            TraceTraits<VM::ValBox>::Update(updater, *cf.valBoxPayload(),
                                            start, end);
            return;

          case VM::ControlFlow::Kind::Continue:
            TraceTraits<VM::Frame*>::Update(updater, *cf.framePointerPayload(),
                                            start, end);
            return;
          default:
            WH_UNREACHABLE("Unknown outcome");
        }
    }
};

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
        switch (er.outcome()) {
          case VM::EvalResult::Outcome::Void:
          case VM::EvalResult::Outcome::Error:
          case VM::EvalResult::Outcome::Exception:
            return;
          case VM::EvalResult::Outcome::Value:
            er.value_.scan(scanner, start, end);
            return;
          default:
            WH_UNREACHABLE("Unknown outcome");
        }
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::EvalResult& er,
                       void const* start, void const* end)
    {
        switch (er.outcome()) {
          case VM::EvalResult::Outcome::Void:
          case VM::EvalResult::Outcome::Error:
          case VM::EvalResult::Outcome::Exception:
            return;
          case VM::EvalResult::Outcome::Value:
            er.value_.update(updater, start, end);
            return;
          default:
            WH_UNREACHABLE("Unknown outcome");
        }
    }
};


} // namespace Whisper

#endif // WHISPER__VM__CONTROL_FLOW_HPP
