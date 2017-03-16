#pragma once

#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <iostream>

template <class Type>
class delegate;

template<class ReturnType, class ... ArgumentType>
class delegate<ReturnType(ArgumentType...)> {

protected:

    template <class ClassType>
    using member_pair = std::pair<ClassType* const, ReturnType(ClassType::* const)(ArgumentType...)>;
    template <class> struct is_member_pair : std::false_type {};
    template <class ClassType> struct is_member_pair<std::pair<ClassType* const, ReturnType(ClassType::* const)(ArgumentType...)>> : std::true_type {};

    template <class ClassType>
    using const_member_pair = std::pair<ClassType const* const, ReturnType(ClassType::* const)(ArgumentType...) const>;
    template <class> struct is_const_member_pair : std::false_type {};
    template <class ClassType> struct is_const_member_pair<std::pair<ClassType const* const, ReturnType(ClassType::* const)(ArgumentType...) const>> : std::true_type {};

    using stub_ptr_type = ReturnType(*) (void*, ArgumentType&&...);

    delegate(void* const o, stub_ptr_type const m) noexcept : object_ptr_(o), stub_ptr_(m) {}

public:

    delegate() = default;
    delegate(delegate const&) = default;
    delegate(delegate&&) = default;
    delegate(std::nullptr_t const) noexcept : delegate() { }

    template <class ClassType, class = std::enable_if_t<std::is_class<ClassType>::value>>
    explicit delegate(ClassType const* const o) noexcept : object_ptr_(const_cast<ClassType*>(o)) {}

    template <class ClassType, class = std::enable_if_t<std::is_class<ClassType>::value>>
    explicit delegate(ClassType const& o) noexcept : object_ptr_(const_cast<ClassType*>(&o)) {}

    template <class ClassType>
    delegate(ClassType* const object_ptr, ReturnType(ClassType::* const method_ptr) (ArgumentType...)) {
        *this = from(object_ptr, method_ptr);
    }

    template <class ClassType>
    delegate(ClassType* const object_ptr, ReturnType(ClassType::* const method_ptr) (ArgumentType...) const) {
        *this = from(object_ptr, method_ptr);
    }

    template <class ClassType>
    delegate(ClassType& object, ReturnType(ClassType::* const method_ptr) (ArgumentType...)) {
        *this = from(object, method_ptr);
    }

    template <class ClassType>
    delegate(ClassType const& object, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const) {
        *this = from(object, method_ptr);
    }

    template <class Functor, class = std::enable_if_t<!std::is_same<delegate, std::decay_t<Functor>>::value>>
    delegate(Functor&& f) : store_(operator new(sizeof(std::decay_t<Functor>)), functor_deleter<std::decay_t<Functor>>), store_size_(sizeof(std::decay_t<Functor>)) {
        using functor_type = std::decay_t<Functor>;
        new (store_.get()) functor_type(std::forward<Functor>(f));
        object_ptr_ = store_.get();
        stub_ptr_ = functor_stub<functor_type>;
        deleter_ = deleter_stub<functor_type>;
    }

    delegate& operator=(delegate const&) = default;
    delegate& operator=(delegate&&) = default;

    template <class ClassType>
    delegate& operator=(ReturnType(ClassType::* const rhs) (ArgumentType...)) {
        return *this = from(static_cast<ClassType*>(object_ptr_), rhs);
    }

    template <class ClassType>
    delegate& operator=(ReturnType(ClassType::* const rhs) (ArgumentType...) const) {
        return *this = from(static_cast<ClassType const*>(object_ptr_), rhs);
    }

    template <class Functor, class = std::enable_if_t<!std::is_same<delegate, std::decay_t<Functor>>::value>>
    delegate& operator=(Functor&& f) {
        using functor_type = std::decay_t<Functor>;

        if ((sizeof(functor_type) > store_size_) || !store_.unique()) {
            store_.reset(operator new(sizeof(functor_type)), functor_deleter<functor_type>);
            store_size_ = sizeof(functor_type);
        }
        else {
            deleter_(store_.get());
        }

        new (store_.get()) functor_type(std::forward<Functor>(f));
        object_ptr_ = store_.get();
        stub_ptr_ = functor_stub<functor_type>;
        deleter_ = deleter_stub<functor_type>;
        return *this;
    }

