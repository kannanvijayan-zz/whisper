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
    _(ConstStmt)         /* done */         \
    _(VarStmt)           /* done */         \
    _(LoopStmt)                             \
    \
    /* Expressions. */                      \
    _(CallExpr)          /* done */         \
    _(DotExpr)           /* done */         \
    _(ArrowExpr)                            \
    _(PosExpr)                              \
    _(NegExpr)           /* done */         \
    _(AddExpr)                              \
    _(SubExpr)                              \
    _(MulExpr)                              \
    _(DivExpr)                              \
    _(ParenExpr)         /* done */         \
    _(NameExpr)          /* done */         \
    _(IntegerExpr)       /* done */


#endif // WHISPER__PARSER__SYNTAX_DEFN_HPP
