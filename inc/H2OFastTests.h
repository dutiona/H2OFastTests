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

#ifndef H2OFASTTESTS_H
#define H2OFASTTESTS_H

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "H2OFastTests_config.h"

namespace H2OFastTests {

	// Implementation details
	namespace detail {

		// Line info struct
		// Holds line number, file name and function name if relevant
		struct LineInfo {
			const std::string file_;
			const std::string func_;
			const int line_;
			LineInfo(const char* file, const char* func, int line)
				: file_(file), func_(func), line_(line)
			{}

			LineInfo& operator=(const LineInfo&) = delete;
		};

		using line_info_t = LineInfo;

		// Basic display
		std::ostream& operator<<(std::ostream& os, const line_info_t& lineInfo) {
			os << lineInfo.file_ << ":" << lineInfo.line_ << " " << lineInfo.func_;
			return os;
		}

		// Internal exception raised when a test failed
		// Used by internal test runner and assert tool to communicate over the test
		class TestFailure : public std::exception {
		public:
			TestFailure(const std::string& message)
				: message_(message) {}
			virtual const char * what() const { return message_.c_str(); }
			TestFailure& operator=(const TestFailure&) = delete;
		private:
			const std::string message_;
		};

		using test_failure_t = TestFailure;

		// Internal impl for processing an assert and raise the TestFailure Exception
		void FailureTest(bool condition, const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			std::ostringstream oss;
			if (!condition) {
				if (lineInfo != nullptr) {
					oss << ((message != nullptr) ? message : "") << "\t(" << *lineInfo << ")";
				}
				else {
					oss << ((message != nullptr) ? message : "");
				}
				throw test_failure_t(oss.str());
			}
		}

		// Assert test class to help verbosing test logic into lambda's impl
		template<class Expr>
		class AsserterExpression {
		public:

			using empty_expression_t = AsserterExpression<nullptr_t>;

			template<class Expr>
			friend AsserterExpression<Expr> AssertThat(Expr&& expr);

			AsserterExpression()
				: expr_(nullptr)
			{}

			AsserterExpression(Expr&& expr) // Expr must be copyable or movable
				: expr_(std::forward<Expr>(expr))
			{}

			AsserterExpression& operator=(const AsserterExpression& rhs) {
				static_assert(std::is_same<Expr, decltype(rhs.expr_)>::value, "Cannot assign between different types.");
				expr_ = rhs.expr_;
			}

			template<class NewExpr>
			AsserterExpression<NewExpr> andThat(NewExpr&& expr) {
				return{ std::forward<NewExpr>(expr) };
			}

			// True condition
			empty_expression_t isTrue(const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(expr_, message, lineInfo);
				return{};
			}

			// False condition
			empty_expression_t isFalse(const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(!expr_, message, lineInfo);
				return{};
			}

			// Verify that two references refer to the same object instance (identity):
			template<class T>
			empty_expression_t isSameAs(const T& actual,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(&expr_ == &actual, message, lineInfo);
				return{};
			}

			// Verify that two references do not refer to the same object instance (identity):
			template<class T>
			empty_expression_t isNotSameAs(const T& actual,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(!(&expr_ == &actual), message, lineInfo);
				return{};
			}

			// Verify that a pointer is nullptr:
			empty_expression_t isNull(const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(expr_ == nullptr, message, lineInfo);
				return{};
			}

			// Verify that a pointer is not nullptr:
			empty_expression_t isNotNull(const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(expr_ != nullptr, message, lineInfo);
				return{};
			}

			// Force the test case result to be fail:
			empty_expression_t fail(const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(false, message, lineInfo);
				return{};
			}

			// Verify that a function raises an exception:
			template<class ExpectedException>
			empty_expression_t expectException(const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				try {
					expr_();
				}
				catch (ExpectedException) {
					return{};
				}
				catch (...) {
					return fail(message, lineInfo);
				}
				return fail(message, lineInfo);
			}

			// Invoque operator == on T
			template<class T>
			empty_expression_t isEqualTo(const T& expected,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(expr_ == expected, message, lineInfo);
				return{};
			}

			// Check if 2 doubles are almost equals (tolerance given)
			empty_expression_t isEqualTo(double expected, double tolerance,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				double diff = expected - expr_;
				FailureTest(fabs(diff) <= fabs(tolerance), message, lineInfo);
				return{};
			}

