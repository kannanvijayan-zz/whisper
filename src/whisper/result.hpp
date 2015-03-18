#ifndef WHISPER__RESULT_HPP
#define WHISPER__RESULT_HPP

#include "common.hpp"
#include "debug.hpp"
#include <new>

namespace Whisper {


template <typename V>
class Result
{
  private:
    static constexpr unsigned Size = sizeof(V);
    static constexpr unsigned Align = alignof(V);
    alignas(Align) char data_[Size];
    bool isValue_;

    V *valuePtr() {
        return reinterpret_cast<V *>(&data_[0]);
    }
    const V *valuePtr() const {
        return reinterpret_cast<const V *>(&data_[0]);
    }

    Result(const V &v)
      : isValue_(true)
    {
        new (valuePtr()) V(v);
    }
    Result(V &&v)
      : isValue_(true)
    {
        new (valuePtr()) V(std::move(v));
    }
    Result()
      : isValue_(false)
    {}

  public:
    Result(const Result<V> &other) = default;
    Result(Result<V> &&other)
      : isValue_(other.isValue_)
    {
        if (isValue_)
            new (valuePtr()) V(std::move(other.value()));
    }

    static Result<V> Value(const V &v) {
        return Result<V>(v);
    }
    static Result<V> Value(V &&v) {
        return Result<V>(std::move(v));
    }

    static Result<V> Error() {
        return Result<V>();
    }

    ~Result() {
        if (isValue())
            valuePtr()->~V();
    }

    bool isValue() const {
        return isValue_;
    }
    bool isError() const {
        return !isValue_;
    }

    const V &value() const {
        WH_ASSERT(isValue());
        return *(valuePtr());
    }
    V &value() {
        WH_ASSERT(isValue());
        return *(valuePtr());
    }
    void setValue(const V &val) {
        if (isValue()) {
            *(valuePtr()) = val;
        } else {
            new (valuePtr()) V(val);
            isValue_ = true;
        }
    }
    void setValue(V &&val) {
        if (isValue()) {
            *(valuePtr()) = std::move(val);
        } else {
            new (valuePtr()) V(std::move(val));
            isValue_ = true;
        }
    }

    void setError() {
        if (isValue())
            valuePtr()->~V();
        isValue_ = false;
    }

    Result<V> &operator =(const Result<V> &other) {
        if (other->isValue())
            setValue(other->value());
        else
            setError();
        return *this;
    }
    Result<V> &operator =(Result<V> &&other) {
        if (other->isValue())
            setValue(std::move(other->value()));
        else
            setError();
        return *this;
    }
};


} // namespace Whisper

#endif // WHISPER__RESULT_HPP
