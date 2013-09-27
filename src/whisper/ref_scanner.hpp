#ifndef WHISPER__REF_SCANNER_HPP
#define WHISPER__REF_SCANNER_HPP

#include "common.hpp"
#include "debug.hpp"

namespace Whisper {

enum RefKind
{
    INVALID = 0,
    HeapThing,
    LIMIT
};

//
// For the garbage collector to do its job, it needs a way of scanning
// the object graph (including proper objects on the managed heap, as well
// as native structures which may be referenced, as well as reference
// things in the managed heap).
//
// The RefScanner abstraction provides this capability.  RefScaner is
// a type-specialized class that is able to traverse values of that type.
//
// A RefScanner returns instances of Ref - a nested type specifiable by
// the refscanner itself.  Refs can be used to retreive and update
// a reference pointer.  Refs can also indicate the kind of thing
// they refer to.
//
// The required methods of a type-specialized RefScanner are:
//
//      RefScanner<T>(T &val);
//      bool hasMoreRefs();
//      Ref nextRef();
//
// The required methods of a Ref implementation are:
//
//      RefKind kind();
//      void *read();
//      void update(void *);
//
template <typename T>
class RefScanner
{
};


} // namespace Whisper

#endif // WHISPER__REF_SCANNER_HPP
