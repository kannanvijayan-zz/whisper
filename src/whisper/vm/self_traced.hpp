#ifndef WHISPER__VM__SELF_TRACED_HPP
#define WHISPER__VM__SELF_TRACED_HPP

#include <sstream>
#include <cstdio>

#include "vm/predeclare.hpp"
#include "vm/array.hpp"
#include "vm/slist.hpp"

namespace Whisper {
namespace VM {

//
// The SelfTraced type allows the use of values which can be traced,
// but whose types do not need to have corresponding entries in the
// StackFormat or HeapFormat enums.
//
// SelfTraced accomplishes this by using a single format enum,
// but lifting the tracing responsbility into virtual methods.
//
// This leads to more space usage for storing SelfTraced values,
// due to the overhead of having a vtable.
//

class BaseSelfTraced
{
  friend class TraceTraits<BaseSelfTraced>;
  protected:
    BaseSelfTraced() {}

  public:
    virtual void scan(AbstractScanner& scanner,
                      void const* start,
                      void const* end) const = 0;

    virtual void update(AbstractUpdater& updater,
                        void const* start,
                        void const* end) = 0;
};

//
// A Self-Traced a wrapper around a custom-traced
// value.  The wrapped value must define a trace()
// and update() method, with the following signatures:
//      scan(AbstractScanner* scanner, void const* start, void const* end);
//      update(AbstractUpdater* updater, void const* start, void const* end);
//
// SelfTraced frees the definer of a traced type
// from having to declare a HeapFormat for the
//
template <typename T>
class SelfTraced : public BaseSelfTraced
{
  static_assert(TraceTraits<T>::Specialized,
                "Wrapped type is not specialized for TraceTraits.");

  friend class TraceTraits<SelfTraced<T>>;
  protected:
    T value_;

  public:
    SelfTraced(T const& value)
      : BaseSelfTraced(),
        value_(value)
    {}

    SelfTraced(T&& value)
      : BaseSelfTraced(),
        value_(std::move(value))
    {}

  public:
    virtual void scan(AbstractScanner& scanner,
                      void const* start,
                      void const* end) const override
    {
        value_.scan(scanner, start, end);
    }

    virtual void update(AbstractUpdater& updater,
                        void const* start,
                        void const* end) override
    {
        value_.update(updater, start, end);
    }
};


} // namespace VM


//
// GC-Specializations.
//

// HeapTraits for arbitrary SelfTraced<T> types just maps them
// to the BaseSelfTraced format, which knows how to trace all
// of it's own subclassed specializations.
template <typename T>
struct HeapTraits<VM::SelfTraced<T>>
{
    static_assert(HeapTraits<T>::Specialized,
                  "Wrapped type is not designated for heap allocation.");
    HeapTraits() = delete;
    static constexpr bool Specialized = true;
    static constexpr HeapFormat Format = HeapFormat::BaseSelfTraced;
    static constexpr bool VarSized = false;
};

template <>
struct TraceTraits<VM::BaseSelfTraced>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::BaseSelfTraced const& selfTraced,
                     void const* start, void const* end)
    {
        GC::ScannerBoxFor<Scanner> scannerBox(scanner);
        selfTraced.scan(scannerBox, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::BaseSelfTraced& selfTraced,
                       void const* start, void const* end)
    {
        GC::UpdaterBoxFor<Updater> updaterBox(updater);
        selfTraced.update(updaterBox, start, end);
    }
};


template <typename T>
struct TraceTraits<VM::SelfTraced<T>>
{
    TraceTraits() = delete;

    static constexpr bool Specialized = true;
    static constexpr bool IsLeaf = false;

    template <typename Scanner>
    static void Scan(Scanner& scanner, VM::SelfTraced<T> const& selfTraced,
                     void const* start, void const* end)
    {
        GC::ScannerBoxFor<Scanner> scannerBox(scanner);
        selfTraced.scan(scannerBox, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SelfTraced<T>& selfTraced,
                       void const* start, void const* end)
    {
        GC::UpdaterBoxFor<Updater> updaterBox(updater);
        selfTraced.update(updaterBox, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__SELF_TRACED_HPP
