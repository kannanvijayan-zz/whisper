#ifndef WHISPER__PARSER__SYNTAX_DEFN_HPP
#define WHISPER__PARSER__SYNTAX_DEFN_HPP


#define WHISPER_DEFN_SYNTAX_NODES(_) \
    /* Top level. */                        \
    _(File)              /* done */         \
    \
    /* Statements. */                       \
    _(EmptyStmt)         /* done */         \
    _(ExprStmt)          /* done */         \
    _(ReturnStmt)        /* done */         \
    _(IfStmt)                               \
    _(DefStmt)           /* done */         \
    _(ConstStmt)                            \
    _(VarStmt)           /* done */         \
    _(LoopStmt)                             \
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
    _(IntegerExpr)       /* done */


#endif // WHISPER__PARSER__SYNTAX_DEFN_HPP
