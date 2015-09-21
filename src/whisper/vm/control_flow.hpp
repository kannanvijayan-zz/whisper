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

    static constexpr size_t CStringSize = sizeof(const char *);
    static constexpr size_t CStringAlign = alignof(const char *);

    static constexpr size_t PayloadSize =
        ConstExprMax<size_t, ValBoxSize, CStringSize>();
    static constexpr size_t PayloadAlign =
        ConstExprMax<size_t, ValBoxAlign, CStringAlign>();

    Kind kind_;
    alignas(PayloadAlign)
    uint8_t payload_[PayloadSize];

    ValBox *valBoxPayload() {
        return reinterpret_cast<ValBox *>(payload_);
    }
    const ValBox *valBoxPayload() const {
        return reinterpret_cast<const ValBox *>(payload_);
    }

    const char **cStringPayload() {
        return reinterpret_cast<const char **>(payload_);
    }
    const char * const *cStringPayload() const {
        return reinterpret_cast<const char * const*>(payload_);
    }

    void initCStringPayload(const char *msg) {
        new (cStringPayload()) (const char *)(msg);
    }
    void initValBoxPayload(const ValBox &val) {
        new (valBoxPayload()) ValBox(val);
    }

  private:
    ControlFlow(Kind kind) : kind_(Kind::Void) {}

    ControlFlow(Kind kind, const char *msg)
      : kind_(kind)
    {
        initCStringPayload(msg);
    }

    ControlFlow(Kind kind, const ValBox &val)
      : kind_(kind)
    {
        initValBoxPayload(val);
    }

  public:
    ControlFlow(const ControlFlow &other)
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
    ControlFlow(const ErrorT_ &err)
      : kind_(Kind::Error)
    {}
    ControlFlow(const OkT_ &ok)
      : kind_(Kind::Value)
    {
        initValBoxPayload(ValBox::Undefined());
    }
    ControlFlow(const OkValT_<ValBox> &okVal)
      : kind_(Kind::Value)
    {
        initValBoxPayload(okVal.val());
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
    static ControlFlow Value(const ValBox &val) {
        return ControlFlow(Kind::Value, val);
    }
    static ControlFlow Return(const ValBox &val) {
        return ControlFlow(Kind::Return, val);
    }
    static ControlFlow Exception(const ValBox &val) {
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

    const ValBox &value() const {
        WH_ASSERT(isValue());
        return *valBoxPayload();
    }
    const ValBox &returnValue() const {
        WH_ASSERT(isReturn());
        return *valBoxPayload();
    }
    const ValBox &exceptionValue() const {
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
    static void Scan(Scanner &scanner, const VM::ControlFlow &cf,
                     const void *start, const void *end)
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
    static void Update(Updater &updater, VM::ControlFlow &cf,
                       const void *start, const void *end)
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
