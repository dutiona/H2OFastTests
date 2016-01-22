#pragma once

#ifndef UNITTESTTOOL_H
#define UNITTESTTOOL_H

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace UnitTestTool {

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

		static std::ostream* default_oss = &std::cout;
		static std::istream* default_iss = &std::cin;
		static std::ostream* default_err = &std::cerr;
		static bool verbose = false;

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
				if (lineInfo != nullptr){
					oss << *lineInfo << std::endl << ((message != nullptr) ? message : "");
				}
				else{
					oss << ((message != nullptr) ? message : "");
				}
				throw TestFailure(oss.str());
			}
		}

		using test_func_t = std::function<void(void)>;

		class Test {
		public:

			enum Status {
				PASSED, FAILED, ERROR, SKIPPED, UNIT, NONE
			};

			Test() : Test({}, [](){}) {}
			Test(const test_func_t&& test) : Test("", std::move(test)) {}
			Test(const std::string& label) : Test(label, [](){}) {}
			Test(const std::string& label, const test_func_t&& test)
				: label_(label), status_(NONE), test_holder_(std::make_shared<test_func_t>(std::move(test)))
			{}

			//Move allowed
			Test(Test&& test)
				: test_holder_(test.test_holder_),
				label_(test.label_), status_(test.status_),
				failure_reason_(test.failure_reason_), error_(test.error_)
			{}
			Test&& operator=(Test&& test) {
				test_holder_ = test.test_holder_;
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

			size_t getPassedCount() const { return getPassedCount_private(); }
			size_t getFailedCount() const { return getFailedCount_private(); }
			size_t getSkippedCount() const { return getSkippedCount_private(); }
			size_t getErrorCount() const { return getErrorCount_private(); }

		private:

			// Solo test Impl

			virtual void run_private() {
				try {
					(*test_holder_)(); // Effective test call
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

			virtual size_t getPassedCount_private() const { return static_cast<size_t>(status_ == PASSED); }
			virtual size_t getFailedCount_private() const { return static_cast<size_t>(status_ == FAILED); }
			virtual size_t getSkippedCount_private() const { return static_cast<size_t>(status_ == SKIPPED); }
			virtual size_t getErrorCount_private() const { return static_cast<size_t>(status_ == ERROR); }

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

		class UnitTest : public Test {
		public:

			UnitTest(std::vector<test_t>&& tests)
				: Test{ } {
				std::for_each(make_move_iterator(tests.begin()), make_move_iterator(tests.end()), [this](test_t&& test){
					tests_.push_back(std::move(std::make_shared<test_t>(std::move(test))));
				});
			}

			UnitTest(const std::string& unit_label, std::vector<test_t>&& tests)
				: Test{ unit_label } {
				std::for_each(make_move_iterator(tests.begin()), make_move_iterator(tests.end()), [this](test_t&& test){
					tests_.push_back(std::make_shared<test_t>(std::move(test)));
				});
			}

		private:

			virtual void run_private() {
				status_ = UNIT;
				for (auto&& test : tests_) {
					test->run();
					switch (test->getStatus()) {
					case PASSED: testsPassed_.push_back(test.get());   break;
					case FAILED: testsFailed_.push_back(test.get());   break;
					case SKIPPED:testsSkipped_.push_back(test.get());  break;
					case ERROR:  testsWithError_.push_back(test.get()); break;
					case UNIT:   *default_err << "nested unit test not implemented yet" << std::endl; break;
					default: break;
					}
				}
			}

			virtual const std::string getLabel_private(bool verbose) const {
				std::ostringstream oss;
				oss << label_ << " UNIT TEST SUMMARY:" << std::endl;

				oss << "\tPASSED:" << getPassedCount() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsPassed_){
						oss << "\t\t" << test->getLabel(verbose) << " PASSED" << std::endl;
					}
				}

				oss << "\tFAILED:" << getFailedCount() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsFailed_){
						oss << "\t\t" << test->getLabel(verbose) << " FAILED: " << test->getFailureReason() << std::endl;
					}
				}

				oss << "\tSKIPPED:" << getSkippedCount() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsSkipped_){
						oss << "\t\t" << test->getLabel(verbose) << " SKIPPED" << std::endl;
					}
				}

				oss << "\tERRORS:" << getErrorCount() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsWithError_){
						oss << "\t\t" << test->getLabel(verbose) << " WITH ERROR: " << test->getError() << std::endl;
					}
				}

				return oss.str();
			}

			virtual size_t getPassedCount_private() const { return testsPassed_.size(); }
			virtual size_t getFailedCount_private() const { return testsFailed_.size(); }
			virtual size_t getSkippedCount_private() const { return testsSkipped_.size(); }
			virtual size_t getErrorCount_private() const { return testsWithError_.size(); }

		private:

			std::vector<std::shared_ptr<test_t>> tests_;

			std::vector<Test*> testsPassed_;
			std::vector<Test*> testsFailed_;
			std::vector<Test*> testsSkipped_;
			std::vector<Test*> testsWithError_;
		};

		using unit_test_t = detail::UnitTest;

		std::vector<test_t>& registry() {
			static std::vector<test_t> tests_list{};
			return tests_list;
		};

		struct UnitTestTool_registrar {
			UnitTestTool_registrar(test_t&& func)	{
				registry().push_back(std::move(func));
			}
		};
	}


	/*
	*
	* Public interface
	*
	*/

	static void setOss(std::ostream* oss){ detail::default_oss = oss; }
	static void setIss(std::istream* iss){ detail::default_iss = iss; }
	static void setErr(std::ostream* err){ detail::default_err = err; }
	static void setVerbose(bool verbose) { detail::verbose = verbose; }

	using test_t = detail::test_t;
	using unit_test_t = detail::unit_test_t;
	using skipped_test_t = detail::skipped_test_t;
	//using test_ptr = detail::test_ptr;
	using test_func_t = detail::test_func_t;
	
	//test_ptr&& make_test(test_func_t&& func){ return std::make_unique<test_t>(std::move(func)); }
	test_t&& make_test(std::string label, test_func_t&& func){ return std::move(test_t(label, std::move(func))); }
	test_t&& skip_test(test_t&& test){ return std::move(skipped_test_t(std::move(test))); }
	test_t&& skip_test(std::string reason, test_t&& test){ return std::move(skipped_test_t(reason, std::move(test))); }
	template<typename ... Args>
	test_t&& unit_test(Args ... args){ return std::move(unit_test_t(std::move(tests))); }
	test_t&& unit_test(std::string label, std::vector<test_t>&& tests){ return std::move(unit_test_t(label, std::move(tests))); }
	/*
	test_ptr unit_test(const std::string& label, std::vector<test_t>&& tests){
		auto&& tests_ptr = std::vector<test_ptr>{};
		std::for_each(make_move_iterator(tests.begin()), make_move_iterator(tests.end()), [&tests_ptr](test_t&& test){
			tests_ptr.push_back(std::make_unique<test_t>(std::move(test)));
		});
		return std::make_unique<unit_test_t>(label, std::move(tests_ptr));
	}
	test_ptr unit_test(std::vector<test_t>&& tests){
		auto&& tests_ptr = std::vector<test_ptr>{};
		std::for_each(make_move_iterator(tests.begin()), make_move_iterator(tests.end()), [&tests_ptr](test_t&& test){
			tests_ptr.push_back(std::make_unique<test_t>(std::move(test)));
		});
		return std::make_unique<unit_test_t>(std::move(tests_ptr));
	}
	*/
	/*
	test_ptr unit_test(const std::string& label, std::vector<test_func_t>&& tests){
		auto tests_ptr = std::vector<test_ptr>{};
		std::for_each(make_move_iterator(tests.begin()), make_move_iterator(tests.end()), [&tests_ptr](test_func_t&& test){
			tests_ptr.push_back(make_test(std::move(test)));
		});
		return std::make_unique<unit_test_t>(label, std::move(tests_ptr));
	}
	test_ptr unit_test(std::vector<test_func_t>&& tests){
		auto tests_ptr = std::vector<test_ptr>{};
		std::for_each(make_move_iterator(tests.begin()), make_move_iterator(tests.end()), [&tests_ptr](test_func_t&& test){
			tests_ptr.push_back(make_test(std::move(test)));
		});
		return std::make_unique<unit_test_t>(std::move(tests_ptr));
	}
	*/

