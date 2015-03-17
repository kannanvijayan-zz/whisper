#ifndef WHISPER__RESULT_HPP
#define WHISPER__RESULT_HPP

#include "common.hpp"
#include "debug.hpp"
#include <new>

namespace Whisper {


// Result class.
template <typename V, typename E>
class Result
{
  private:
    static constexpr unsigned Size = Max(sizeof(V), sizeof(E));
    static constexpr unsigned Align = Max(alignof(V), alignof(E));
    alignas(Align) char data_[Size];
    bool isValue_;

    V *valuePtr() {
        return reinterpret_cast<V *>(&data_[0]);
    }
    const V *valuePtr() const {
        return reinterpret_cast<const V *>(&data_[0]);
    }

    E *errorPtr() {
        return reinterpret_cast<E *>(&data_[0]);
    }
    const E *errorPtr() const {
        return reinterpret_cast<const E *>(&data_[0]);
    }

    struct ValueInit { constexpr ValueInit() {} };
    struct ErrorInit { constexpr ErrorInit() {} };
    static constexpr ValueInit V_ = ValueInit();
    static constexpr ErrorInit E_ = ErrorInit();

    Result(ValueInit, const V &v)
      : isValue_(true)
    {
        new (valuePtr()) V(v);
    }
    Result(ValueInit, V &&v)
      : isValue_(true)
    {
        new (valuePtr()) V(std::move(v));
    }

    Result(ErrorInit, const E &e)
      : isValue_(false)
    {
        new (errorPtr()) E(e);
    }
    Result(ErrorInit, E &&e)
      : isValue_(false)
    {
        new (errorPtr()) E(std::move(e));
    }

  public:
    Result<V, E>(const Result<V, E> &other) = default;
    Result<V, E>(Result<V, E> &&other)
      : isValue_(other.isValue_)
    {
        if (isValue_)
            new (valuePtr()) V(std::move(other.value()));
        else
            new (errorPtr()) E(std::move(other.error()));
    }

    static Result<V, E> Value(const V &v) {
        return Result<V, E>(V_, v);
    }
    static Result<V, E> Value(V &&v) {
        return Result<V, E>(V_, std::move(v));
    }

    static Result<V, E> Error(const E &e) {
        return Result<V, E>(E_, e);
    }
    static Result<V, E> Error(E &&e) {
        return Result<V, E>(E_, std::move(e));
    }

    ~Result() {
        if (isValue())
            valuePtr()->~V();
        else
            errorPtr()->~E();
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
            errorPtr()->~E();
            new (valuePtr()) V(val);
            isValue_ = true;
        }
    }
    void setValue(V &&val) {
        if (isValue()) {
            *(valuePtr()) = std::move(val);
        } else {
            errorPtr()->~E();
            new (valuePtr()) V(std::move(val));
            isValue_ = true;
        }
    }

    const E &error() const {
        WH_ASSERT(isError());
        return *(errorPtr());
    }
    E &error() {
        WH_ASSERT(isError());
        return *(errorPtr());
    }
    void setError(const E &err) {
        if (isError()) {
            *(errorPtr()) = err;
        } else {
            valuePtr()->~V();
            new (errorPtr()) E(err);
            isValue_ = false;
        }
    }
    void setError(E &&err) {
        if (isError()) {
            *(errorPtr()) = std::move(err);
        } else {
            valuePtr()->~V();
            new (errorPtr()) E(std::move(err));
            isValue_ = false;
        }
    }

    Result<V, E> &operator =(const Result<V, E> &other) {
        if (other->isValue())
            setValue(other->value());
        else
            setError(other->error());
        return *this;
    }
    Result<V, E> &operator =(Result<V, E> &&other) {
        if (other->isValue())
            setValue(std::move(other->value()));
        else
            setError(std::move(other->error()));
        return *this;
    }
};

