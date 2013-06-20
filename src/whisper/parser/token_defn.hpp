#ifndef WHISPER__PARSER__TOKEN_DEFN_HPP
#define WHISPER__PARSER__TOKEN_DEFN_HPP


// Macro iterating over all token kinds.
#define WHISPER_DEFN_TOKENS(_) \
    /* Special. */              \
    _(End)                      \
    _(Error)                    \
    _(LineTerminatorSequence)   \
    _(Whitespace)               \
    _(SingleLineComment)        \
    _(MultiLineComment)         \
    \
    /* Punctuators. */          \
    _(OpenBrace)                \
    _(CloseBrace)               \
    _(OpenParen)                \
    _(CloseParen)               \
    _(OpenBracket)              \
    _(CloseBracket)             \
    _(Dot)                      \
    _(Semicolon)                \
    _(Comma)                    \
    _(LessThan)                 \
    _(GreaterThan)              \
    _(LessEqual)                \
    _(GreaterEqual)             \
    _(Equal)                    \
    _(NotEqual)                 \
    _(StrictEqual)              \
    _(StrictNotEqual)           \
    _(Plus)                     \
    _(Minus)                    \
    _(Star)                     \
    _(Percent)                  \
    _(PlusPlus)                 \
    _(MinusMinus)               \
    _(ShiftLeft)                \
    _(ShiftRight)               \
    _(ShiftUnsignedRight)       \
    _(BitAnd)                   \
    _(BitOr)                    \
    _(BitXor)                   \
    _(LogicalNot)               \
    _(Tilde)                    \
    _(LogicalAnd)               \
    _(LogicalOr)                \
    _(Question)                 \
    _(Colon)                    \
    _(Assign)                   \
    _(PlusAssign)               \
    _(MinusAssign)              \
    _(StarAssign)               \
    _(PercentAssign)            \
    _(ShiftLeftAssign)          \
    _(ShiftRightAssign)         \
    _(ShiftUnsignedRightAssign) \
    _(BitAndAssign)             \
    _(BitOrAssign)              \
    _(BitXorAssign)             \
    _(Divide)                   \
    _(DivideAssign)             \
    \
    /* Literals. */             \
    _(RegularExpressionLiteral) \
    _(NumericLiteral)           \
    \
    /* Idents. */               \
    _(IdentifierName)


#endif // WHISPER__PARSER__TOKEN_DEFN_HPP
