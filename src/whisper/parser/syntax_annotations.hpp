#ifndef WHISPER__PARSER__SYNTAX_ANNOTATIONS_HPP
#define WHISPER__PARSER__SYNTAX_ANNOTATIONS_HPP

#include <list>
#include "allocators.hpp"
#include "parser/tokenizer.hpp"
#include "parser/syntax_defn.hpp"
#include "parser/syntax_tree.hpp"

namespace Whisper {
namespace AST {

class BaseNode;

//
// The syntax annotator applies annotations to syntax trees.
// It uses a visitor pattern to implement the annotation process.
//

class SyntaxAnnotatorError
{
  friend class SyntaxAnnotator;

  private:
    inline SyntaxAnnotatorError() {}
};


class SyntaxAnnotator
{
  private:
    STLBumpAllocator<uint8_t> allocator_;
    BaseNode *root_;
    const CodeSource &source_;

    const char *error_ = nullptr;

  public:
    SyntaxAnnotator(STLBumpAllocator<uint8_t> allocator,
                    BaseNode *root, const CodeSource &source)
      : allocator_(allocator), root_(root), source_(source)
    {}

    bool hasError() const {
        return error_ != nullptr;
    }

    const char *error() const {
        return error_;
    }

    bool annotate();

  private:
    void annotate(BaseNode *node, BaseNode *parent);

#define DEF_ANNOT_(name) \
    void annotate##name(name##Node *node, BaseNode *parent);
      WHISPER_DEFN_SYNTAX_NODES(DEF_ANNOT_);
#undef DEF_ANNOT_

    void emitError(const char *msg);

    template <typename T>
    inline STLBumpAllocator<T> allocatorFor() const {
        return STLBumpAllocator<T>(allocator_);
    }

    template <typename T, typename... ARGS>
    inline T *make(ARGS... args)
    {
        return new (allocatorFor<T>().allocate(1)) T(
            std::forward<ARGS>(args)...);
    }
};

//
// Annotates a NumericLiteralNode with its value.
//
class NumericLiteralAnnotation
{
  friend class SyntaxAnnotator;
  private:
    bool isInt_;
    union {
        int32_t intVal_;
        double doubleVal_;
    };

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

  public:
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


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_ANNOTATIONS_HPP
