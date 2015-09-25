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
    BaseNode* root_;
    CodeSource const& source_;

    char const* error_ = nullptr;

  public:
    SyntaxAnnotator(STLBumpAllocator<uint8_t> allocator,
                    BaseNode* root, CodeSource const& source)
      : allocator_(allocator), root_(root), source_(source)
    {}

    bool hasError() const {
        return error_ != nullptr;
    }

    char const* error() const {
        return error_;
    }

    bool annotate();

  private:
    void annotate(BaseNode* node, BaseNode* parent);

#define DEF_ANNOT_(name) \
    void annotate##name(name##Node* node, BaseNode* parent);
      WHISPER_DEFN_SYNTAX_NODES(DEF_ANNOT_);
#undef DEF_ANNOT_

    void emitError(char const* msg);

    template <typename T>
    inline STLBumpAllocator<T> allocatorFor() const {
        return STLBumpAllocator<T>(allocator_);
    }

    template <typename T, typename... ARGS>
    inline T* make(ARGS... args)
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
    bool isInt32_;
    union {
        int32_t int32Val_;
        double doubleVal_;
    };

    NumericLiteralAnnotation(int32_t ival)
      : isInt32_(true)
    {
        int32Val_ = ival;
    }

    NumericLiteralAnnotation(double dval)
      : isInt32_(ToInt32(dval) == dval)
    {
        if (isInt32_)
            int32Val_ = ToInt32(dval);
        else
            doubleVal_ = dval;
    }

  public:
    bool isInt32() const {
        return isInt32_;
    }

    bool isDouble() const {
        return !isInt32_;
    }

    int32_t int32Value() const {
        WH_ASSERT(isInt32());
        return int32Val_;
    }

    double doubleValue() const {
        WH_ASSERT(isDouble());
        return doubleVal_;
    }
};


} // namespace AST
} // namespace Whisper


#endif // WHISPER__PARSER__SYNTAX_ANNOTATIONS_HPP
