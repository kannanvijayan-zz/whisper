#ifndef WHISPER__VM__MODULE_HPP
#define WHISPER__VM__MODULE_HPP


#include "vm/core.hpp"
#include "vm/string.hpp"
#include "vm/array.hpp"
#include "vm/source_file.hpp"

#include <new>

namespace Whisper {
namespace VM {


//
// A Module contains all the information associated with a given
// module.  This includes a sorted array of symbols mapping to
// their respective definitions within the module, as well as
// the list of files which contain the contents of the module.
//
class Module
{
    friend struct GC::TraceTraits<Module>;

  public:
    typedef Array<SourceFile *> SourceFileArray;

    class Entry
    {
      friend struct GC::TraceTraits<Entry>;
      private:
        uint32_t sourceFile_;
        HeapField<String *> name_;

      public:
        Entry(uint32_t sourceFile, String *name)
          : name_(name)
        {}

        uint32_t sourceFile() const {
            return sourceFile_;
        }

        String *name() const {
            return name_;
        }
    };
    typedef Array<Entry> BindingArray;

  private:
    HeapField<SourceFileArray *> sourceFiles_;
    HeapField<BindingArray *> bindings_;

  public:
    Module(SourceFileArray *sourceFiles, BindingArray *bindings)
      : sourceFiles_(sourceFiles),
        bindings_(bindings)
    {
        WH_ASSERT(sourceFiles != nullptr);
        WH_ASSERT(bindings != nullptr);
    }

    SourceFileArray *sourceFiles() const {
        return sourceFiles_;
    }

    inline uint32_t numBindings() const;

    BindingArray *bindings() const {
        return bindings_;
    }

    inline Entry const &rawBindingEntry(uint32_t idx) const;
};


} // namespace VM
} // namespace Whisper


#include "vm/module_specializations.hpp"


namespace Whisper {
namespace VM {


inline
uint32_t
Module::numBindings() const
{
    return bindings_->length();
}

inline
Module::Entry const &
Module::rawBindingEntry(uint32_t idx) const
{
    return bindings_->getRaw(idx);
}


} // namespace VM
} // namespace Whisper


#endif // WHISPER__VM__MODULE_HPP
