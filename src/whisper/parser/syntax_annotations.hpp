#ifndef WHISPER__PARSER__SYNTAX_ANNOTATIONS_HPP
#define WHISPER__PARSER__SYNTAX_ANNOTATIONS_HPP

#include <list>
#include "parser/tokenizer.hpp"
#include "parser/syntax_defn.hpp"

namespace Whisper {
namespace AST {

//
// Annotates a NumericLiteralNode with its value.
//
class NumericLiteralAnnotation
{
  private:
    bool isInt_;
    union {
        int32_t intVal_;
        double doubleVal_;
    };

  public:
    NumericLiteralAnnotation()
    {}

    NumericLiteralAnnotation(int32_t ival)
      : isInt_(true)
    {
        intVal_ = ival;
    }

    NumericLiteralAnnotation(double dval)
      : isInt_(false)
    {
        doubleVal_ = dval;
    }

    bool isInt() const {
        return isInt_;
    }

    bool isDouble() const {
        return !isInt_;
    }

    int32_t intVal() const {
        WH_ASSERT(isInt());
        return intVal_;
    }

    double doubleVal() const {
        WH_ASSERT(isDouble());
        return doubleVal_;
    }
};

//
// The syntax annotator applies annotations to syntax trees.
// It uses a visitor pattern to implement the annotation process.
//

class SyntaxAnnotatorError
{
  template <typename Handler>
  friend class SyntaxWalker;

  private:
    inline SyntaxWalkerError() {}
};


class SyntaxAnnotator
{
  private:
    BaseNode *root_;
    Handler *handler_;

    const char *error_ = nullptr;

  public:
    SyntaxAnnotator(BaseNode *root, Handler *handler)
      : root_(root), handler_(handler)
    {}

    bool hasError() const {
        return error_ != nullptr;
    }

    const char *error() const {
        return error_;
    }

    bool annotate();

  private:
    void annot(BaseNode *node, BaseNode *parent);

    void annotProgram(ProgramNode *node, BaseNode *parent);
    void annotFunctionDeclaration(FunctionDeclarationNode *node,
                                 BaseNode *parent);
    void annotThis(ThisNode *node, BaseNode *parent);
};


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_ANNOTATIONS_HPP
