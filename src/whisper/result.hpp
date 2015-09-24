#ifndef WHISPER__RESULT_HPP
#define WHISPER__RESULT_HPP

#include "common.hpp"
#include "debug.hpp"
#include <new>

namespace Whisper {


class ErrorT_
{
  public:
    ErrorT_() {}
    ~ErrorT_() {}
};

class OkT_
{
  public:
    OkT_() {}
    ~OkT_() {}
};

template <typename T>
class OkValT_
{
    template <typename V>
    friend class Result;
  private:
    T val_;

  public:
    OkValT_(T const& val) : val_(val) {}
    ~OkValT_() {}

    T const& val() const {
        return val_;
    }
};

inline ErrorT_ ErrorVal() { return ErrorT_(); }
inline OkT_ OkVal() { return OkT_(); }

template <typename T>
inline OkValT_<T> OkVal(T const& t) { return OkValT_<T>(t); }

template <typename V>
class Result
{
  private:
    static constexpr unsigned Size = sizeof(V);
    static constexpr unsigned Align = alignof(V);
    alignas(Align) char data_[Size];
    bool isValue_;

    V* valuePtr() {
        return reinterpret_cast<V*>(&data_[0]);
    }
    V const* valuePtr() const {
        return reinterpret_cast<V const*>(&data_[0]);
    }

    Result(V const& v)
      : isValue_(true)
    {
        new (valuePtr()) V(v);
    }
    Result(V&& v)
      : isValue_(true)
    {
        new (valuePtr()) V(std::move(v));
    }
    Result()
      : isValue_(false)
    {}

  public:
    Result(Result<V> const& other) = default;
    Result(Result<V>&& other)
      : isValue_(other.isValue_)
    {
        if (isValue_)
            new (valuePtr()) V(std::move(other.value()));
    }
    Result(ErrorT_ const& error)
      : isValue_(false)
    {}

    Result(OkValT_<V> const& val)
      : isValue_(true)
    {
        new (valuePtr()) V(val.val_);
    }
    Result(OkValT_<V>&& val)
      : isValue_(true)
    {
        new (valuePtr()) V(std::move(val.val_));
    }

    static Result<V> Value(V const& v) {
        return Result<V>(v);
    }
    static Result<V> Value(V&& v) {
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
    explicit operator bool() const {
        return isValue();
    }

    V const& value() const {
        WH_ASSERT(isValue());
        return *(valuePtr());
    }
    V& value() {
        WH_ASSERT(isValue());
        return *(valuePtr());
    }
    void setValue(V const& val) {
        if (isValue()) {
            *(valuePtr()) = val;
        } else {
            new (valuePtr()) V(val);
            isValue_ = true;
        }
    }
    void setValue(V&& val) {
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

    Result<V>& operator =(Result<V> const& other) {
        if (other->isValue())
            setValue(other->value());
        else
            setError();
        return *this;
    }
    Result<V>& operator =(Result<V>&& other) {
        if (other->isValue())
            setValue(std::move(other->value()));
        else
            setError();
        return *this;
    }
};

template <typename P>
class Result<P*>
{
  private:
    P* ptr_;

    Result(P* ptr)
      : ptr_(ptr)
    {
        WH_ASSERT(ptr != nullptr);
    }
    Result()
      : ptr_(nullptr)
    {}

  public:
    Result(ErrorT_ const& error)
      : ptr_(nullptr)
    {}

    static Result<P*> Value(P* ptr) {
        return Result<P*>(ptr);
    }

    static Result<P*> Error() {
        return Result<P*>();
    }

    template <typename Q>
    Result(OkValT_<Q *> const& val)
      : ptr_(val.val_)
    {}
    template <typename Q>
    Result(OkValT_<Q *>&& val)
      : ptr_(val.val_)
    {}

    bool isValue() const {
        return ptr_ != nullptr;
    }
    bool isError() const {
        return ptr_ == nullptr;
    }

    explicit operator bool() const {
        return isValue();
    }
    P* value() const {
        WH_ASSERT(isValue());
        return ptr_;
    }
    void setValue(P* ptr) {
        WH_ASSERT(ptr != nullptr);
        ptr_ = ptr;
    }


    void setError() {
        ptr_ = nullptr;
    }
};

class OkResult
{
  private:
    bool ok_;

    OkResult(bool ok) : ok_(ok) {}

  public:
    OkResult(ErrorT_ const& error)
      : ok_(false)
    {}
    OkResult(OkT_ const& ok)
      : ok_(true)
    {}

    static OkResult Ok() {
        return OkResult(true);
    }
    static OkResult Error() {
        return OkResult(false);
    }

    explicit operator bool() const {
        return ok_;
    }
    bool isOk() const {
        return ok_;
    }
    bool isError() const {
        return !ok_;
    }
};


} // namespace Whisper

#endif // WHISPER__RESULT_HPP
