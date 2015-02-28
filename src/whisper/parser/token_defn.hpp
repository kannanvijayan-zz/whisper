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
    /* Property accessors. */               \
    _(Dot)                 /* .  */         \
    _(Arrow)               /* -> */         \
    \
    /* Nested braces. */                    \
    _(OpenParen)           /* ( */          \
    _(CloseParen)          /* ) */          \
    _(OpenBrace)           /* { */          \
    _(CloseBrace)          /* } */          \
    \
    /* Punctuation. */                      \
    _(Equal)               /* = */          \
    _(Plus)                /* + */          \
    _(Minus)               /* - */          \
    _(Star)                /* * */          \
    _(Slash)               /* / */          \
    \
    /* Literals. */                         \
    _(IntegerLiteral)                       \
    \
    /* Identifiers. */                      \
    _(Identifier)                           \
    \
    /* Keywords. */                         \
    _(DefKeyword)                           \
    _(IfKeyword)                            \
    _(ElseKeyword)                          \
    _(ElsifKeyword)                         \
    _(ReturnKeyword)

#define WHISPER_FIRST_KEYWORD_TOKEN             DefKeyword
#define WHISPER_LAST_KEYWORD_TOKEN              ReturnKeyword

#define WHISPER_DEFN_KEYWORDS(_) \
    /* TokenId                     String           Priority            */  \
    \
    /* Keywords and Literals. */                                            \
    _(DefKeyword,                  "def",           1                     ) \
    _(IfKeyword,                   "if",            1                     ) \
    _(ElseKeyword,                 "else",          1                     ) \
    _(ElsifKeyword,                "elsif",         1                     ) \
    _(ReturnKeyword,               "return",        1                     )


#define WHISPER_DEFN_QUICK_TOKENS(_) \
    /* TokenId                      Char                                */  \
    \
    _(Semicolon,                    ';'                                  )  \
    _(Comma,                        ','                                  )  \
    _(OpenParen,                    '('                                  )  \
    _(CloseParen,                   ')'                                  )  \
    _(OpenBrace,                    '{'                                  )  \
    _(CloseBrace,                   '}'                                  )

#endif // WHISPER__PARSER__TOKEN_DEFN_HPP
