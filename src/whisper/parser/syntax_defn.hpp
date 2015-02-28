#ifndef WHISPER__PARSER__SYNTAX_DEFN_HPP
#define WHISPER__PARSER__SYNTAX_DEFN_HPP


#define WHISPER_DEFN_SYNTAX_NODES(_) \
    /* Top level. */                        \
    _(File)                                 \
    \
    /* Statements. */                       \
    _(EmptyStmt)                            \
    _(ExprStmt)                             \
    \
    /* Expressions. */                      \
    _(CallExpr)                             \
    _(DotExpr)                              \
    _(ArrowExpr)                            \
    _(PosExpr)                              \
    _(NegExpr)                              \
    _(AddExpr)                              \
    _(SubExpr)                              \
    _(MulExpr)                              \
    _(DivExpr)                              \
    _(ParenExpr)                            \
    _(NameExpr)                             \
    _(IntegerExpr)


#endif // WHISPER__PARSER__SYNTAX_DEFN_HPP