			// Check if 2 floats are almost equals (tolerance given)
			empty_expression_t isEqualTo(float expected, float tolerance,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				float diff = expected - expr_;
				FailureTest(fabs(diff) <= fabs(tolerance), message, lineInfo);
				return{};
			}

			// Check if 2 char* are equals, considering the case by default
			empty_expression_t isEqualTo(const char* expected, bool ignoreCase = false,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				auto expected_str = std::string{ expected };
				auto expr_str = std::string{ expr_ };
				if (ignoreCase) {
					std::transform(expected_str.begin(), expected_str.end(), expected_str.begin(), ::tolower);
					std::transform(expr_str.begin(), expr_str.end(), expr_str.begin(), ::tolower);
				}
				FailureTest(expected_str == expr_str, message, lineInfo);
				return{};
			}

			// Check if 2 strings are equals, considering the case by default
			empty_expression_t isEqualTo(std::string expected, bool ignoreCase = false,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				if (ignoreCase) {
					std::transform(expected.begin(), expected.end(), expected.begin(), ::tolower);
					std::transform(expr_.begin(), expr_.end(), expr_.begin(), ::tolower);
				}
				FailureTest(expected == expr_, message, lineInfo);
				return{};
			}

			// Invoque !operator == on T
			template<class T>
			empty_expression_t isNotEqualTo(const T& notExpected,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				FailureTest(!(notExpected == expr_), message, lineInfo);
				return{};
			}

			// Check if 2 doubles are not almost equals (tolerance given)
			empty_expression_t isNotEqualTo(double notExpected, double tolerance,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				double diff = notExpected - expr_;
				FailureTest(fabs(diff) > fabs(tolerance), message, lineInfo);
				return{};
			}

			// Check if 2 floats are not almost equals (tolerance given)
			empty_expression_t isNotEqualTo(float notExpected, float expr_, float tolerance,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				float diff = notExpected - expr_;
				FailureTest(fabs(diff) > fabs(tolerance), message, lineInfo);
				return{};
			}

			// Check if 2 char* are not equals, considering the case by default
			empty_expression_t isNotEqualTo(const char* notExpected, bool ignoreCase = false,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				auto notExpected_str = std::string{ notExpected };
				auto expr_str = std::string{ expr_ };
				if (ignoreCase) {
					std::transform(notExpected_str.begin(), notExpected_str.end(), notExpected_str.begin(), ::tolower);
					std::transform(expr_str.begin(), expr_str.end(), expr_str.begin(), ::tolower);
				}
				FailureTest(notExpected_str != expr_str, message, lineInfo);
				return{};
			}

			// Check if 2 strings are not equals, considering the case by default
			empty_expression_t isNotEqualTo(std::string notExpected, bool ignoreCase = false,
				const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
				if (ignoreCase) {
					std::transform(notExpected.begin(), notExpected.end(), notExpected.begin(), ::tolower);
					std::transform(expr_.begin(), expr_.end(), expr_.begin(), ::tolower);
				}
				FailureTest(notExpected != expr_, message, lineInfo);
				return{};
			}

		private:

			Expr expr_; // Current value stored

		};

		// Functor to build an Asserter with template deduction
		template<class Expr>
		AsserterExpression<Expr> AssertThat(Expr&& expr) {
			return{ std::forward<Expr>(expr) };
		}


		using test_func_t = std::function<void(void)>;
		using duration_t = std::chrono::duration<double, std::milli>; // ms

		// Standard class discribing a test
		class Test {
		public:

			// States of the test
			enum Status {
				PASSED,	// test successfuly passed
				FAILED,	// test failed to pass (an assert failed)
				ERROR,	// an error occured during the test :
						// any exception was catched like bad_alloc
				SKIPPED,// test was skipped and not run
				NONE	// the run_scenario function wasn't run yet
						// for the scenario holding the test
			};

			// All available constructors
			Test()
				: Test({}, []() {}) {}
			Test(const test_func_t&& test)
				: Test("", std::move(test)) {}
			Test(const std::string& label)
				: Test(label, []() {}) {}
			Test(const std::string& label, const test_func_t&& test)
				: label_(label), status_(NONE), test_holder_(std::make_unique<test_func_t>(std::move(test)))
			{}

			// Copy forbidden
			Test(const Test&) = delete;
			Test& operator=(const Test&) = delete;

