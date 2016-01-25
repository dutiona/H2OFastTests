#pragma once

#ifndef H2OFASTTESTS_H
#define H2OFASTTESTS_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace H2OFastTests {

	// Implementation details

	namespace detail {
		struct LineInfo {
			const std::string file_;
			const std::string func_;
			const int line_;
			LineInfo(const char* file, const char* func, int line)
				: file_(file), func_(func), line_(line)
			{}
		};

		std::ostream& operator<<(std::ostream& oss, const LineInfo& lineInfo) {
			oss << lineInfo.file_ << ":" << lineInfo.line_ << " " << lineInfo.func_;
			return oss;
		}

		class TestFailure : public std::exception {
		public:
			TestFailure(std::string message) : message_(message) {}
			virtual const char * what() const { return message_.c_str(); }
		private:
			const std::string message_;
		};

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

		class Test {
		public:

			enum Status {
				PASSED, FAILED, ERROR, SKIPPED, NONE
			};

			Test() : Test({}, [](){}) {}
			Test(const test_func_t&& test) : Test("", std::move(test)) {}
			Test(const std::string& label) : Test(label, [](){}) {}
			Test(const std::string& label, const test_func_t&& test)
				: label_(label), status_(NONE), test_holder_(std::make_shared<test_func_t>(std::move(test)))
			{}

			// Default move impl for VC2013

			Test(Test&& test)
				: test_holder_(std::make_shared<test_func_t>(std::move(*test.test_holder_))),
				label_(test.label_), status_(test.status_),
				failure_reason_(test.failure_reason_), error_(test.error_)
			{}

			Test&& operator=(Test&& test) {
				test_holder_ = std::make_shared<test_func_t>(std::move(*test.test_holder_));
				label_ = test.label_;
				status_ = test.status_;
				failure_reason_ = test.failure_reason_;
				error_ = test.error_;
				return std::move(*this);
			}

			virtual ~Test() {}

			// NVI

			void run() { run_private(); }

			const std::string getLabel(bool verbose) const { return getLabel_private(verbose); }
			const std::string getFailureReason() const { return getFailureReason_private(); }
			const std::string getSkippedReason() const { return getSkippedReason_private(); }
			const std::string getError() const { return getError_private(); }
			Status getStatus() const { return getStatus_private(); }

		private:

			virtual void run_private() {
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
			}

			virtual const std::string getLabel_private(bool /*verbose*/) const { return label_; }
			virtual const std::string getFailureReason_private() const { return failure_reason_; }
			virtual const std::string getSkippedReason_private() const { return skipped_reason_; }
			virtual const std::string getError_private() const { return error_; }
			virtual Status getStatus_private() const { return status_; }

		protected:

			std::shared_ptr<test_func_t> test_holder_;
			std::string label_;
			std::string failure_reason_;
			std::string skipped_reason_;
			std::string error_;
			Status status_;

		};

		using test_t = detail::Test;

		class SkippedTest : public Test {
		public:

			SkippedTest(Test&& test) : Test{ std::move(test) } {}
			SkippedTest(const std::string& reason, Test&& test)
				: Test{ std::move(test) } {
				skipped_reason_ = reason;
			}

		private:

			virtual void run_private() { status_ = SKIPPED; }

		};

		using skipped_test_t = detail::SkippedTest;

		// Helper functions to build/skip a test case
		std::shared_ptr<test_t> make_test(test_func_t&& func) { return std::make_shared<test_t>(std::move(func)); }
		std::shared_ptr<test_t> make_test(const std::string& label, test_func_t&& func) { return std::make_shared<test_t>(label, std::move(func)); }
		std::shared_ptr<test_t> make_test(std::shared_ptr<test_t>&& test) { return test; }
		std::shared_ptr<test_t> skip_test(test_t&& test) { return std::make_shared<skipped_test_t>(std::move(test)); }
		std::shared_ptr<test_t> skip_test(const std::string& reason, test_t&& test) { return std::make_shared<skipped_test_t>(reason, std::move(test)); }

		using registry_storage_t = std::vector<std::shared_ptr<test_t>>;

		// Registry main impl
		class Registry {
		public:
			Registry(const std::string& label, registry_storage_t* tests_list) : label_(label), tests_list_(tests_list) {}
			registry_storage_t& get() {	return *tests_list_; };
			const registry_storage_t& get() const { return const_cast<Registry*>(this)->get(); };
			const std::string& getLabel() const { return label_; }
		private:
			std::string label_;
			registry_storage_t* tests_list_;
		};

		//Building a registry and filling it in a static context
		class RegistryManager {
		public:
			RegistryManager(const std::string& label, registry_storage_t* tests_list, std::initializer_list<std::shared_ptr<test_t>> tests)
				: run_(false), registry_(label, tests_list) {
				for (auto& test : tests)
					push_back(test);
			}

			void push_back(const std::shared_ptr<test_t>& func) {
				registry_.get().push_back(std::move(func));
			}

			Registry* get() { return &registry_; }
			const Registry* get() const { return &registry_; }

			void run_tests() {
				auto tests = registry_.get();
				for (auto&& test : tests) {
					test->run();
					switch (test->getStatus()) {
					case test_t::PASSED: testsPassed_.push_back(test.get());   break;
					case test_t::FAILED: testsFailed_.push_back(test.get());   break;
					case test_t::SKIPPED:testsSkipped_.push_back(test.get());  break;
					case test_t::ERROR:  testsWithError_.push_back(test.get());break;
					default: break;
					}
				}
				run_ = true;
			}

			size_t getPassedCount() const { return run_ ? testsPassed_.size() : 0; }
			const std::vector<const test_t*>& getPassedTests() const { return testsPassed_; }

			size_t getFailedCount() const { return run_ ? testsFailed_.size() : 0; }
			const std::vector<const test_t*>& getFailedTests() const { return testsFailed_; }

			size_t getSkippedCount() const { return run_ ? testsSkipped_.size() : 0; }
			const std::vector<const test_t*>& getSkippedTests() const { return testsSkipped_; }

			size_t getWithErrorCount() const { return run_ ? testsWithError_.size() : 0; }
			const std::vector<const test_t*>& getWithErrorTests() const { return testsWithError_; }

			size_t getAllTestsCount() const { return run_ ? get()->get().size() : 0; }
			const registry_storage_t& getAllTests() const { return get()->get(); }

		private:

			bool run_;
			Registry registry_;
			std::vector<const test_t*> testsPassed_;
			std::vector<const test_t*> testsFailed_;
			std::vector<const test_t*> testsSkipped_;
			std::vector<const test_t*> testsWithError_;

		};
	}


	/*
	*
	* Public interface
	*
	*/

	using line_info_t = detail::LineInfo;
	using registry_manager_t = detail::RegistryManager;

	//Interface to Implement to access access a registry information
	//Possibility to export services into a DLL for forther customization
	class IRegistryTraversal {
	public:
		IRegistryTraversal(const registry_manager_t& registry) : registry_(registry) {}
		~IRegistryTraversal() {}
	protected:
		const registry_manager_t& getRegistryManager() const { return registry_; }
	protected:
		registry_manager_t registry_;
	};

	// Trivial impl for console display results
	class RegistryTraversal_ConsoleIO : IRegistryTraversal {
	public:
		RegistryTraversal_ConsoleIO(const registry_manager_t& registry) : IRegistryTraversal{ registry } {}
		std::ostream& print(std::ostream& oss, bool verbose) const {
			auto& registry_manager = getRegistryManager();
			oss << "[" << registry_.get()->getLabel() << "] UNIT TEST SUMMARY:" << std::endl;

			oss << "\tPASSED:" << registry_.getPassedCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getPassedTests()){
					oss << "\t\t[" << test->getLabel(verbose) << "] PASSED" << std::endl;
				}
			}

			oss << "\tFAILED:" << registry_.getFailedCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getFailedTests()){
					oss << "\t\t[" << test->getLabel(verbose) << "] FAILED: " << test->getFailureReason() << std::endl;
				}
			}

			oss << "\tSKIPPED:" << registry_.getSkippedCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getSkippedTests()){
					oss << "\t\t[" << test->getLabel(verbose) << "] SKIPPED" << std::endl;
				}
			}

			oss << "\tERRORS:" << registry_.getWithErrorCount() << "/" << registry_.getAllTestsCount() << std::endl;
			if (verbose) {
				for (const auto test : registry_.getWithErrorTests()){
					oss << "\t\t[" << test->getLabel(verbose) << "] WITH ERROR: " << test->getError() << std::endl;
				}
			}

			return oss;
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
	static H2OFastTests::detail::registry_storage_t tests_list_ ## name; \
	static H2OFastTests::detail::RegistryManager registry_manager_ ## name {description, &tests_list_ ## name, { __VA_ARGS__ } };
#define describe_test(test) H2OFastTests::detail::make_test((test))
#define describe_test_label(description, test) H2OFastTests::detail::make_test(description, (test))
#define run_scenario(name) registry_manager_ ## name.run_tests();
#define print_result(name, verbose) H2OFastTests::RegistryTraversal_ConsoleIO(registry_manager_ ## name).print(std::cout, verbose)
#define line_info() &H2OFastTests::line_info_t(__FILE__, "", __LINE__)
#define line_info_f() &H2OFastTests::line_info_t(__FILE__, __FUNCTION__, __LINE__)

#endif