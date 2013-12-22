#ifndef WHISPER__PARSER__SYNTAX_DEFN_HPP
#define WHISPER__PARSER__SYNTAX_DEFN_HPP


#define WHISPER_DEFN_SYNTAX_NODES(_) \
    /* Top level. */                        \
    _(File)                                 \
    \
    /* File declarations. */                \
    _(ModuleDecl)                           \
    _(ImportDecl)                           \
    _(FuncDecl)                             \
    _(Block)                                \
    \
    /* Statements. */                       \
    _(ReturnStmt)                           \
    _(EmptyStmt)                            \
    _(ExprStmt)                             \
    \
    /* Expressions. */                      \
    _(IdentifierExpr)                       \
    _(IntegerLiteralExpr)                   \
    \
    /* Typenames. */                        \
    _(IntType)


#endif // WHISPER__PARSER__SYNTAX_DEFN_HPP
