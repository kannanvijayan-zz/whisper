
#include <cstdio>

#include "vm/core.hpp"
#include "vm/exception.hpp"
#include "vm/string.hpp"
#include "runtime_inlines.hpp"

namespace Whisper {
namespace VM {


size_t
Exception::snprint(char* buf, size_t n)
{
    if (this->isInternalException())
        return this->toInternalException()->snprint(buf, n);
#define EXCEPTION_KIND_SNPRINT_(name) \
    if (this->is##name()) \
        return this->to##name()->snprint(buf, n);
    WHISPER_DEFN_EXCEPTION_KINDS(EXCEPTION_KIND_SNPRINT_)
#undef EXCEPTION_KIND_SNPRINT_

    WH_UNREACHABLE("Unknown exception variant.");
    return std::numeric_limits<size_t>::max();
}

/* static */ Result<NameLookupFailedException*>
NameLookupFailedException::Create(AllocationContext acx,
                                  Handle<Wobject*> object,
                                  Handle<String*> name)
{
    return acx.create<NameLookupFailedException>(object, name);
}

size_t
NameLookupFailedException::snprint(char* buf, size_t n)
{
    char* end = buf + n;
    char* cur = buf;

    auto ensureNullByte = MakeScopeExit([&] {
        if (cur < end) {
            *cur = '\0';
        } else if (n > 0) {
            end[-1] = '\0';
        }
    });

    auto recordWr = [&] (size_t wr) -> size_t {
        size_t left = static_cast<size_t>(end - cur);
        if (wr < left) {
            cur += wr;
        } else {
            cur = end;
        }
        return wr;
    };

    return recordWr(snprintf(cur, end - cur, "Name lookup failed: %s",
                                name_->c_chars()));
}

/* static */ Result<FunctionNotOperativeException*>
FunctionNotOperativeException::Create(AllocationContext acx,
                                      Handle<FunctionObject*> func)
{
    return acx.create<FunctionNotOperativeException>(func);
}

size_t
FunctionNotOperativeException::snprint(char* buf, size_t n)
{
    char* end = buf + n;
    char* cur = buf;

    auto ensureNullByte = MakeScopeExit([&] {
        if (cur < end) {
            *cur = '\0';
        } else if (n > 0) {
            end[-1] = '\0';
        }
    });

    auto recordWr = [&] (size_t wr) -> size_t {
        size_t left = static_cast<size_t>(end - cur);
        if (wr < left) {
            cur += wr;
        } else {
            cur = end;
        }
        return wr;
    };

    return recordWr(snprintf(cur, end - cur, "Function not operative."));
}

/* static */ Result<InternalException*>
InternalException::Create(AllocationContext acx,
                          char const* message,
                          ArrayHandle<Box> const& arguments)
{
    uint32_t numArguments = arguments.length();
    return acx.createSized<InternalException>(CalculateSize(numArguments),
                message, numArguments, arguments);
}

size_t
InternalException::snprint(char* buf, size_t n)
{
    char* end = buf + n;
    char* cur = buf;
    size_t written = 0;

    auto ensureNullByte = MakeScopeExit([&] {
        if (cur < end) {
            *cur = '\0';
        } else if (n > 0) {
            end[-1] = '\0';
        }
    });

    auto recordWr = [&] (size_t wr) -> size_t {
        size_t left = static_cast<size_t>(end - cur);
        if (wr < left) {
            cur += wr;
        } else {
            cur = end;
        }
        return wr;
    };

    if (numArguments_ == 0)
        return recordWr(snprintf(cur, end - cur, "%s", message_));

    written += recordWr(snprintf(cur, end - cur, "%s: ", message_));
    for (uint32_t i = 0; i < numArguments_; i++) {
        if (i > 0)
            written += recordWr(snprintf(cur, end - cur, ", "));

        written += recordWr(arguments_[i]->snprint(cur, end - cur));
    }
    return written;
}



} // namespace VM
} // namespace Whisper