			// Default move impl (for VC2013)
			Test(Test&& test)
				: test_holder_(std::move(test.test_holder_)),
				label_(test.label_), status_(test.status_), exec_time_ms_(test.exec_time_ms_),
				failure_reason_(test.failure_reason_), skipped_reason_(test.skipped_reason_), error_(test.error_)
			{}
			Test&& operator=(Test&& test) {
				test_holder_ = std::move(test.test_holder_);
				label_ = test.label_;
				status_ = test.status_;
				exec_time_ms_ = test.exec_time_ms_;
				failure_reason_ = test.failure_reason_;
				skipped_reason_ = test.skipped_reason_;
				error_ = test.error_;
				return std::move(*this);
			}

			// Virtual destructor impl
			virtual ~Test() {}

			// Information getters
			const std::string& getLabel(bool verbose) const { return getLabel_private(verbose); }
			const std::string& getFailureReason() const { return getFailureReason_private(); }
			const std::string& getSkippedReason() const { return getSkippedReason_private(); }
			const std::string& getError() const { return getError_private(); }
			duration_t getExecTimeMs() const { return getExecTimeMs_private(); }
			Status getStatus() const { return getStatus_private(); }

		protected:

			void run() { run_private(); } // Called by RegistryManager

			// Run the test and capture and set the state
			virtual void run_private() {
				auto start = std::chrono::high_resolution_clock::now();
				try {
					(*test_holder_)(); /* /!\ Here is the test call /!\ */
					status_ = PASSED;
				}
				catch (const test_failure_t& failure) {
					status_ = FAILED;
					failure_reason_ = failure.what();
				}
				catch (const std::exception& e) {
					status_ = ERROR;
					error_ = e.what();
				}
				exec_time_ms_ = std::chrono::high_resolution_clock::now() - start;
			}

			// Informations getters impl
			virtual const std::string& getLabel_private(bool /*verbose*/) const { return label_; }
			virtual const std::string& getFailureReason_private() const { return failure_reason_; }
			virtual const std::string& getSkippedReason_private() const { return skipped_reason_; }
			virtual const std::string& getError_private() const { return error_; }
			virtual duration_t getExecTimeMs_private() const { return exec_time_ms_; }
			virtual Status getStatus_private() const { return status_; }

		protected:

			duration_t exec_time_ms_;
			std::unique_ptr<test_func_t> test_holder_;
			std::string label_;
			std::string failure_reason_;
			std::string skipped_reason_;
			std::string error_;
			Status status_;

			friend class RegistryManager;
		};

		using test_t = detail::Test;

		std::ostream& operator<<(std::ostream& os, test_t::Status status) {
			switch (status)	{
			case test_t::PASSED:
				os << "PASSED";
				break;
			case test_t::FAILED:
				os << "FAILED";
				break;
			case test_t::ERROR:
				os << "ERROR";
				break;
			case test_t::SKIPPED:
				os << "SKIPPED";
				break;
			case test_t::NONE:
			default:
				os << "NOT RUN YET";
				break;
			}
			return os;
		}

		// This class wrap a test and make it so it's skipped (never run)
		class SkippedTest : public Test {
		public:

			SkippedTest(Test&& test)
				: Test{ std::move(test) } {}
			SkippedTest(const std::string& reason, Test&& test)
				: Test{ std::move(test) } {
				skipped_reason_ = reason;
			}

		protected:

			// Put state to skipped and don't run the test
			virtual void run_private() { status_ = SKIPPED; }

		};

		using skipped_test_t = detail::SkippedTest;

		// Helper functions to build/skip a test case
		test_t make_test(test_func_t&& func) { return test_t{ std::move(func) }; }
		test_t make_test(const std::string& label, test_func_t&& func) { return test_t{ label, std::move(func) }; }
		test_t make_skipped_test(test_t&& test) { return skipped_test_t{ std::move(test) }; }
		test_t make_skipped_test(const std::string& reason, test_t&& test) { return skipped_test_t{ reason, std::move(test) }; }
		test_t make_skipped_test(test_func_t&& func) { return skipped_test_t{ std::move(make_test(std::move(func))) }; }
		test_t make_skipped_test(const std::string& reason, test_func_t&& func) { return skipped_test_t{ reason, std::move(make_test(std::move(func))) }; }

		// POD containing informations about a test
		struct TestInfos {
			const test_t& test;
			using status_t = test_t::Status;
			TestInfos(const test_t& t) : test(t) {}
			TestInfos& operator=(const TestInfos&) = delete;
		};

