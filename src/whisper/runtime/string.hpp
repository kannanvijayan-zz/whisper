#ifndef WHISPER__RUNTIME__STRING_HPP
#define WHISPER__RUNTIME__STRING_HPP

#include <algorithm>

#include "common.hpp"
#include "debug.hpp"

#include "runtime/rooting.hpp"
#include "runtime/heap_thing.hpp"

namespace Whisper {
namespace Runtime {


class String : public HeapThing
{
  private:
    uint32_t charCount_;
    uint8_t data_[0];

  public:
    String(uint32_t size, uint32_t charCount, const uint8_t *data)
      : charCount_(charCount)
    {
        std::copy(data, data + size, data_);
    }
};


} // namespace Runtime
} // namespace Whisper

#endif // WHISPER__RUNTIME__STRING_HPP
