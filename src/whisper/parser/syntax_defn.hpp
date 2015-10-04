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
    _(PosExpr)           /* done */         \
    _(NegExpr)           /* done */         \
    _(AddExpr)           /* done */         \
    _(SubExpr)           /* done */         \
    _(MulExpr)                              \
    _(DivExpr)                              \
    _(ParenExpr)         /* done */         \
    _(NameExpr)          /* done */         \
    _(IntegerExpr)       /* done */


#endif // WHISPER__PARSER__SYNTAX_DEFN_HPP
