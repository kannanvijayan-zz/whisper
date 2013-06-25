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
    /* Punctuators. */                      \
    _(OpenBrace)                            \
    _(CloseBrace)                           \
    _(OpenParen)                            \
    _(CloseParen)                           \
    _(OpenBracket)                          \
    _(CloseBracket)                         \
    _(Dot)                                  \
    _(Semicolon)                            \
    _(Comma)                                \
    _(LessThan)                             \
    _(GreaterThan)                          \
    _(LessEqual)                            \
    _(GreaterEqual)                         \
    _(Equal)                                \
    _(NotEqual)                             \
    _(StrictEqual)                          \
    _(StrictNotEqual)                       \
    _(Plus)                                 \
    _(Minus)                                \
    _(Star)                                 \
    _(Percent)                              \
    _(PlusPlus)                             \
    _(MinusMinus)                           \
    _(ShiftLeft)                            \
    _(ShiftRight)                           \
    _(ShiftUnsignedRight)                   \
    _(BitAnd)                               \
    _(BitOr)                                \
    _(BitXor)                               \
    _(LogicalNot)                           \
    _(Tilde)                                \
    _(LogicalAnd)                           \
    _(LogicalOr)                            \
    _(Question)                             \
    _(Colon)                                \
    _(Assign)                               \
    _(PlusAssign)                           \
    _(MinusAssign)                          \
    _(StarAssign)                           \
    _(PercentAssign)                        \
    _(ShiftLeftAssign)                      \
    _(ShiftRightAssign)                     \
    _(ShiftUnsignedRightAssign)             \
    _(BitAndAssign)                         \
    _(BitOrAssign)                          \
    _(BitXorAssign)                         \
    _(Divide)                               \
    _(DivideAssign)                         \
    \
    /* Literals. */                         \
    _(RegularExpressionLiteral)             \
    _(NumericLiteral)                       \
    _(StringLiteral)                        \
    \
    /* IdentifierNames. */                  \
    _(IdentifierName)                       \
    \
    /* Keywords and Literals. */            \
    _(NullLiteral)                          \
    _(FalseLiteral)                         \
    _(TrueLiteral)                          \
    _(BreakKeyword)                         \
    _(CaseKeyword)                          \
    _(CatchKeyword)                         \
    _(ContinueKeyword)                      \
    _(DebuggerKeyword)                      \
    _(DefaultKeyword)                       \
    _(DeleteKeyword)                        \
    _(DoKeyword)                            \
    _(ElseKeyword)                          \
    _(FinallyKeyword)                       \
    _(ForKeyword)                           \
    _(FunctionKeyword)                      \
    _(IfKeyword)                            \
    _(InKeyword)                            \
    _(InstanceOfKeyword)                    \
    _(NewKeyword)                           \
    _(ReturnKeyword)                        \
    _(SwitchKeyword)                        \
    _(ThisKeyword)                          \
    _(ThrowKeyword)                         \
    _(TryKeyword)                           \
    _(TypeOfKeyword)                        \
    _(VarKeyword)                           \
    _(VoidKeyword)                          \
    _(WhileKeyword)                         \
    _(WithKeyword)                          \
    \
    /* FutureReserved Words. */             \
    _(ClassFutureWord)                      \
    _(ConstFutureWord)                      \
    _(EnumFutureWord)                       \
    _(ExportFutureWord)                     \
    _(ExtendsFutureWord)                    \
    _(ImportFutureWord)                     \
    _(SuperFutureWord)                      \
    \
    /* Strict FutureReserved Words. */      \
    _(ImplementsStrictFutureWord)           \
    _(InterfaceStrictFutureWord)            \
    _(LetStrictFutureWord)                  \
    _(PackageStrictFutureWord)              \
    _(PrivateStrictFutureWord)              \
    _(ProtectedStrictFutureWord)            \
    _(PublicStrictFutureWord)               \
    _(StaticStrictFutureWord)               \
    _(YieldStrictFutureWord)