template <typename V>
class Result<V, void>
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
    Result<V, void>(const Result<V, void> &other) = default;
    Result<V, void>(Result<V, void> &&other)
      : isValue_(other.isValue_)
    {
        if (isValue_)
            new (valuePtr()) V(std::move(other.value()));
    }

    static Result<V, void> Value(const V &v) {
        return Result<V, void>(v);
    }
    static Result<V, void> Value(V &&v) {
        return Result<V, void>(std::move(v));
    }

    static Result<V, void> Error() {
        return Result<V, void>();
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

    Result<V, void> &operator =(const Result<V, void> &other) {
        if (other->isValue())
            setValue(other->value());
        else
            setError();
        return *this;
    }
    Result<V, void> &operator =(Result<V, void> &&other) {
        if (other->isValue())
            setValue(std::move(other->value()));
        else
            setError();
        return *this;
    }
};

template <typename E>
class Result<void, E>
{
  private:
    static constexpr unsigned Size = sizeof(E);
    static constexpr unsigned Align = alignof(E);
    alignas(Align) char data_[Size];
    bool isValue_;

    E *errorPtr() {
        return reinterpret_cast<E *>(&data_[0]);
    }
    const E *errorPtr() const {
        return reinterpret_cast<const E *>(&data_[0]);
    }

    Result()
      : isValue_(true)
    {}

    Result(const E &e)
      : isValue_(false)
    {
        new (errorPtr()) E(e);
    }
    Result(E &&e)
      : isValue_(false)
    {
        new (errorPtr()) E(std::move(e));
    }

  public:
    Result<void, E>(const Result<void, E> &other) = default;
    Result<void, E>(Result<void, E> &&other)
      : isValue_(other.isValue_)
    {
        if (!isValue_)
            new (errorPtr()) E(std::move(other.error()));
    }

    static Result<void, E> Value() {
        return Result<void, E>();
    }

    static Result<void, E> Error(const E &e) {
        return Result<void, E>(e);
    }
    static Result<void, E> Error(E &&e) {
        return Result<void, E>(std::move(e));
    }

    ~Result() {
        if (isError())
            errorPtr()->~E();
    }

    bool isValue() const {
        return isValue_;
    }
    bool isError() const {
        return !isValue_;
    }

    void setValue() {
        if (isError()) {
            errorPtr()->~E();
            isValue_ = true;
        }
    }

    const E &error() const {
        WH_ASSERT(isError());
        return *(errorPtr());
    }
    E &error() {
        WH_ASSERT(isError());
        return *(errorPtr());
    }
    void setError(const E &err) {
        if (isError()) {
            *(errorPtr()) = err;
        } else {
            new (errorPtr()) E(err);
            isValue_ = false;
        }
    }
    void setError(E &&err) {
        if (isError()) {
            *(errorPtr()) = std::move(err);
        } else {
            new (errorPtr()) E(std::move(err));
            isValue_ = false;
        }
    }

    Result<void, E> &operator =(const Result<void, E> &other) {
        if (other->isValue())
            setValue();
        else
            setError(other->error());
        return *this;
    }
    Result<void, E> &operator =(Result<void, E> &&other) {
        if (other->isValue())
            setValue();
        else
            setError(std::move(other->error()));
        return *this;
    }
};

template <>
class Result<void, void>
{
  private:
    bool isValue_;

    Result(bool isValue)
      : isValue_(isValue)
    {}

  public:
    Result<void, void>(const Result<void, void> &other) = default;
    Result<void, void>(Result<void, void> &&other)
      : isValue_(other.isValue_)
    {}

    static Result<void, void> Value() {
        return Result<void, void>(true);
    }
    static Result<void, void> Error() {
        return Result<void, void>(false);
    }

    ~Result() {}

    bool isValue() const {
        return isValue_;
    }
    bool isError() const {
        return !isValue_;
    }

    void setValue() {
        isValue_ = true;
    }
    void setError() {
        isValue_ = false;
    }

    Result<void, void> &operator =(const Result<void, void> &other) {
        if (other.isValue())
            setValue();
        else
            setError();
        return *this;
    }
    Result<void, void> &operator =(Result<void, void> &&other) {
        if (other.isValue())
            setValue();
        else
            setError();
        return *this;
    }
};

template <typename V>
using ValueResult = Result<V, void>;

template <typename E>
using ErrorResult = Result<void, E>;

typedef Result<void, void> TrivialResult;


} // namespace Whisper

#endif // WHISPER__RESULT_HPP
