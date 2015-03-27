#ifndef WHISPER__PARSER__PACKED_WRITER_HPP
#define WHISPER__PARSER__PACKED_WRITER_HPP

#include <unordered_map>
#include "allocators.hpp"
#include "runtime.hpp"
#include "fnv_hash.hpp"
#include "parser/code_source.hpp"
#include "parser/syntax_tree.hpp"
#include "vm/string.hpp"
#include "vm/box.hpp"
#include "gc.hpp"

namespace Whisper {
namespace AST {


class PackedWriterError
{
  public:
    PackedWriterError() {}
};

class IdentifierKey
{
  private:
    const uint8_t *text_;
    uint32_t length_;

  public:
    IdentifierKey(const uint8_t *text, uint32_t length)
      : text_(text),
        length_(length)
    {}

    const uint8_t *text() const {
        return text_;
    }

    uint32_t length() const {
        return length_;
    }

    size_t hash() const {
        FNVHash hash;
        for (uint32_t i = 0; i < length_; i++)
            hash.update(text_[i]);
        return hash.digest();
    }
    struct Hash
    {
        size_t operator ()(const IdentifierKey &info) const {
            return info.hash();
        }
    };

    bool operator ==(const IdentifierKey &other) const {
        if (length_ != other.length_)
            return false;
        for (uint32_t i = 0; i < length_; i++) {
            if (text_[i] != other.text_[i])
                return false;
        }
        return true;
    }
    struct Equal
    {
        size_t operator ()(const IdentifierKey &a,
                           const IdentifierKey &b) const
        {
            return a == b;
        }
    };
};

class PackedWriter
{
    friend class TraceTraits<PackedWriter>;
  public:
    typedef uint32_t *Position;

    // The maximum number size of the packed buffer and
    // the constant array, is 0x0fffffff == ((1 << 28) - 1).
    // This leaves 4 high bits unused.
    static constexpr uint32_t MaxBufferSize = (ToUInt32(1) << 28) - 1u;
    static constexpr uint32_t MaxConstPoolSize = (ToUInt32(1) << 28) - 1u;

  private:
    STLBumpAllocator<uint32_t> allocator_;
    const SourceReader &src_;
    AllocationContext &acx_;

    // Output packed syntax tree.
    static constexpr uint32_t InitialBufferSize = 128;
    uint32_t *start_;
    uint32_t *end_;
    uint32_t *cursor_;

    // Output constPool array.
    static constexpr uint32_t InitialConstPoolSize = 16;
    VM::Box *constPool_;
    uint32_t constPoolCapacity_;
    uint32_t constPoolSize_;

    // Hash table mapping identifier names
    typedef STLBumpAllocator<std::pair<const IdentifierKey, uint32_t>>
            IdentifierMapAllocator;
    typedef std::unordered_map<IdentifierKey,
                               uint32_t,
                               IdentifierKey::Hash,
                               IdentifierKey::Equal,
                               IdentifierMapAllocator>
            IdentifierMap;
    IdentifierMap identifierMap_;

    const char *error_;

  public:
    PackedWriter(const STLBumpAllocator<uint32_t> &allocator,
                 const SourceReader &src,
                 AllocationContext &acx)
      : allocator_(allocator),
        src_(src),
        acx_(acx),
        start_(nullptr),
        end_(nullptr),
        cursor_(nullptr),
        constPool_(nullptr),
        constPoolCapacity_(0),
        constPoolSize_(0),
        identifierMap_(IdentifierMapAllocator(allocator)),
        error_(nullptr)
    {}

    bool writeNode(const BaseNode *node);

    uint32_t bufferSize() const {
        return (cursor_ - start_);
    }
    ArrayHandle<uint32_t> buffer() const {
        return ArrayHandle<uint32_t>::FromTrackedLocation(start_,
                                                          bufferSize());
    }