#define WHISPER_DEFN_KEYWORDS(_) \
    /* TokenId                     String           Priority            */ \
    \
    /* Keywords and Literals. */                                            \
    _(NullLiteral,                 "null",          (0+0+0+0+0+0+0+0+0)   ) \
    _(FalseLiteral,                "false",         (0+0+0+0+0+0+0+0+0)   ) \
    _(TrueLiteral,                 "true",          (0+0+0+0+0+0+0+0+0)   ) \
    _(BreakKeyword,                "break",         (2+0+0+0+0)           ) \
    _(CaseKeyword,                 "case",          (2+0+0+0+0)           ) \
    _(CatchKeyword,                "catch",         (3+0+0)               ) \
    _(ContinueKeyword,             "continue",      (2+0+0+0+0)           ) \
    _(DebuggerKeyword,             "debugger",      (4)                   ) \
    _(DefaultKeyword,              "default",       (2+0+0+0+0)           ) \
    _(DeleteKeyword,               "delete",        (3+0+0)               ) \
    _(DoKeyword,                   "do",            (3+0+0)               ) \
    _(ElseKeyword,                 "else",          (1+0+0+0+0+0+0)       ) \
    _(FinallyKeyword,              "finally",       (3+0+0)               ) \
    _(ForKeyword,                  "for",           (1+0+0+0+0+0+0)       ) \
    _(FunctionKeyword,             "function",      (1+0+0+0+0+0+0)       ) \
    _(IfKeyword,                   "if",            (1+0+0+0+0+0+0)       ) \
    _(InKeyword,                   "in",            (1+0+0+0+0+0+0)       ) \
    _(InstanceOfKeyword,           "instanceof",    (2+0+0+0+0)           ) \
    _(NewKeyword,                  "new",           (1+0+0+0+0+0+0)       ) \
    _(ReturnKeyword,               "return",        (2+0+0+0+0)           ) \
    _(SwitchKeyword,               "switch",        (2+0+0+0+0)           ) \
    _(ThisKeyword,                 "this",          (0+0+0+0+0+0+0+0+0)   ) \
    _(ThrowKeyword,                "throw",         (3+0+0)               ) \
    _(TryKeyword,                  "try",           (3+0+0)               ) \
    _(TypeOfKeyword,               "typeof",        (2+0+0+0+0)           ) \
    _(VarKeyword,                  "var",           (0+0+0+0+0+0+0+0+0)   ) \
    _(VoidKeyword,                 "void",          (4)                   ) \
    _(WhileKeyword,                "while",         (2+0+0+0+0)           ) \
    _(WithKeyword,                 "with",          (4)                   ) \
    \
    /* FutureReserved Words. */                                             \
    _(ClassFutureWord,             "class",         5                     ) \
    _(ConstFutureWord,             "const",         5                     ) \
    _(EnumFutureWord,              "enum",          5                     ) \
    _(ExportFutureWord,            "export",        5                     ) \
    _(ExtendsFutureWord,           "extends",       5                     ) \
    _(ImportFutureWord,            "import",        5                     ) \
    _(SuperFutureWord,             "super",         5                     ) \
    \
    /* Strict FutureReserved Words. */                                      \
    _(ImplementsStrictFutureWord,  "implements",    6                     ) \
    _(InterfaceStrictFutureWord,   "interface",     6                     ) \
    _(LetStrictFutureWord,         "let",           6                     ) \
    _(PackageStrictFutureWord,     "package",       6                     ) \
    _(PrivateStrictFutureWord,     "private",       6                     ) \
    _(ProtectedStrictFutureWord,   "protected",     6                     ) \
    _(PublicStrictFutureWord,      "public",        6                     ) \
    _(StaticStrictFutureWord,      "static",        6                     ) \
    _(YieldStrictFutureWord,       "yield",         6                     )

#define WHISPER_KEYWORD_FUTURE_RESERVED_PRIO            (5)
#define WHISPER_KEYWORD_STRICT_FUTURE_RESERVED_PRIO     (6)


#endif // WHISPER__PARSER__TOKEN_DEFN_HPP
