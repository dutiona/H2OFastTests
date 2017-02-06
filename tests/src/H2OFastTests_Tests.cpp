/*
*
*  (C) Copyright 2016 Michaël Roynard
*
*  Distributed under the MIT License, Version 1.0. (See accompanying
*  file LICENSE or copy at https://opensource.org/licenses/MIT)
*
*  See https://github.com/dutiona/H2OFastTests for documentation.
*/

#include "H2OFastTests.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

#include <type_traits>
#include <utility>

using namespace H2OFastTests::Asserter;

struct CustomClass {
    bool value;
};

bool operator==(const CustomClass& lhs, const CustomClass& rhs) {
    return lhs.value == rhs.value;
}

class CustomException : public std::exception
{};

void throwCustomException() {
    throw CustomException{};
}

register_scenario(H2OFastTests_Tests)
{
    auto epsf_v = 1e-5f;
    auto epsd_v = 1e-5;

    set_up([]() {
        std::cout << "Setup" << std::endl;
    });

    tear_down([]() {
        std::cout << "Teardown" << std::endl;
    });

    skip_test("Test skip", "Assert::AreEqual(double, tolerance = 1e-5)", [epsd_v]() {
        AssertThat(0.).isEqualTo(epsd_v, epsd_v, "Expect 0. == 1e-5");
    });

    add_test("Assert::AreNotEqual(double, tolerance = 1e-5)", [epsd_v]() {
        AssertThat(0.).isNotEqualTo(epsd_v * 10, epsd_v, "Expect 0. != 10e-5");
    });

    add_test("Assert::AreEqual(float, tolerance = 1e-5)", [epsf_v]() {
        AssertThat(0.f).isEqualTo(epsf_v, epsf_v, "Expect 0.f == 1e-5f");
    });

    add_test("Assert::AreNotEqual(float, tolerance = 1e-5)", [epsf_v]() {
        AssertThat(0.f).isNotEqualTo(epsf_v * 10, epsf_v, "Expect 0.f != 10e-5f");
    });

    add_test("Assert::AreEqual(char*, ignoreCase = false)", []() {
        AssertThat("aaa").isEqualTo("aaa", false, "Expect aaa == aaa");
    });

    add_test("Assert::AreNotEqual(char*, ignoreCase = false)", []() {
        AssertThat("aaa").isNotEqualTo("aAa", false, "Expect aaa != aAa");
    });

    add_test("Assert::AreEqual(char*, ignoreCase = true)", []() {
        AssertThat("aaa").isEqualTo("aAa", true, "Expect aaa == aAa");
    });

    add_test("Assert::AreNotEqual(char*, ignoreCase = true)", []() {
        AssertThat("aaa").isNotEqualTo("aAb", true, "Expect aaa != aAb");
    });

    add_test("Assert::AreEqual(std::string, ignoreCase = false)", []() {
        AssertThat(std::string{ "aaa" }).isEqualTo(std::string{ "aaa" }, false, "Expect aaa == aaa");
    });

    add_test("Assert::AreNotEqual(std::string, ignoreCase = false)", []() {
        AssertThat(std::string{ "aaa" }).isNotEqualTo(std::string{ "aAa" }, false, "Expect aaa != aAa");
    });

    add_test("Assert::AreEqual(std::string, ignoreCase = true)", []() {
        AssertThat(std::string{ "aaa" }).isEqualTo(std::string{ "aAa" }, true, "Expect aaa == aAa");
    });

    add_test("Assert::AreNotEqual(std::string, ignoreCase = true)", []() {
        AssertThat(std::string{ "aaa" }).isNotEqualTo(std::string{ "aAb" }, true, "Expect aaa != aAb");
    });

    add_test("Assert::AreEqual(CustomClass)", []() {
        AssertThat(CustomClass{ true }).isEqualTo(CustomClass{ true }, "Expect CustomClass{ true } == CustomClass{ true }");
    });

    add_test("Assert::AreNotEqual(CustomClass)", []() {
        AssertThat(CustomClass{ true }).isNotEqualTo(CustomClass{ false }, "Expect CustomClass{ true } != CustomClass{ false}");
    });

    int a = 0;
    a++;

    add_test("Assert::AreSame(void*)", []() {
        void* a;
        AssertThat(a).isSameAs(a, "Expect &a == &a");
        //throw std::runtime_error{ ":(" }; // Checking error display
    });

    add_test("Assert::AreNotSame(void*)", []() {
        void* a; void* b;
        AssertThat(a).isNotSameAs(b, "Expect &a == &a");
    });

    add_test("Assert::IsNull(nullptr)", []() {
        void* a = nullptr;
        AssertThat(a).isNull("Expect nullptr == nullptr");
    });

    add_test("Assert::IsNotNull(int)", []() {
        int a = 0;
        AssertThat(&a).isNotNull("Expect int != nullptr");
    });

    add_test("Assert::IsTrue(true)", []() {
        AssertThat(true).isTrue("Expect true == true");
    });

    add_test("Assert::IsFalse(false)", []() {
        AssertThat(false).isFalse("Expect true != false");
    });

    add_test("Assert::ExceptException<CustomException>", []() {
        AssertThat([]() {
            throw CustomException{};
        }).expectException<CustomException>("Expect catch(CustomException)");

        AssertThat(throwCustomException).expectException<CustomException>("Expect catch(CustomException)");
    });
}


//int main(int /*argc*/, char** /*argv*/) {
//    register_observer(H2OFastTests_Tests, H2OFastTests::ConsoleIO_Observer);
//    run_scenario(H2OFastTests_Tests);
//    //print_result_verbose(H2OFastTests_Tests);
//
//    std::cout << "Press enter to continue...";
//    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//}



int foo(double, double, float, long) {
    std::cout << "int foo(double, double, float, long)" << std::endl;
    return 0;
}

class Foo {
public:
    Foo() {}

    char bar(long, double, int) {
        std::cout << "chat Foo::bar(long, double, int)" << std::endl;
        return 'c';
    }

    static void hey(long) {
        std::cout << "void Foo::hey(long)" << std::endl;
    }
};


int main() {

    auto d_foo = decorate<int(double, double, float, long)>::from(foo);
    d_foo(0., 0., 0.f, 0l);

    Foo f;
    auto d_foo_bar = decorate<char(long, double, int)>::from<Foo, &Foo::bar>(&f);
    d_foo_bar(0l, 0., 0);

    auto d_foo_hey = decorate<void(long)>::from(Foo::hey);
    d_foo_hey(0l);

    return 0;
}