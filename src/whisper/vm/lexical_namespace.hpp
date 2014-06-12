#ifndef WHISPER__VM__LEXICAL_NAMESPACE_HPP
#define WHISPER__VM__LEXICAL_NAMESPACE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"

#include "vm/lexical_namespace_prelude.hpp"

namespace Whisper {
namespace VM {


class LexicalNamespace
{
    friend struct GC::TraceTraits<LexicalNamespace>;

  public:
    enum Access
    {
        Public,
        Private
    };

    class Entry
    {
      friend struct GC::TraceTraits<Entry>;
      private:
        HeapField<String *> name_;
        HeapField<GC::AllocThing *> defn_;
        uint32_t flags_;

      public:
        Entry(String *name) : name_(name), defn_(nullptr) {}

        String *name() const {
            return name_;
        }
    };
    typedef Array<Entry> BindingArray;

  private:
    HeapField<LexicalNamespace *> parent_;
    HeapField<BindingArray *> bindings_;

  public:
    LexicalNamespace(LexicalNamespace *parent, BindingArray *bindings);

    LexicalNamespace *parent() const {
        return parent_;
    }

    BindingArray *bindings() const {
        return bindings_;
    }
};


} // namespace VM
} // namespace Whisper

#include "vm/lexical_namespace_specializations.hpp"

#endif // WHISPER__VM__LEXICAL_NAMESPACE_HPP
