
#include "parser/tokenizer.hpp"


namespace Whisper {


const char *
Token::TypeString(Type type)
{
    switch (type) {
#define DEF_CASE_(tok) \
      case tok: \
        return #tok;
      WHISPER_DEFN_TOKENS(DEF_CASE_)
#undef DEF_CASE_
      case INVALID:
        return "INVALID";
      case LIMIT:
        return "INVALID_LIMIT";
    }
    return "INVALID_UNKNOWN";
}


} // namespace Whisper