    template <ReturnType(*const function_ptr)(ArgumentType...)>
    static delegate from() noexcept {
        return{ nullptr, function_stub<function_ptr> };
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...)>
    static delegate from(ClassType* const object_ptr) noexcept {
        return{ object_ptr, method_stub<ClassType, method_ptr> };
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const>
    static delegate from(ClassType const* const object_ptr) noexcept {
        return{ const_cast<ClassType*>(object_ptr), const_method_stub<ClassType, method_ptr> };
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...)>
    static delegate from(ClassType& object) noexcept {
        return{ &object, method_stub<ClassType, method_ptr> };
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const>
    static delegate from(ClassType const& object) noexcept {
        return{ const_cast<ClassType*>(&object), const_method_stub<ClassType, method_ptr> };
    }

    template <class Functor>
    static delegate from(Functor&& f) {
        return std::forward<Functor>(f);
    }

    static delegate from(ReturnType(*const function_ptr)(ArgumentType...)) {
        return function_ptr;
    }

    template <class ClassType>
    static delegate from(ClassType* const object_ptr, ReturnType(ClassType::* const method_ptr)(ArgumentType...)) {
        return member_pair<ClassType>(object_ptr, method_ptr);
    }

    template <class ClassType>
    static delegate from(ClassType const* const object_ptr, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const) {
        return const_member_pair<ClassType>(object_ptr, method_ptr);
    }

    template <class ClassType>
    static delegate from(ClassType& object, ReturnType(ClassType::* const method_ptr)(ArgumentType...)) {
        return member_pair<ClassType>(&object, method_ptr);
    }

    template <class ClassType>
    static delegate from(ClassType const& object, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const) {
        return const_member_pair<ClassType>(&object, method_ptr);
    }

    void reset() { stub_ptr_ = nullptr; store_.reset(); }
    void reset_stub() noexcept { stub_ptr_ = nullptr; }
    void swap(delegate& other) noexcept { std::swap(*this, other); }

    bool operator==(delegate const& rhs) const noexcept { return (object_ptr_ == rhs.object_ptr_) && (stub_ptr_ == rhs.stub_ptr_); }
    bool operator!=(delegate const& rhs) const noexcept { return !operator==(rhs); }
    bool operator<(delegate const& rhs) const noexcept { return (object_ptr_ < rhs.object_ptr_) || ((object_ptr_ == rhs.object_ptr_) && (stub_ptr_ < rhs.stub_ptr_)); }
    bool operator==(std::nullptr_t const) const noexcept { return !stub_ptr_; }
    bool operator!=(std::nullptr_t const) const noexcept { return stub_ptr_; }

    explicit operator bool() const noexcept { return stub_ptr_; }

    /* /!\ ACTUAL CALL /!\ */
    virtual ReturnType operator()(ArgumentType&& ... args) const { return stub_ptr_(object_ptr_, std::forward<ArgumentType>(args)...); }

private:

    friend struct std::hash<delegate>;
    using deleter_type = void(*) (void*);

    void* object_ptr_;
    stub_ptr_type stub_ptr_;

    deleter_type deleter_;

    std::shared_ptr<void> store_;
    std::size_t store_size_;

    template <class Type>
    static void functor_deleter(void* const p) {
        static_cast<Type*>(p)->~Type();
        operator delete(p);
    }

    template <class Type>
    static void deleter_stub(void* const p) {
        static_cast<Type*>(p)->~Type();
    }

    template <ReturnType(*function_ptr)(ArgumentType...)>
    static ReturnType function_stub(void* const, ArgumentType&& ... args) {
        return function_ptr(std::forward<ArgumentType>(args)...);
    }

    template <class ClassType, ReturnType(ClassType::*method_ptr)(ArgumentType...)>
    static ReturnType method_stub(void* const object_ptr, ArgumentType&& ... args) {
        return (static_cast<ClassType*>(object_ptr)->*method_ptr)(std::forward<ArgumentType>(args)...);
    }

    template <class ClassType, ReturnType(ClassType::*method_ptr)(ArgumentType...) const>
    static ReturnType const_method_stub(void* const object_ptr, ArgumentType&& ... args) {
        return (static_cast<ClassType const*>(object_ptr)->*method_ptr)(std::forward<ArgumentType>(args)...);
    }

    template <class Functor>
    static std::enable_if_t<!(is_member_pair<Functor>::value || is_const_member_pair<Functor>::value), ReturnType>
        functor_stub(void* const object_ptr, ArgumentType&& ... args) {
        return (*static_cast<Functor*>(object_ptr))(std::forward<ArgumentType>(args)...);
    }

    template <class Functor>
    static std::enable_if_t<is_member_pair<Functor>::value || is_const_member_pair<Functor>::value, ReturnType>
        functor_stub(void* const object_ptr, ArgumentType&& ... args) {
        return (static_cast<Functor*>(object_ptr)->first->*static_cast<Functor*>(object_ptr)->second)(std::forward<ArgumentType>(args)...);
    }
};

namespace std
{
    template <class ReturnType, class ... ArgumentType>
    struct hash<delegate<ReturnType(ArgumentType...)>> {
        size_t operator()(delegate<ReturnType(ArgumentType...)> const& d) const noexcept {
            auto const seed(hash<void*>()(d.object_ptr_));
            return hash<typename delegate<ReturnType(ArgumentType...)>::stub_ptr_type>()(d.stub_ptr_) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };
}