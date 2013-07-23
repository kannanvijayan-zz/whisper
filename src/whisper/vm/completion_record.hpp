#ifndef WHISPER__VM__COMPLETION_RECORD_HPP
#define WHISPER__VM__COMPLETION_RECORD_HPP

#include "common.hpp"
#include "debug.hpp"
#include "value.hpp"
#include "rooting.hpp"

namespace Whisper {
namespace VM {

//
// A Completion Record, as per ECMA 262 edition 6, part 8.2.3
// "The Competion Record Specification Type"
//
// A completion record object uses the flag bits in the object header
// to store its type.
//
class CompletionRecord : public HeapThingPayload<HeapType::CompletionRecord>
{
  public:
    enum CompletionType : uint8_t { Normal, Break, Continue, Return, Throw };
    static constexpr uint32_t TypeShift = 0;
    static constexpr uint32_t TypeMask = 0x7u;
    static constexpr uint32_t HasValueShift = 3;

  private:
    Value valueOrTarget_;

    void initCompletionRecordFlags(CompletionType type, bool hasVal) {
        WH_ASSERT((static_cast<uint32_t>(type) & TypeMask) == 0);
        uint32_t flags = static_cast<uint32_t>(type) << TypeShift;
        flags |= static_cast<uint32_t>(hasVal) << HasValueShift;
        initFlags(flags);
    }

  public:
    CompletionRecord(CompletionType type)
      : valueOrTarget_(UndefinedValue())
    {
        initCompletionRecordFlags(type, false);
    }

    CompletionRecord(CompletionType type, Value val)
      : valueOrTarget_(val)
    {
        initCompletionRecordFlags(type, true);
    }

  private:
    inline CompletionType type() const {
        return static_cast<CompletionType>(flags() & TypeMask);
    }

    inline bool hasValueOrTarget() const {
        return flags() & (static_cast<uint32_t>(1) << HasValueShift);
    }

  public:
    inline bool isNormal() const {
        return type() == Normal;
    }

    inline bool isBreak() const {
        return type() == Break;
    }

    inline bool isContinue() const {
        return type() == Continue;
    }

    inline bool isReturn() const {
        return type() == Return;
    }

    inline bool isThrow() const {
        return type() == Throw;
    }

    inline bool isAbrupt() const {
        return type() != Normal;
    }

    inline bool hasValue() const {
        WH_ASSERT(isNormal() || isReturn() || isThrow());
        return hasValueOrTarget();
    }

    inline const Value &value() const {
        WH_ASSERT(hasValue());
        return valueOrTarget_;
    }

    inline bool hasTarget() const {
        WH_ASSERT(isBreak() || isContinue());
        return hasValueOrTarget();
    }

    inline const Value &target() const {
        WH_ASSERT(hasTarget());
        return valueOrTarget_;
    }
};

typedef HeapThingWrapper<CompletionRecord> WrappedCompletionRecord;


} // namespace VM
} // namespace Whisper

#endif // WHISPER__VM__COMPLETION_RECORD_HPP
