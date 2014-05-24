#ifndef WHISPER__COMPACT_BUFFER_HPP
#define WHISPER__COMPACT_BUFFER_HPP

#include "common.hpp"
#include "helpers.hpp"
#include "debug.hpp"

#include <limits>
#include <type_traits>

namespace Whisper {


//
// Compact Buffers are helper classes used to read and write
// various encoded values from a byte buffer.
//

template <bool Checked>
class PackbufReader
{
  private:
    const uint8_t * const data_;
    const uint8_t * const end_;

    template <typename T>
    using ResultType = typename std::conditional<Checked, Maybe<T>, T>::type;

  public:
    PackbufReader(const uint8_t *data, const uint8_t *end)
      : data_(data), end_(end)
    {
        WH_ASSERT(data_ <= end_);
        WH_ASSERT((end_ - data_) <= ptrdiff_t(UINT32_MAX));
    }

    const uint8_t *data() const {
        return data_;
    }
    const uint8_t *end() const {
        return end_;
    }

    class Cursor
    {
      friend class PackbufReader;
      private:
        const PackbufReader &reader_;
        const uint8_t * cur_;
        const uint8_t * const end_;

        ResultType<uint32_t> nextByte() {
            WH_ASSERT_IF(!Checked, canRead(1));
            if (Checked && !canRead(1))
                return ResultType<uint32_t>();

            return ResultType<uint32_t>(*cur_++);
        }

      public:
        Cursor() : reader_(nullptr), cur_(nullptr), end_(nullptr) {}
        Cursor(const PackbufReader<Checked> &reader)
          : reader_(reader), cur_(reader_.data()), end_(reader_.end())
        {}

        bool isValid() const {
            return reader_ != nullptr;
        }

        bool canRead(uint32_t count) const {
            return uint32_t(end_ - cur_) >= count;
        }

        ResultType<uint8_t> read8() {
            return nextByte();
        }
    };
};
typedef PackbufReader<true> CheckedPackbufReader;
typedef PackbufReader<false> UncheckedPackbufReader;


} // namespace Whisper

#endif // WHISPER__COMPACT_BUFFER_HPP
