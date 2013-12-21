#ifndef WHISPER__PARSER__TOKEN_DEFN_HPP
#define WHISPER__PARSER__TOKEN_DEFN_HPP


// Macro iterating over all token kinds.
#define WHISPER_DEFN_TOKENS(_) \
    /* Special. */                          \
    _(End)                                  \
    _(Error)                                \
    _(LineTerminatorSequence)               \
    _(Whitespace)                           \
    _(SingleLineComment)                    \
    _(MultiLineComment)                     \
    \
    /* General punctuation. */              \
    _(Semicolon)           /* ; */          \
    _(Comma)               /* , */          \
    \
    /* Qualified accessors. */              \
    _(ColonColon)          /* :: */         \
    _(Dot)                 /* . */          \
    \
    /* Nested braces. */                    \
    _(OpenParen)           /* ( */          \
    _(CloseParen)          /* ) */          \
    _(OpenBrace)           /* { */          \
    _(CloseBrace)          /* } */          \
    \
    /* Assignment operators. */             \
    _(Assign)              /* = */          \
    \
    /* Literals. */                         \
    _(IntegerLiteral)                       \
    \
    /* Identifiers. */                      \
    _(Identifier)                           \
    \
    /* Keywords. */                         \
    _(ModuleKeyword)                        \
    _(ImportKeyword)                        \
    _(AsKeyword)                            \
    _(PublicKeyword)                        \
    _(PrivateKeyword)                       \
    _(FuncKeyword)                          \
    _(IntKeyword)                           \
    _(ReturnKeyword)

#define WHISPER_FIRST_KEYWORD_TOKEN             ModuleKeyword
#define WHISPER_LAST_KEYWORD_TOKEN              ReturnKeyword

#define WHISPER_DEFN_KEYWORDS(_) \
    /* TokenId                     String           Priority            */  \
    \
    /* Keywords and Literals. */                                            \
    _(ModuleKeyword,               "module",        (6+0+0+0+0+0+0+0+0)   ) \
    _(ImportKeyword,               "import",        (6+0+0+0+0+0+0+0+0)   ) \
    _(AsKeyword,                   "as",            (5+0+0+0+0+0+0+0+0)   ) \
    _(PublicKeyword,               "public",        (5+0+0+0+0+0+0+0+0)   ) \
    _(PrivateKeyword,              "private",       (5+0+0+0+0+0+0+0+0)   ) \
    _(FuncKeyword,                 "func",          (4+0+0+0+0+0+0+0+0)   ) \
    _(IntKeyword,                  "int",           (3+0+0+0+0+0+0+0+0)   ) \
    _(ReturnKeyword,               "return",        (3+0+0+0+0+0+0+0+0)   )


#define WHISPER_DEFN_QUICK_TOKENS(_) \
    /* TokenId                      Char                                */  \
    \
    _(Semicolon,                    ';'                                  )  \
    _(Comma,                        ','                                  )  \
    _(Dot,                          '.'                                  )  \
    _(OpenParen,                    '('                                  )  \
    _(CloseParen,                   ')'                                  )  \
    _(OpenBrace,                    '{'                                  )  \
    _(CloseBrace,                   '}'                                  )  \
    _(Assign,                       '='                                  )

#endif // WHISPER__PARSER__TOKEN_DEFN_HPP