#define register_test(label, unit_test_func) \
	static UnitTestTool::detail::UnitTestTool_registrar UnitTestTool_registrar(UnitTestTool::make_test(label, unit_test_func));
	
#define register_tests(label, unit_test_func_list) \
	static UnitTestTool::detail::UnitTestTool_registrar UnitTestTool_registrar(UnitTestTool::unit_test(label, std::move(unit_test_func_list)));

	void run_tests() { for (auto&& test : detail::registry()) test.run(); }
	void display_results(bool verbose = false) { for (auto&& test : detail::registry()) *detail::default_oss << test.getLabel(verbose); }
	void display_results_err(bool verbose = false) { for (auto&& test : detail::registry()) *detail::default_err << test.getLabel(verbose); }
	void run_and_display(bool verbose = false) { run_tests(); display_results(verbose); }

#define LINE_INFO() &UnitTestTool::detail::LineInfo(__FILE__, "", __LINE__)

	class Assert
	{
	public:

		// Verify that two objects are equal.
		template<typename T>
		static void AreEqual(const T& expected, const T& actual,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(expected == actual, message, lineInfo);
		}

		// double equality comparison:
		static void AreEqual(double expected, double actual, double tolerance,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			double diff = expected - actual;
			FailOnCondition(fabs(diff) <= fabs(tolerance), message, lineInfo);
		}

		// float equality comparison:
		static void AreEqual(float expected, float actual, float tolerance,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			float diff = expected - actual;
			FailOnCondition(fabs(diff) <= fabs(tolerance), message, lineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(const char* expected, const char* actual, bool ignoreCase = false,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			AreEqual(std::string{ expected }, std::string{ actual }, ignoreCase, message, lineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(std::string expected, std::string actual, bool ignoreCase = false,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			if (ignoreCase){
				std::transform(expected.begin(), expected.end(), expected.begin(), ::tolower);
				std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
			}
			FailOnCondition(expected == actual, message, lineInfo);
		}

		// Verify that two references refer to the same object instance (identity):
		template<typename T>
		static void AreSame(const T& expected, const T& actual,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(&expected == &actual, message, lineInfo);
		}

		// Generic AreNotEqual comparison:
		template<typename T> static void AreNotEqual(const T& notExpected, const T& actual,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(!(notExpected == actual), message, lineInfo);
		}

		// double AreNotEqual comparison:
		static void AreNotEqual(double notExpected, double actual, double tolerance,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			double diff = notExpected - actual;
			FailOnCondition(fabs(diff) > fabs(tolerance), message, lineInfo);
		}

		// float AreNotEqual comparison:
		static void AreNotEqual(float notExpected, float actual, float tolerance,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			float diff = notExpected - actual;
			FailOnCondition(fabs(diff) > fabs(tolerance), message, lineInfo);
		}

		// char* string AreNotEqual comparison:
		static void AreNotEqual(const char* notExpected, const char* actual, bool ignoreCase = false,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			AreNotEqual(std::string{ notExpected }, std::string{ actual }, ignoreCase, message, lineInfo);
		}

		// wchar_t* string AreNotEqual comparison with char* message:
		static void AreNotEqual(std::string notExpected, std::string actual, bool ignoreCase = false,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			if (ignoreCase){
				std::transform(notExpected.begin(), notExpected.end(), notExpected.begin(), ::tolower);
				std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
			}
			FailOnCondition(notExpected != actual, message, lineInfo);
		}


		// Verify that two references do not refer to the same object instance (identity):
		template<typename T>
		static void AreNotSame(const T& notExpected, const T& actual,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(!(&notExpected == &actual), message, lineInfo);
		}

		// Verify that a pointer is NULL:
		template<typename T> static void IsNull(const T* actual,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(actual == nullptr, message, lineInfo);
		}

		// Verify that a pointer is not NULL:
		template<typename T> static void IsNotNull(const T* actual,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(actual != nullptr, message, lineInfo);
		}

		// Verify that a condition is true:
		static void IsTrue(bool condition,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(condition, message, lineInfo);
		}

		// Verify that a conditon is false:
		static void IsFalse(bool condition,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(!condition, message, lineInfo);
		}

		// Force the test case result to be Failed:
		static void Fail(const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(false, message, lineInfo);
		}


		// Verify that a function raises an exception:
		template<typename ExpectedException, typename Functor>
		static void ExpectException(Functor functor,
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			try{
				functor();
			}
			catch (ExpectedException){
				return;
			}
			catch (...){
				Assert::Fail(message, pLineInfo);
			}
		}

		template<typename ExpectedException, typename ReturnType>
		static void ExpectException(ReturnType(*func)(),
			const char* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			Assert::IsNotNull(func, message, lineInfo);

			try{
				func();
			}
			catch (ExpectedException){
				return;
			}
			catch (...){
				Assert::Fail(message, pLineInfo);
			}
		}
	};

}

#endif