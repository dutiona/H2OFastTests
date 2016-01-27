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

register_scenario(H2OFastTests_Tests, "Tests case scenario for H2OFastTests lib",

	describe_test_label("FailOnCondition : fail", [](){
		using H2OFastTests::detail::FailOnCondition;
		using H2OFastTests::detail::TestFailure;
		Assert::ExpectException<TestFailure>(
			[](){ FailOnCondition(false, "failure", line_info()); }, "Except TestFailure", line_info());
	})
);

#endif