		using tests_infos_t = TestInfos;

		// Interface for making an observer
		class IRegistryObserver {
		public:
			virtual void update(const tests_infos_t& infos) const = 0;
		};

		using registry_observer_t = IRegistryObserver;

		// Implementation of the observable part of the DP observer
		class IRegistryObservable {
		public:
			void notify(const tests_infos_t& infos) const {
				for (auto& observer : list_observers_) {
					observer->update(infos);
				}
			}
			void addObserver(const std::shared_ptr<registry_observer_t>& observer) { list_observers_.insert(observer); }
			void removeObserver(const std::shared_ptr<registry_observer_t>& observer) { list_observers_.erase(observer); }
		private:
			std::set<std::shared_ptr<registry_observer_t>> list_observers_;
		};

		using registry_observable_t = IRegistryObservable;

		// Global static registry storage object
		using test_list_t = std::vector<test_t>;
		using registry_storage_t = std::map<std::string, test_list_t>;

		registry_storage_t& get_registry() {
			static registry_storage_t registry;
			return registry;
		}

		// Manage a registry in a static context
		class RegistryManager : public registry_observable_t{
		public:

			// Build a registry and fill it in a static context
			template<class ... Args>
			RegistryManager(const std::string& label, Args&& ... tests_or_funcs)
				: run_(false), label_(label), exec_time_ms_accumulator_(duration_t{ 0 }) {
				get_registry().emplace(label, test_list_t{});
				push_back(std::forward<Args>(tests_or_funcs)...);
			}

			//Recursive variadic to iterate over the test pack
			void push_back(test_t&& test) {
				get_registry()[label_].push_back(std::move(test));
			}

			template<class ... Args>
			void push_back(test_t&& test, Args&& ... tests_or_funcs) {
				push_back(std::move(test));
				push_back(std::forward<Args>(tests_or_funcs)...);
			}

			void push_back(test_func_t&& func) {
				get_registry()[label_].push_back(std::move(make_test(std::move(func))));
			}

			template<class ... Args>
			void push_back(test_func_t&& func, Args&& ... tests_or_funcs) {
				push_back(std::move(func));
				push_back(std::forward<Args>(tests_or_funcs)...);
			}

			template<class BadArgument, class ... Args>
			void push_back(BadArgument&& unknown_typed_arg, Args&& ... tests_or_funcs) {
				// TODO : decide to eigther crach at compile time
				static_assert(false, "Unknown type passed to initialize test registry.");
				// or print a warning
				//std::cerr << "Ignoring bad type passed to initialize test registry." << std::endl;
				// or silently ignore
				//push_back(std::forward<Args>(tests_or_funcs)...);
			}

			// Run all the tests
			void run_tests() {
				auto& tests = get_registry()[label_];
				for (auto& test : tests) {
					test.run();
					exec_time_ms_accumulator_ += test.getExecTimeMs();
					notify(tests_infos_t{ test });
					switch (test.getStatus()) {
					case test_t::PASSED:
						tests_passed_.push_back(&test);
						break;
					case test_t::FAILED:
						tests_failed_.push_back(&test);
						break;
					case test_t::SKIPPED:
						tests_skipped_.push_back(&test);
						break;
					case test_t::ERROR:
						tests_with_error_.push_back(&test);
						break;
					default: break;
					}
				}
				run_ = true;
			}

			// Get informations
			const std::string& getLabel() const { return label_; }

			size_t getPassedCount() const { return run_ ? tests_passed_.size() : 0; }
			const std::vector<const test_t*>& getPassedTests() const { return tests_passed_; }

			size_t getFailedCount() const { return run_ ? tests_failed_.size() : 0; }
			const std::vector<const test_t*>& getFailedTests() const { return tests_failed_; }

			size_t getSkippedCount() const { return run_ ? tests_skipped_.size() : 0; }
			const std::vector<const test_t*>& getSkippedTests() const { return tests_skipped_; }

			size_t getWithErrorCount() const { return run_ ? tests_with_error_.size() : 0; }
			const std::vector<const test_t*>& getWithErrorTests() const { return tests_with_error_; }

			size_t getAllTestsCount() const { return run_ ? get_registry()[label_].size() : 0; }
			const test_list_t& getAllTests() const { return get_registry()[label_]; }
			duration_t getAllTestsExecTimeMs() const { return run_ ? exec_time_ms_accumulator_ : duration_t{ 0 }; }

