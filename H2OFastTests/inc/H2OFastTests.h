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
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>


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
		};

		// Basic display
		std::ostream& operator<<(std::ostream& os, const LineInfo& lineInfo) {
			os << lineInfo.file_ << ":" << lineInfo.line_ << " " << lineInfo.func_;
			return os;
		}

		// Internal exception raised when a test failed
		// Used by internal test runner and assert tool to communicate over the test
		class TestFailure : public std::exception {
		public:
			TestFailure(const std::string& message) : message_(message) {}
			virtual const char * what() const { return message_.c_str(); }
		private:
			const std::string message_;
		};

		// Internal impl for processing an assert and raise the TestFailure Exception
		void FailOnCondition(bool condition, const char* message = nullptr, const LineInfo* lineInfo = nullptr) {
			if (!condition) {
				std::ostringstream oss;
				if (lineInfo != nullptr) {
					oss << *lineInfo << std::endl << ((message != nullptr) ? message : "");
				}
				else {
					oss << ((message != nullptr) ? message : "");
				}
				throw TestFailure(oss.str());
			}
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
			Test() : Test({}, []() {}) {}
			Test(const test_func_t&& test) : Test("", std::move(test)) {}
			Test(const std::string& label) : Test(label, []() {}) {}
			Test(const std::string& label, const test_func_t&& test)
				: label_(label), status_(NONE), test_holder_(std::make_shared<test_func_t>(std::move(test)))
			{}

			// Default move impl (for VC2013)
			Test(Test&& test)
				: test_holder_(test.test_holder_),
				label_(test.label_), status_(test.status_), exec_time_ms_(test.exec_time_ms_),
				failure_reason_(test.failure_reason_), skipped_reason_(test.skipped_reason_), error_(test.error_)
			{}
			Test&& operator=(Test&& test) {
				test_holder_ = test.test_holder_;
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

		private:

			// NVI for public interface
			void run() { run_private(); }

			// Run the test and capture and set the state
			virtual void run_private() {
				auto start = std::chrono::high_resolution_clock::now();
				try {
					(*test_holder_)(); /* /!\ Here is the test call /!\ */
					status_ = PASSED;
				}
				catch (const TestFailure& failure) {
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
			std::shared_ptr<test_func_t> test_holder_;
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
			case H2OFastTests::detail::Test::PASSED:
				os << "PASSED";
				break;
			case H2OFastTests::detail::Test::FAILED:
				os << "FAILED";
				break;
			case H2OFastTests::detail::Test::ERROR:
				os << "ERROR";
				break;
			case H2OFastTests::detail::Test::SKIPPED:
				os << "SKIPPED";
				break;
			case H2OFastTests::detail::Test::NONE:
			default:
				os << "NOT RUN YET";
				break;
			}
			return os;
		}

		// This class wrap a test and make it so it's skipped (never run)
		class SkippedTest : public Test {
		public:

			SkippedTest(Test&& test) : Test{ std::move(test) } {}
			SkippedTest(const std::string& reason, Test&& test)
				: Test{ std::move(test) } {
				skipped_reason_ = reason;
			}

		private:

			// Put state to skipped and don't run the test
			virtual void run_private() { status_ = SKIPPED; }

		};

		using skipped_test_t = detail::SkippedTest;

		// Helper functions to build/skip a test case
		std::shared_ptr<test_t> make_test(test_func_t&& func) { return std::make_shared<test_t>(std::move(func)); }
		std::shared_ptr<test_t> make_test(const std::string& label, test_func_t&& func) { return std::make_shared<test_t>(label, std::move(func)); }
		std::shared_ptr<test_t> make_test(std::shared_ptr<test_t>&& test) { return test; }
		std::shared_ptr<test_t> make_skipped_test(test_t&& test) { return std::make_shared<skipped_test_t>(std::move(test)); }
		std::shared_ptr<test_t> make_skipped_test(const std::string& reason, test_t&& test) { return std::make_shared<skipped_test_t>(reason, std::move(test)); }
		std::shared_ptr<test_t> make_skipped_test(std::shared_ptr<test_t>&& test) { return std::make_shared<skipped_test_t>(std::move(*test)); }
		std::shared_ptr<test_t> make_skipped_test(const std::string& reason, std::shared_ptr<test_t>&& test) { return std::make_shared<skipped_test_t>(reason, std::move(*test)); }

		using registry_storage_t = std::deque<std::shared_ptr<test_t>>;

		// Registry main impl
		class Registry {
		public:
			// Build a registry with a pointer on the static storage as 2nd arg
			Registry(const std::string& label, registry_storage_t* tests_list) : label_(label), tests_list_(tests_list) {}
			//Getters
			registry_storage_t& get() { return *tests_list_; };
			const registry_storage_t& get() const { return const_cast<Registry*>(this)->get(); };
			const std::string& getLabel() const { return label_; }
		private:
			std::string label_;
			registry_storage_t* tests_list_;
		};

		// POD containing informations about a test
		struct TestInfos {
			const test_t& test;
			using status_t = test_t::Status;
		};

		using tests_infos_t = TestInfos;

		// Interface for making an observer
		class IRegistryObserver {
		public:
			virtual void update(const tests_infos_t& infos) const = 0;
		};

		// Implementation of the observable part of the DP observer
		class IRegistryObservable {
		public:
			void notify(const tests_infos_t& infos) const {
				for (auto& observer : list_observers_) {
					observer->update(infos);
				}
			}
			void addObserver(const std::shared_ptr<IRegistryObserver>& observer) { list_observers_.insert(observer); }
			void removeObserver(const std::shared_ptr<IRegistryObserver>& observer) { list_observers_.erase(observer); }
		private:
			std::set<std::shared_ptr<IRegistryObserver>> list_observers_;
		};

		// Manage a registry in a static context
		class RegistryManager : public IRegistryObservable{
		public:
			// Build a registry and fill it in a static context
			RegistryManager(const std::string& label, registry_storage_t* tests_list, std::initializer_list<std::shared_ptr<test_t>> tests)
				: run_(false), registry_(label, tests_list)/*, exec_time_ms_accumulator_(duration_t{ 0 })*/ {
				for (auto& test : tests)
					push_back(test);
			}

			// Run all the tests
			void run_tests() {
				auto tests = registry_.get();
				for (auto&& test : tests) {
					test->run();
					//exec_time_ms_accumulator_ += test->getExecTimeMs();
					notify(tests_infos_t{ *test });
					switch (test->getStatus()) {
					case test_t::PASSED:
						tests_passed_.push_back(test.get());
						break;
					case test_t::FAILED:
						tests_failed_.push_back(test.get());
						break;
					case test_t::SKIPPED:
						tests_skipped_.push_back(test.get());
						break;
					case test_t::ERROR:
						tests_with_error_.push_back(test.get());
						break;
					default: break;
					}
				}
				run_ = true;
			}

			// Get informations
			const std::string& getLabel() const { return get()->getLabel(); }

			size_t getPassedCount() const { return run_ ? tests_passed_.size() : 0; }
			const std::vector<const test_t*>& getPassedTests() const { return tests_passed_; }

			size_t getFailedCount() const { return run_ ? tests_failed_.size() : 0; }
			const std::vector<const test_t*>& getFailedTests() const { return tests_failed_; }

			size_t getSkippedCount() const { return run_ ? tests_skipped_.size() : 0; }
			const std::vector<const test_t*>& getSkippedTests() const { return tests_skipped_; }

			size_t getWithErrorCount() const { return run_ ? tests_with_error_.size() : 0; }
			const std::vector<const test_t*>& getWithErrorTests() const { return tests_with_error_; }

			size_t getAllTestsCount() const { return run_ ? get()->get().size() : 0; }
			const registry_storage_t& getAllTests() const { return get()->get(); }
			duration_t getAllTestsExecTimeMs() const { return run_ ? exec_time_ms_accumulator_ : duration_t{ 0 }; }

		private:

			void push_back(const std::shared_ptr<test_t>& func) { registry_.get().push_back(std::move(func)); }
			Registry* get() { return &registry_; }
			const Registry* get() const { return &registry_; }

			bool run_;
			Registry registry_;
			duration_t exec_time_ms_accumulator_;
			std::vector<const test_t*> tests_passed_;
			std::vector<const test_t*> tests_failed_;
			std::vector<const test_t*> tests_skipped_;
			std::vector<const test_t*> tests_with_error_;

		};
	}


	/*
	*
	* Public interface
	*
	*/

	// Public accessible types
	using line_info_t = detail::LineInfo;
	using test_infos_t = detail::tests_infos_t;
	using registry_storage_t = detail::registry_storage_t;
	using registry_manager_t = detail::RegistryManager;
	using registry_observer_t = detail::IRegistryObserver;

	// Interface to Implement to access access a registry information
	// Possibility to export services into a DLL for forther customization
	class IRegistryTraversal {
	public:
		IRegistryTraversal(const registry_manager_t& registry) : registry_(registry) {}
		virtual ~IRegistryTraversal() {}
	protected:
		const registry_manager_t& getRegistryManager() const { return registry_; }
	protected:
		registry_manager_t registry_;
	};

	// Trivial impl for console display results
	class RegistryTraversal_ConsoleIO : private IRegistryTraversal {
	public:
		RegistryTraversal_ConsoleIO(const registry_manager_t& registry) : IRegistryTraversal{ registry } {}
		std::ostream& print(std::ostream& os, bool verbose) const {
			auto& registry_manager = getRegistryManager();
			os << "UNIT TEST SUMMARY [" << registry_.getLabel() << "] [" << registry_.getAllTestsExecTimeMs().count() << "ms] :" << std::endl;

			os << "\tPASSED:" << registry_.getPassedCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getPassedTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl;
				}
			}

			os << "\tFAILED:" << registry_.getFailedCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getFailedTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl
						<< "\t\t" << test->getFailureReason() << std::endl;
				}
			}

			os << "\tSKIPPED:" << registry_.getSkippedCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getSkippedTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl;
				}
			}

			os << "\tERRORS:" << registry_.getWithErrorCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getWithErrorTests()) {
					os << "\t\t[" << test->getLabel(verbose) << "] [" << test->getExecTimeMs().count() << "ms] " << test->getStatus() << std::endl
						<< "\t\t" << test->getError() << std::endl;
				}
			}

			return os;
		}
	};

	// Observer impl example
	class ConsoleIO_Observer : public registry_observer_t {
		virtual void update(const test_infos_t& infos) const {
			std::cout << (infos.test.getStatus() == test_infos_t::status_t::SKIPPED ? "SKIPPING TEST [" : "RUNNING TEST [")
				<< infos.test.getLabel(false) << "] [" << infos.test.getExecTimeMs().count() << "ms] :" << std::endl
				<< "Status : " << infos.test.getStatus() << std::endl;
		}
	};
	

	// Assert test class to help verbosing test logic into lambda's impl
	// Heavily inspired by the eponym MSVC's builtin assert class
	class Assert {
	public:

		// Verify that two objects are equal.
		template<typename T>
		static void AreEqual(const T& expected, const T& actual,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(expected == actual, message, lineInfo);
		}

		// double equality comparison:
		static void AreEqual(double expected, double actual, double tolerance,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			double diff = expected - actual;
			detail::FailOnCondition(fabs(diff) <= fabs(tolerance), message, lineInfo);
		}

		// float equality comparison:
		static void AreEqual(float expected, float actual, float tolerance,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			float diff = expected - actual;
			detail::FailOnCondition(fabs(diff) <= fabs(tolerance), message, lineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(const char* expected, const char* actual, bool ignoreCase = false,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			AreEqual(std::string{ expected }, std::string{ actual }, ignoreCase, message, lineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(std::string expected, std::string actual, bool ignoreCase = false,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			if (ignoreCase) {
				std::transform(expected.begin(), expected.end(), expected.begin(), ::tolower);
				std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
			}
			detail::FailOnCondition(expected == actual, message, lineInfo);
		}

		// Verify that two references refer to the same object instance (identity):
		template<typename T>
		static void AreSame(const T& expected, const T& actual,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(&expected == &actual, message, lineInfo);
		}

		// Generic AreNotEqual comparison:
		template<typename T> static void AreNotEqual(const T& notExpected, const T& actual,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(!(notExpected == actual), message, lineInfo);
		}

		// double AreNotEqual comparison:
		static void AreNotEqual(double notExpected, double actual, double tolerance,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			double diff = notExpected - actual;
			detail::FailOnCondition(fabs(diff) > fabs(tolerance), message, lineInfo);
		}

		// float AreNotEqual comparison:
		static void AreNotEqual(float notExpected, float actual, float tolerance,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			float diff = notExpected - actual;
			detail::FailOnCondition(fabs(diff) > fabs(tolerance), message, lineInfo);
		}

		// char* string AreNotEqual comparison:
		static void AreNotEqual(const char* notExpected, const char* actual, bool ignoreCase = false,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			AreNotEqual(std::string{ notExpected }, std::string{ actual }, ignoreCase, message, lineInfo);
		}

		// wchar_t* string AreNotEqual comparison with char* message:
		static void AreNotEqual(std::string notExpected, std::string actual, bool ignoreCase = false,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			if (ignoreCase) {
				std::transform(notExpected.begin(), notExpected.end(), notExpected.begin(), ::tolower);
				std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
			}
			detail::FailOnCondition(notExpected != actual, message, lineInfo);
		}

		// Verify that two references do not refer to the same object instance (identity):
		template<typename T>
		static void AreNotSame(const T& notExpected, const T& actual,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(!(&notExpected == &actual), message, lineInfo);
		}

		// Verify that a pointer is NULL:
		template<typename T> static void IsNull(const T* actual,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(actual == nullptr, message, lineInfo);
		}

		// Verify that a pointer is not NULL:
		template<typename T> static void IsNotNull(const T* actual,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(actual != nullptr, message, lineInfo);
		}

		// Verify that a condition is true:
		static void IsTrue(bool condition,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(condition, message, lineInfo);
		}

		// Verify that a conditon is false:
		static void IsFalse(bool condition,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(!condition, message, lineInfo);
		}

		// Force the test case result to be Failed:
		static void Fail(const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			detail::FailOnCondition(false, message, lineInfo);
		}

		// Verify that a function raises an exception:
		template<typename ExpectedException, typename Functor>
		static void ExpectException(Functor functor,
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			try {
				functor();
			}
			catch (ExpectedException) {
				return;
			}
			catch (...) {
				Assert::Fail(message, pLineInfo);
			}
		}

		template<typename ExpectedException, typename ReturnType>
		static void ExpectException(ReturnType(*func)(),
			const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
			Assert::IsNotNull(func, message, lineInfo);

			try {
				func();
			}
			catch (ExpectedException) {
				return;
			}
			catch (...) {
				Assert::Fail(message, pLineInfo);
			}
		}
	};
}

//Helper macros to use the unit test suit
#define register_scenario(name, description, ...) \
	static H2OFastTests::registry_storage_t tests_list_ ## name; \
	static H2OFastTests::registry_manager_t registry_manager_ ## name {description, &tests_list_ ## name, { __VA_ARGS__ } };
#define register_observer(name, observer_class) \
	registry_manager_ ## name.addObserver(std::make_shared<observer_class>());
#define describe_test(test) H2OFastTests::detail::make_test((test))
#define describe_test_label(label, test) H2OFastTests::detail::make_test(label, (test))
#define skip_test(test) H2OFastTests::detail::make_skipped_test((test))
#define skip_test_reason(reason, test) H2OFastTests::detail::make_skipped_test(reason, (test))
#define run_scenario(name) registry_manager_ ## name.run_tests();
#define print_result(name, verbose) H2OFastTests::RegistryTraversal_ConsoleIO(registry_manager_ ## name).print(std::cout, (verbose))
#define line_info() &H2OFastTests::line_info_t(__FILE__, "", __LINE__)
#define line_info_f() &H2OFastTests::line_info_t(__FILE__, __FUNCTION__, __LINE__)

#endif