/*
*
*  (C) Copyright 2016 Michaël Roynard
*
*  Distributed under the MIT License, Version 1.0. (See accompanying
*  file LICENSE or copy at https://opensource.org/licenses/MIT)
*
*  See https://github.com/dutiona/H2OFastTests for documentation.
*/

#include "H2OFastTests.h"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace H2OFastTests::Asserter;


struct CustomClass {
	bool value;
};

bool operator==(const CustomClass& lhs, const CustomClass& rhs) {
	return lhs.value == rhs.value;
}

class CustomException : public std::exception
{};

void throwCustomException(){
	throw CustomException{};
}

register_scenario(H2OFastTests_Tests, "Tests case scenario for H2OFastTests lib",

	describe_test("Assert::AreEqual(double, tolerance = 1e-5)", [](){
		AssertThat(0.).isEqualTo(1e-5, 1e-5, "Expect 0. == 1e-5", line_info());
	}),

	describe_test("Assert::AreNotEqual(double, tolerance = 1e-5)", [](){
		AssertThat(0.).isNotEqualTo(10e-5, 1e-5, "Expect 0. != 10e-5", line_info());
	}),

	describe_test("Assert::AreEqual(float, tolerance = 1e-5)", [](){
		AssertThat(0.f).isEqualTo(1e-5f, 1e-5f, "Expect 0.f == 1e-5f", line_info());
	}),

	describe_test("Assert::AreNotEqual(float, tolerance = 1e-5)", [](){
		AssertThat(0.f).isNotEqualTo(10e-5f, 1e-5f, "Expect 0.f != 10e-5f", line_info());
	}),

	describe_test("Assert::AreEqual(char*, ignoreCase = false)", [](){
		AssertThat("aaa").isEqualTo("aaa", false, "Expect aaa == aaa", line_info());
	}),

	describe_test("Assert::AreNotEqual(char*, ignoreCase = false)", [](){
		AssertThat("aaa").isNotEqualTo("aAa", false, "Expect aaa != aAa", line_info());
	}),

	describe_test("Assert::AreEqual(char*, ignoreCase = true)", [](){
		AssertThat("aaa").isEqualTo("aAa", true, "Expect aaa == aAa", line_info());
	}),

	describe_test("Assert::AreNotEqual(char*, ignoreCase = true)", [](){
		AssertThat("aaa").isNotEqualTo("aAb", true, "Expect aaa != aAb", line_info());
	}),

	describe_test("Assert::AreEqual(std::string, ignoreCase = false)", [](){
		AssertThat(std::string{ "aaa" }).isEqualTo(std::string{ "aaa" }, false, "Expect aaa == aaa", line_info());
	}),

	describe_test("Assert::AreNotEqual(std::string, ignoreCase = false)", [](){
		AssertThat(std::string{ "aaa" }).isNotEqualTo(std::string{ "aAa" }, false, "Expect aaa != aAa", line_info());
	}),

	describe_test("Assert::AreEqual(std::string, ignoreCase = true)", [](){
		AssertThat(std::string{ "aaa" }).isEqualTo(std::string{ "aAa" }, true, "Expect aaa == aAa", line_info());
	}),

	describe_test("Assert::AreNotEqual(std::string, ignoreCase = true)", [](){
		AssertThat(std::string{ "aaa" }).isNotEqualTo(std::string{ "aAb" }, true, "Expect aaa != aAb", line_info());
	}),

	describe_test("Assert::AreEqual(CustomClass)", [](){
		AssertThat(CustomClass{ true }).isEqualTo(CustomClass{ true }, "Expect CustomClass{ true } == CustomClass{ true }", line_info());
	}),

	describe_test("Assert::AreNotEqual(CustomClass)", [](){
		AssertThat(CustomClass{ true }).isNotEqualTo(CustomClass{ false }, "Expect CustomClass{ true } != CustomClass{ false}", line_info());
	}),

	describe_test("Assert::AreSame(void*)", [](){
		void* a;
		AssertThat(a).isSameAs(a, "Expect &a == &a", line_info());
	}),

	describe_test("Assert::AreNotSame(void*)", [](){
		void* a; void* b;
		AssertThat(a).isNotSameAs(b, "Expect &a == &a", line_info());
	}),

	describe_test("Assert::IsNull(nullptr)", [](){
		void* a = nullptr;
		AssertThat(a).isNull("Expect nullptr == nullptr", line_info());
	}),

	describe_test("Assert::IsNotNull(int)", [](){
		int a = 0;
		AssertThat(&a).isNotNull("Expect int != nullptr", line_info());
	}),

	describe_test("Assert::IsTrue(true)", [](){
		AssertThat(true).isTrue("Expect true == true", line_info());
	}),

	describe_test("Assert::IsFalse(false)", [](){
		AssertThat(false).isFalse("Expect true != false", line_info());
	}),

	describe_test("Assert::ExceptException<CustomException>", [](){
		AssertThat([](){
			throw CustomException{};
		}).expectException<CustomException>("Expect catch(CustomException)", line_info());

		AssertThat(throwCustomException).expectException<CustomException>("Expect catch(CustomException)", line_info());
	})
);

int main(int /*argc*/, char* /*argv[]*/){
	run_scenario(H2OFastTests_Tests);
	print_result(H2OFastTests_Tests, true);

	std::cout << "Press enter to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}