		private:

			bool run_;
			std::string label_;
			duration_t exec_time_ms_accumulator_;
			std::vector<const test_t*> tests_passed_;
			std::vector<const test_t*> tests_failed_;
			std::vector<const test_t*> tests_skipped_;
			std::vector<const test_t*> tests_with_error_;

		};

		using registry_manager_t = RegistryManager;
	}

	/*
	*
	* Public interface
	*
	*/

	// Public accessible types
	using line_info_t = detail::line_info_t;
	using test_infos_t = detail::tests_infos_t;
	using registry_storage_t = detail::registry_storage_t;
	using registry_manager_t = detail::RegistryManager;
	using registry_observer_t = detail::IRegistryObserver;

	// Asserter exposition
	namespace Asserter {
		using detail::AsserterExpression;
		using detail::AssertThat;
	}

	// Interface to Implement to access access a registry information
	// Possibility to export services into a DLL for forther customization
	class IRegistryTraversal {
	public:
		IRegistryTraversal(const registry_manager_t& registry) : registry_(registry) {}
		virtual ~IRegistryTraversal() {}
	protected:
		const registry_manager_t& getRegistryManager() const { return registry_; }
	private:
		registry_manager_t registry_;
	};

	// Trivial impl for console display results
	class RegistryTraversal_ConsoleIO : private IRegistryTraversal {
	public:
		RegistryTraversal_ConsoleIO(const registry_manager_t& registry) : IRegistryTraversal{ registry } {}
		std::ostream& print(std::ostream& os, bool verbose) const {
			auto& registry_manager = getRegistryManager();
			os << "UNIT TEST SUMMARY [" << registry_manager.getLabel() << "] [" << registry_manager.getAllTestsExecTimeMs().count() << "ms]:" << std::endl;

			os << "\tPASSED:" << registry_manager.getPassedCount() << "/" << registry_manager.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_manager.getPassedTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl;
				}
			}

			os << "\tFAILED:" << registry_manager.getFailedCount() << "/" << registry_manager.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_manager.getFailedTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl
						<< "\t\tMessage: " << test->getFailureReason() << std::endl;
				}
			}

			os << "\tSKIPPED:" << registry_manager.getSkippedCount() << "/" << registry_manager.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_manager.getSkippedTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl;
				}
			}

			os << "\tERRORS:" << registry_manager.getWithErrorCount() << "/" << registry_manager.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_manager.getWithErrorTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl
						<< "\t\tMessage: " << test->getError() << std::endl;
				}
			}

			return os;
		}
	};

	// Observer impl example
	class ConsoleIO_Observer : public registry_observer_t {
		virtual void update(const test_infos_t& infos) const {
			std::cout << (infos.test.getStatus() == test_infos_t::status_t::SKIPPED ? "SKIPPING TEST [" : "RUNNING TEST [")
				<< infos.test.getLabel(false) << "] [" << infos.test.getExecTimeMs().count() << "ms]:" << std::endl
				<< "Status: " << infos.test.getStatus() << std::endl;
		}
	};

}

//Helper macros to use the unit test suit
#define register_scenario(name, label, ...) \
	namespace H2OFastTests { static registry_manager_t registry_manager_ ## name {label, __VA_ARGS__ }; }
#define run_scenario(name) \
	H2OFastTests::registry_manager_ ## name.run_tests();

#define describe_test(label, test) \
	H2OFastTests::detail::make_test(label, (test))

#define skip_test(test) \
	H2OFastTests::detail::make_skipped_test((test))
#define skip_test_reason(reason, test) \
	H2OFastTests::detail::make_skipped_test(reason, (test))

#define add_test_to_scenario(name, made_test) \
	H2OFastTests::registry_manager_ ## name.push_back(made_test);

#define register_observer(name, class_name, instance_ptr) \
	H2OFastTests::registry_manager_ ## name.addObserver(std::shared_ptr<class_name>(instance_ptr));

#define print_result(name, verbose) \
	H2OFastTests::RegistryTraversal_ConsoleIO(H2OFastTests::registry_manager_ ## name).print(std::cout, (verbose))

#define line_info() \
	&H2OFastTests::line_info_t(__FILE__, "", __LINE__)
#define line_info_f() \
	&H2OFastTests::line_info_t(__FILE__, __FUNCTION__, __LINE__)

#endif
