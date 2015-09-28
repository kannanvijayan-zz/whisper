#ifndef WHISPER__VM__CONTROL_FLOW_HPP
#define WHISPER__VM__CONTROL_FLOW_HPP

#include "vm/core.hpp"
#include "vm/predeclare.hpp"
#include "vm/box.hpp"

namespace Whisper {
namespace VM {


class ControlFlow
{
    friend class TraceTraits<ControlFlow>;
  public:
    enum class Kind {
        Void,
        Error,
        Value,
        Return,
        Exception
    };

  private:
    static constexpr size_t ValBoxSize = sizeof(ValBox);
    static constexpr size_t ValBoxAlign = alignof(ValBox);

    static constexpr size_t CStringSize = sizeof(char const*);
    static constexpr size_t CStringAlign = alignof(char const*);

    static constexpr size_t PayloadSize =
        ConstExprMax<size_t, ValBoxSize, CStringSize>();
    static constexpr size_t PayloadAlign =
        ConstExprMax<size_t, ValBoxAlign, CStringAlign>();

    Kind kind_;
    alignas(PayloadAlign)
    uint8_t payload_[PayloadSize];

    ValBox* valBoxPayload() {
        return reinterpret_cast<ValBox*>(payload_);
    }
    ValBox const* valBoxPayload() const {
        return reinterpret_cast<ValBox const*>(payload_);
    }

    char const** cStringPayload() {
        return reinterpret_cast<char const**>(payload_);
    }
    char const* const* cStringPayload() const {
        return reinterpret_cast<char const* const*>(payload_);
    }

    void initCStringPayload(char const* msg) {
        new (cStringPayload()) (char const*)(msg);
    }
    void initValBoxPayload(ValBox const& val) {
        new (valBoxPayload()) ValBox(val);
    }

  private:
    ControlFlow(Kind kind) : kind_(Kind::Void) {}

    ControlFlow(Kind kind, char const* msg)
      : kind_(kind)
    {
        initCStringPayload(msg);
    }

    ControlFlow(Kind kind, ValBox const& val)
      : kind_(kind)
    {
        initValBoxPayload(val);
    }

  public:
    ControlFlow(ControlFlow const& other)
      : kind_(other.kind_)
    {
        switch (kind_) {
          case Kind::Void:
          case Kind::Error:
            break;

          case Kind::Value:
          case Kind::Return:
          case Kind::Exception:
            initValBoxPayload(*other.valBoxPayload());
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
            break;

          case Kind::Value:
          case Kind::Return:
          case Kind::Exception:
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
    static ControlFlow Exception(ValBox const& val) {
        return ControlFlow(Kind::Exception, val);
    }

    Kind kind() const {
        return kind_;
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
    bool isException() const {
        return kind() == Kind::Exception;
    }

    bool isExpressionResult() const {
        return isValue() || isError() || isException();
    }

    ValBox const& value() const {
        WH_ASSERT(isValue());
        return *valBoxPayload();
    }
    ValBox const& returnValue() const {
        WH_ASSERT(isReturn());
        return *valBoxPayload();
    }
    ValBox const& exceptionValue() const {
        WH_ASSERT(isException());
        return *valBoxPayload();
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
            return;
          case VM::ControlFlow::Kind::Value:
          case VM::ControlFlow::Kind::Return:
          case VM::ControlFlow::Kind::Exception:
            TraceTraits<VM::ValBox>::Scan(scanner, *cf.valBoxPayload(),
                                          start, end);
            return;
        }
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::ControlFlow& cf,
                       void const* start, void const* end)
    {
        switch (cf.kind()) {
          case VM::ControlFlow::Kind::Void:
          case VM::ControlFlow::Kind::Error:
            return;
          case VM::ControlFlow::Kind::Value:
          case VM::ControlFlow::Kind::Return:
          case VM::ControlFlow::Kind::Exception:
            TraceTraits<VM::ValBox>::Update(updater, *cf.valBoxPayload(),
                                            start, end);
            return;
        }
    }
};


} // namespace Whisper

#endif // WHISPER__VM__CONTROL_FLOW_HPP
