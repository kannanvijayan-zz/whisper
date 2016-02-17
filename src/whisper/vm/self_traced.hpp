#ifndef WHISPER__VM__SELF_TRACED_HPP
#define WHISPER__VM__SELF_TRACED_HPP

#include <sstream>
#include <cstdio>

#include "vm/predeclare.hpp"
#include "vm/array.hpp"
#include "vm/slist.hpp"

namespace Whisper {
namespace VM {

class BaseSelfTraced
{
  friend class TraceTraits<BaseSelfTraced>;
  protected:
    void* valuePtr_;

    BaseSelfTraced(void* valuePtr) : valuePtr_(valuePtr) {}

  public:
    template <typename Scanner>
    inline void scan(Scanner& scanner, void const* start, void const* end)
        const;

    template <typename Updater>
    inline void update(Updater& updater, void const* start, void const* end);
};

//
// A Self-Traced value is a traced value embedding another
// traced value.  It knows how to trace the embedded value
// using provided tracer functions.
//
template <typename T>
class SelfTraced : public BaseSelfTraced
{
  friend class TraceTraits<SelfTraced<T>>;
  public:
    class SelfScanner {
        virtual void operator ()(void const* addr, HeapThing* ptr) = 0;
    };
    class SelfUpdater {
        virtual void operator ()(void* addr, HeapThing* ptr) = 0;
    };

    typedef void (*ScanFuncPtr)(SelfScanner& scanner, T const& obj,
                                void const* start, void const* end);
    typedef void (*UpdateFuncPtr)(SelfUpdater& updater, T& obj,
                                  void const* start, void const* end);

  protected:
    ScanFuncPtr scanFunc_;
    UpdateFuncPtr updateFunc_;
    T value_;

  public:
    SelfTraced(T const& value, ScanFuncPtr scanFunc, UpdateFuncPtr updateFunc)
      : BaseSelfTraced(&value_),
        scanFunc_(scanFunc),
        updateFunc_(updateFunc),
        value_(value)
    {}

  private:
    template <typename Scanner>
    struct SelfScannerImpl : public SelfScanner
    {
        Scanner& scanner;
        SelfScannerImpl(Scanner& scanner) : scanner(scanner) {}
        virtual void operator ()(void const* addr, HeapThing* ptr) override {
            scanner(addr, ptr);
        }
    };
    template <typename Updater>
    struct SelfUpdaterImpl : public SelfUpdater
    {
        Updater& updater;
        SelfUpdaterImpl(Updater& updater) : updater(updater) {}
        virtual void operator ()(void* addr, HeapThing* ptr) override {
            updater(addr, ptr);
        }
    };

  public:
    template <typename Scanner>
    void scanImpl(Scanner& scanner, void const* start, void const* end) const
    {
        SelfScannerImpl<Scanner> scannerImpl(scanner);
        T const& obj = *reinterpret_cast<T const*>(valuePtr_);
        scanFunc_(scannerImpl, obj, start, end);
    }

    template <typename Updater>
    void updateImpl(Updater& updater, void const* start, void const* end)
    {
        SelfUpdaterImpl<Updater> updaterImpl(updater);
        T& obj = *reinterpret_cast<T*>(valuePtr_);
        updateFunc_(updaterImpl, obj, start, end);
    }
};

// Just use SelfTraced<HeapThing> as the surrogate type for
// specializing during scanning.  The actual update and tracing
// functions will be coordinated with the actual type of the
// object being scanned or updated.

template <typename Scanner>
inline void
BaseSelfTraced::scan(Scanner& scanner, void const* start, void const* end)
const
{
    SelfTraced<HeapThing> const* specializedThis =
        reinterpret_cast<SelfTraced<HeapThing> const*>(this);
    return specializedThis->scanImpl(scanner, start, end);
}

template <typename Updater>
inline void
BaseSelfTraced::update(Updater& updater, void const* start, void const* end)
{
    SelfTraced<HeapThing>* specializedThis =
        reinterpret_cast<SelfTraced<HeapThing>*>(this);
    return specializedThis->updateImpl(updater, start, end);
}


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
    HeapTraits() = delete;
    static constexpr bool Specialized = true;
    static const HeapFormat ArrayFormat = HeapFormat::BaseSelfTraced;
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
        selfTraced.scan(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::BaseSelfTraced& selfTraced,
                       void const* start, void const* end)
    {
        selfTraced.update(updater, start, end);
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
        selfTraced.scanImpl(scanner, start, end);
    }

    template <typename Updater>
    static void Update(Updater& updater, VM::SelfTraced<T>& selfTraced,
                       void const* start, void const* end)
    {
        selfTraced.updateImpl(updater, start, end);
    }
};


} // namespace Whisper


#endif // WHISPER__VM__SELF_TRACED_HPP
