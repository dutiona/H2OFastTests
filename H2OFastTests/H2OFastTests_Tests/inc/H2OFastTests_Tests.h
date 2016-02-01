/*
*
*  (C) Copyright 2016 Michaël Roynard
*
*  Distributed under the MIT License, Version 1.0. (See accompanying
*  file LICENSE or copy at https://opensource.org/licenses/MIT)
*
*  See https://github.com/dutiona/H2OFastTests for documentation.
*/

#pragma once

#ifndef H2OFASTTESTS_TESTS_H
#define H2OFASTTESTS_TESTS_H

#include "H2OFastTests.h"

using H2OFastTests::Assert;


struct CustomClass {
	bool value;
};

bool operator==(const CustomClass& lhs, const CustomClass& rhs) {
	return lhs.value == rhs.value;
}

register_scenario(H2OFastTests_Tests, "Tests case scenario for H2OFastTests lib",

	describe_test("Assert::AreEqual(double, tolerance = 1e-5)", [](){
		Assert::AreEqual(0., 1e-5, 1e-5, "Expect 0. == 1e-5", line_info());
	}),

	describe_test("Assert::AreNotEqual(double, tolerance = 1e-5)", [](){
		Assert::AreNotEqual(0., 10e-5, 1e-5, "Expect 0. != 10e-5", line_info());
	}),

	describe_test("Assert::AreEqual(float, tolerance = 1e-5)", [](){
		Assert::AreEqual(0.f, 1e-5f, 1e-5f, "Expect 0.f == 1e-5f", line_info());
	}),

	describe_test("Assert::AreNotEqual(float, tolerance = 1e-5)", [](){
		Assert::AreNotEqual(0.f, 10e-5f, 1e-5f, "Expect 0.f != 10e-5f", line_info());
	}),

	describe_test("Assert::AreEqual(char*, ignoreCase = false)", [](){
		Assert::AreEqual("aaa", "aaa", false, "Expect aaa == aaa", line_info());
	}),

	describe_test("Assert::AreNotEqual(char*, ignoreCase = false)", [](){
		Assert::AreNotEqual("aaa", "aAa", false, "Expect aaa != aAa", line_info());
	}),

	describe_test("Assert::AreEqual(char*, ignoreCase = true)", [](){
		Assert::AreEqual("aaa", "aAa", true, "Expect aaa == aAa", line_info());
	}),

	describe_test("Assert::AreNotEqual(char*, ignoreCase = true)", [](){
		Assert::AreNotEqual("aaa", "aAb", true, "Expect aaa != aAb", line_info());
	}),

	describe_test("Assert::AreEqual(std::string, ignoreCase = false)", [](){
		Assert::AreEqual(std::string{ "aaa" }, std::string{ "aaa" }, false, "Expect aaa == aaa", line_info());
	}),

	describe_test("Assert::AreNotEqual(std::string, ignoreCase = false)", [](){
		Assert::AreNotEqual(std::string{ "aaa" }, std::string{ "aAa" }, false, "Expect aaa != aAa", line_info());
	}),

	describe_test("Assert::AreEqual(std::string, ignoreCase = true)", [](){
		Assert::AreEqual(std::string{ "aaa" }, std::string{ "aAa" }, true, "Expect aaa == aAa", line_info());
	}),

	describe_test("Assert::AreNotEqual(std::string, ignoreCase = true)", [](){
		Assert::AreNotEqual(std::string{ "aaa" }, std::string{ "aAb" }, true, "Expect aaa != aAb", line_info());
	}),

	describe_test("Assert::AreEqual(CustomClass)", [](){
		Assert::AreEqual(CustomClass{ true }, CustomClass{ true }, "Expect CustomClass{ true } == CustomClass{ true }", line_info());
	}),

	describe_test("Assert::AreNotEqual(CustomClass)", [](){
		Assert::AreNotEqual(CustomClass{ true }, CustomClass{ false }, "Expect CustomClass{ true } != CustomClass{ false}", line_info());
	}),

	describe_test("Assert::AreSame(void*)", [](){
		void* a;
		Assert::AreSame(a, a, "Expect &a == &a", line_info());
	}),

	describe_test("Assert::AreNotSame(void*)", [](){
		void* a; void* b;
		Assert::AreNotSame(a, b, "Expect &a != &b", line_info());
	}),

	describe_test("Assert::IsNull(nullptr)", [](){
		void* a = nullptr;
		Assert::IsNull(a, "Expect nullptr == nullptr", line_info());
	}),

	describe_test("Assert::IsNotNull(int)", [](){
		int a = 0;
		Assert::IsNotNull(&a, "Expect int != nullptr", line_info());
	}),

	describe_test("Assert::IsTrue(true)", [](){
		Assert::IsTrue(true, "Expect true == true", line_info());
	}),

	describe_test("Assert::IsFalse(false)", [](){
		Assert::IsFalse(false, "Expect true != false", line_info());
	})
);

#endif