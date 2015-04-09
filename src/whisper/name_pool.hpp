#ifndef WHISPER__NAME_POOL_HPP
#define WHISPER__NAME_POOL_HPP

#include "vm/array.hpp"
#include "vm/string.hpp"

#define WHISPER_DEFN_NAME_POOL(_) \
    _(AtFile,       "@File"         ) \
    _(AtEmptyStmt,  "@EmptyStmt"    ) \
    _(AtExprStmt,   "@ExprStmt"     ) \
    _(AtReturnStmt, "@ReturnStmt"   ) \
    _(AtIfStmt,     "@IfStmt"       ) \
    _(AtDefStmt,    "@DefStmt"      ) \
    _(AtConstStmt,  "@ConstStmt"    ) \
    _(AtVarStmt,    "@VarStmt"      ) \
    _(AtLoopStmt,   "@LoopStmt"     ) \
    _(AtCallExpr,   "@CallExpr"     ) \
    _(AtDotExpr,    "@DotExpr"      ) \
    _(AtArrowExpr,  "@ArrowExpr"    ) \
    _(AtPosExpr,    "@PosExpr"      ) \
    _(AtNegExpr,    "@NegExpr"      ) \
    _(AtAddExpr,    "@AddExpr"      ) \
    _(AtSubExpr,    "@SubExpr"      ) \
    _(AtMulExpr,    "@MulExpr"      ) \
    _(AtDivExpr,    "@DivExpr"      ) \
    _(AtParenExpr,  "@ParenExpr"    ) \
    _(AtNameExpr,   "@NameExpr"     ) \
    _(AtInteger,    "@Integer"      )

namespace Whisper {
namespace NamePool {


enum class Id : uint32_t {
    INVALID = 0,
#define NAME_POOL_ID_(name, str) name,
    WHISPER_DEFN_NAME_POOL(NAME_POOL_ID_)
#undef NAME_POOL_ID_
    LIMIT
};

constexpr uint32_t IndexOfId(Id id) {
    return static_cast<uint32_t>(id) - 1;
}

constexpr uint32_t Size() {
    return IndexOfId(Id::LIMIT);
}


} // namespace NamePool
} // namespace Whisper


#endif // WHISPER__NAME_POOL_HPP
