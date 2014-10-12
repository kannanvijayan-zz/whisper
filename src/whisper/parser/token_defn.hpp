#ifndef WHISPER__PARSER__TOKEN_DEFN_HPP
#define WHISPER__PARSER__TOKEN_DEFN_HPP


// Macro iterating over all token kinds.
#define WHISPER_DEFN_TOKENS(_) \
    /* Special */                           \
    _(End)                                  \
    _(Error)                                \
    _(Newline)                              \
    _(Whitespace)                           \
    _(Comment)                              \
    _(Backslash)                            \
    \
    /* Operators */                         \
    _(Plus)                                 \
    _(Minus)                                \
    _(Star)                                 \
    _(StarStar)                             \
    _(Slash)                                \
    _(SlashSlash)                           \
    _(Percent)                              \
    _(ShiftLeft)                            \
    _(ShiftRight)                           \
    _(Ampersand)                            \
    _(Pipe)                                 \
    _(Carat)                                \
    _(Tilde)                                \
    _(Less)                                 \
    _(Greater)                              \
    _(LessEqual)                            \
    _(GreaterEqual)                         \
    _(EqualEqual)                           \
    _(BangEqual)                            \
    /* Delimiters */                        \
    _(OpenParen)                            \
    _(CloseParen)                           \
    _(OpenBracket)                          \
    _(CloseBracket)                         \
    _(OpenCurly)                            \
    _(CloseCurly)                           \
    _(Comma)                                \
    _(Colon)                                \
    _(Dot)                                  \
    _(Semicolon)                            \
    _(At)                                   \
    _(Equal)                                \
    _(RightArrow)                           \
    _(PlusEqual)                            \
    _(MinusEqual)                           \
    _(StarEqual)                            \
    _(SlashEqual)                           \
    _(SlashSlashEqual)                      \
    _(PercentEqual)                         \
    _(AmpersandEqual)                       \
    _(PipeEqual)                            \
    _(CaratEqual)                           \
    _(ShiftRightEqual)                      \
    _(ShiftLeftEqual)                       \
    _(StarStarEqual)                        \
    /* Literals */                          \
    _(StringLiteral)                        \
    _(IntegerLiteral)                       \
    _(FloatLiteral)                         \
    _(ImaginaryIntegerLiteral)              \
    _(ImaginaryFloatLiteral)                \
    \
    /* Identifiers */                       \
    _(Identifier)                           \
    \
    /* Keywords */                          \
    _(FalseKeyword)                         \
    _(TrueKeyword)                          \
    _(NoneKeyword)                          \
    _(AndKeyword)                           \
    _(AsKeyword)                            \
    _(AssertKeyword)                        \
    _(BreakKeyword)                         \
    _(ClassKeyword)                         \
    _(ContinueKeyword)                      \
    _(DefKeyword)                           \
    _(DelKeyword)                           \
    _(ElifKeyword)                          \
    _(ElseKeyword)                          \
    _(ExceptKeyword)                        \
    _(FinallyKeyword)                       \
    _(ForKeyword)                           \
    _(FromKeyword)                          \
    _(GlobalKeyword)                        \
    _(IfKeyword)                            \
    _(ImportKeyword)                        \
    _(InKeyword)                            \
    _(IsKeyword)                            \
    _(LambdaKeyword)                        \
    _(NonlocalKeyword)                      \
    _(NotKeyword)                           \
    _(OrKeyword)                            \
    _(PassKeyword)                          \
    _(RaiseKeyword)                         \
    _(ReturnKeyword)                        \
    _(TryKeyword)                           \
    _(WhileKeyword)                         \
    _(WithKeyword)                          \
    _(YieldKeyword)

#define WHISPER_FIRST_KEYWORD_TOKEN             FalseKeyword
#define WHISPER_LAST_KEYWORD_TOKEN              YieldKeyword

#define WHISPER_DEFN_KEYWORDS(_) \
    /* TokenId                     String           Priority       */ \
    _(FalseKeyword,                "False",         (1+0)           ) \
    _(TrueKeyword,                 "True",          (1+0)           ) \
    _(NoneKeyword,                 "None",          (1+0)           ) \
    _(AndKeyword,                  "and",           (1+0)           ) \
    _(AsKeyword,                   "as",            (2+0+0)         ) \
    _(AssertKeyword,               "assert",        (4+0+0+0)       ) \
    _(BreakKeyword,                "break",         (1+0)           ) \
    _(ClassKeyword,                "class",         (2+0+0)         ) \
    _(ContinueKeyword,             "continue",      (2+0+0)         ) \
    _(DefKeyword,                  "def",           (1+0)           ) \
    _(DelKeyword,                  "del",           (2+0+0)         ) \
    _(ElifKeyword,                 "elif",          (1+0)           ) \
    _(ElseKeyword,                 "else",          (1+0)           ) \
    _(ExceptKeyword,               "except",        (2+0+0)         ) \
    _(FinallyKeyword,              "finally",       (3+0+0+0)       ) \
    _(ForKeyword,                  "for",           (1+0)           ) \
    _(FromKeyword,                 "from",          (3+0+0+0)       ) \
    _(GlobalKeyword,               "global",        (2+0+0)         ) \
    _(IfKeyword,                   "if",            (1+0)           ) \
    _(ImportKeyword,               "import",        (3+0+0+0)       ) \
    _(InKeyword,                   "in",            (1+0)           ) \
    _(IsKeyword,                   "is",            (2+0+0)         ) \
    _(LambdaKeyword,               "lambda",        (2+0+0)         ) \
    _(NonlocalKeyword,             "nonlocal",      (3+0+0+0)       ) \
    _(NotKeyword,                  "not",           (1+0)           ) \
    _(OrKeyword,                   "or",            (1+0)           ) \
    _(PassKeyword,                 "pass",          (2+0+0)         ) \
    _(RaiseKeyword,                "raise",         (3+0+0+0)       ) \
    _(ReturnKeyword,               "return",        (1+0)           ) \
    _(TryKeyword,                  "try",           (2+0+0)         ) \
    _(WhileKeyword,                "while",         (1+0)           ) \
    _(WithKeyword,                 "with",          (2+0+0)         ) \
    _(YieldKeyword,                "yield",         (3+0+0+0)       )

#define WHISPER_DEFN_QUICK_TOKENS(_) \
    /* TokenId                      Char   */ \
    _(OpenParen,                    '('     ) \
    _(CloseParen,                   ')'     ) \
    _(OpenBracket,                  '['     ) \
    _(CloseBracket,                 ']'     ) \
    _(OpenCurly,                    '{'     ) \
    _(CloseCurly,                   '}'     ) \
    _(Comma,                        ','     ) \
    _(Colon,                        ':'     ) \
    _(Semicolon,                    ';'     ) \
    _(At,                           '@'     )

#endif // WHISPER__PARSER__TOKEN_DEFN_HPP