    uint32_t constPoolSize() const {
        return constPoolSize_;
    }
    ArrayHandle<VM::Box> constPool() const {
        return ArrayHandle<VM::Box>::FromTrackedLocation(constPool_,
                                                         constPoolSize_);
    }
  private:
    void write(uint32_t word) {
        WH_ASSERT(cursor_ <= end_);
        if (cursor_ == end_)
            expandBuffer();
        *cursor_ = word;
        cursor_++;
    }
    void writeDummy() {
        write(static_cast<uint32_t>(-1));
    }

    void writeAt(Position pos, uint32_t word) {
        WH_ASSERT(pos >= start_ && pos < cursor_);
        *pos = word;
    }
    void writeOffsetDistance(Position *writePos, bool increment=true) {
        writeAt(*writePos, cursor_ - *writePos);
        if (increment)
            ++*writePos;
    }

    static constexpr uint32_t MaxTypeExtra = 0xfffffu;
    void writeTypeExtra(uint32_t extra) {
        WH_ASSERT(cursor_ > start_);
        WH_ASSERT(extra <= MaxTypeExtra);
        cursor_[-1] |= (extra << 12);
    }

    Position position() const {
        return cursor_;
    }
    uint32_t distanceFromPosition(Position pos) {
        WH_ASSERT(pos >= start_ && pos < cursor_);
        return cursor_ - pos;
    }

    bool hasError() const {
        return error_ != nullptr;
    }
    const char *error() const {
        WH_ASSERT(hasError());
        return error_;
    }
    void emitError(const char *msg) {
        WH_ASSERT(!hasError());
        error_ = msg;
        throw PackedWriterError();
    }

    void expandBuffer();

    static constexpr uint32_t MaxArgs = 0xffffu;
    static constexpr uint32_t MaxBlockStatements = 0xffffu;
    static constexpr uint32_t MaxElsifClauses = 0xffffu;
    static constexpr uint32_t MaxParams = 0xffffu;
    static constexpr uint32_t MaxBindings = 0xffffu;

    static constexpr uint32_t MaxConstants = 0xffffffu;
    uint32_t addIdentifier(const IdentifierToken &ident);
    uint32_t addToConstPool(VM::Box thing);
    void expandConstPool();

    void parseInteger(const IntegerLiteralToken &token, int32_t *resultOut);
    void parseBinInteger(const IntegerLiteralToken &token, int32_t *resultOut);
    void parseOctInteger(const IntegerLiteralToken &token, int32_t *resultOut);
    void parseDecInteger(const IntegerLiteralToken &token, int32_t *resultOut);
    void parseHexInteger(const IntegerLiteralToken &token, int32_t *resultOut);

    void writeBinaryExpr(const Expression *lhs, const Expression *rhs);
    void writeBlock(const Block *block);
    void writeSizedBlock(const Block *block);

#define METHOD_(ntype) void write##ntype(const ntype##Node *n);
    WHISPER_DEFN_SYNTAX_NODES(METHOD_)
#undef METHOD_
};


} // namespace AST


template <>
struct StackTraits<AST::PackedWriter>
{
    StackTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr StackFormat Format = StackFormat::PackedWriter;
};

template <>
struct StackFormatTraits<StackFormat::PackedWriter>
{
    StackFormatTraits() = delete;
    typedef AST::PackedWriter Type;
};

template <>
struct TraceTraits<AST::PackedWriter>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner &scanner, const AST::PackedWriter &pw,
                     const void *start, const void *end)
    {
        for (uint32_t i = 0; i < pw.constPoolSize_; i++) {
            TraceTraits<VM::Box>::Scan(scanner, pw.constPool_[i],
                                       start, end);
        }
    }

    template <typename Updater>
    static void Update(Updater &updater, AST::PackedWriter &pw,
                       const void *start, const void *end)
    {
        for (uint32_t i = 0; i < pw.constPoolSize_; i++) {
            TraceTraits<VM::Box>::Update(updater, pw.constPool_[i],
                                         start, end);
        }
    }
};


} // namespace Whisper

#endif // WHISPER__PARSER__PACKED_WRITER_HPP
