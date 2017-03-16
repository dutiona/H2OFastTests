#pragma once

#include "Delegate.hpp"

#include <iostream>

template <class Type>
class decorate;

template<class ReturnType, class ... ArgumentType>
class decorate<ReturnType(ArgumentType...)> : public delegate<ReturnType(ArgumentType...)> {
public:

    template<class ... Args>
    decorate(Args&& ... args) : delegate<ReturnType(ArgumentType...)>(std::forward<Args>(args)...) {}

    ReturnType operator()(ArgumentType&& ... args) const override {

        std::cout << "decorated before" << std::endl;
        auto&& result = delegate<ReturnType(ArgumentType...)>::operator()(std::forward<ArgumentType>(args)...);
        std::cout << "decorated after" << std::endl;
        return std::forward<ReturnType>(result);
    }

    template <ReturnType(*const function_ptr)(ArgumentType...)>
    static decorate from() noexcept {
        return delegate<ReturnType(*const function_ptr)(ArgumentType...)>::from();
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...)>
    static decorate from(ClassType* const object_ptr) noexcept {
        return delegate<ReturnType(ArgumentType...)>::template from<ClassType, method_ptr>(object_ptr);
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const>
    static decorate from(ClassType const* const object_ptr) noexcept {
        return delegate<ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const>::from(object_ptr);
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...)>
    static decorate from(ClassType& object) noexcept {
        return delegate<ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...)>::from(object);
    }

    template <class ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const>
    static decorate from(ClassType const& object) noexcept {
        return delegate<ClassType, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const>::from(object);
    }

    template <class Functor>
    static decorate from(Functor&& f) {
        return delegate<Functor>::from(f);
    }

    static decorate from(ReturnType(*const function_ptr)(ArgumentType...)) {
        return delegate<ReturnType(ArgumentType...)>{ function_ptr };
    }

    template <class ClassType>
    static decorate from(ClassType* const object_ptr, ReturnType(ClassType::* const method_ptr)(ArgumentType...)) {
        return delegate<ClassType>::from(object_ptr, method_ptr);
    }
    
    template <class ClassType>
    static decorate from(ClassType const* const object_ptr, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const) {
        return delegate<ClassType>::from(object_ptr, method_ptr);
    }

    template <class ClassType>
    static decorate from(ClassType& object, ReturnType(ClassType::* const method_ptr)(ArgumentType...)) {
        return delegate<ClassType>::from(&object, method_ptr);
    }

    template <class ClassType>
    static decorate from(ClassType const& object, ReturnType(ClassType::* const method_ptr)(ArgumentType...) const) {
        return delegate<ClassType>::from(&object, method_ptr);
    }
};

template<class ... ArgumentType>
class decorate<void(ArgumentType...)> : public delegate<void(ArgumentType...)> {
public:

    template<class ... Args>
    decorate(Args&& ... args) : delegate<void(ArgumentType...)>(std::forward<Args>(args)...) {}

    void operator()(ArgumentType&& ... args) const override {

        std::cout << "decorated before" << std::endl;
        delegate<void(ArgumentType...)>::operator()(std::forward<ArgumentType>(args)...);
        std::cout << "decorated after" << std::endl;
    }

    template <void(*const function_ptr)(ArgumentType...)>
    static decorate from() noexcept {
        return delegate<void(*const function_ptr)(ArgumentType...)>::from();
    }

    template <class ClassType, void(ClassType::* const method_ptr)(ArgumentType...)>
    static decorate from(ClassType* const object_ptr) noexcept {
        return delegate<ClassType, void(ClassType::* const method_ptr)(ArgumentType...)>::from(object_ptr)
    }

    template <class ClassType, void(ClassType::* const method_ptr)(ArgumentType...) const>
    static decorate from(ClassType const* const object_ptr) noexcept {
        return delegate<ClassType, void(ClassType::* const method_ptr)(ArgumentType...) const>::from(object_ptr);
    }

    template <class ClassType, void(ClassType::* const method_ptr)(ArgumentType...)>
    static decorate from(ClassType& object) noexcept {
        return delegate<ClassType, void(ClassType::* const method_ptr)(ArgumentType...)>::from(object);
    }

    template <class ClassType, void(ClassType::* const method_ptr)(ArgumentType...) const>
    static decorate from(ClassType const& object) noexcept {
        return delegate<ClassType, void(ClassType::* const method_ptr)(ArgumentType...) const>::from(object);
    }

    template <class Functor>
    static decorate from(Functor&& f) {
        return delegate<Functor>::from(f);
    }

    static decorate from(void(*const function_ptr)(ArgumentType...)) {
        return delegate<void(ArgumentType...)>{ function_ptr };
    }

    template <class ClassType>
    static decorate from(ClassType* const object_ptr, void(ClassType::* const method_ptr)(ArgumentType...)) {
        return delegate<ClassType>::from(object_ptr, method_ptr);
    }

    template <class ClassType>
    static decorate from(ClassType const* const object_ptr, void(ClassType::* const method_ptr)(ArgumentType...) const) {
        return delegate<ClassType>::from(object_ptr, method_ptr);
    }

    template <class ClassType>
    static decorate from(ClassType& object, void(ClassType::* const method_ptr)(ArgumentType...)) {
        return delegate<ClassType>::from(&object, method_ptr);
    }

    template <class ClassType>
    static decorate from(ClassType const& object, void(ClassType::* const method_ptr)(ArgumentType...) const) {
        return delegate<ClassType>::from(&object, method_ptr);
    }

};