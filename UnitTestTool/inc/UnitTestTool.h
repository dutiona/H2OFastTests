#pragma once

#ifndef UNITTESTTOOL_H
#define UNITTESTTOOL_H

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace UnitTestTool {

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

		void FailOnCondition(bool condition, const std::string* message = nullptr, const LineInfo* lineInfo = nullptr) {
			if (!condition) {
				std::ostringstream oss;
				oss << *lineInfo << std::endl << ((message != nullptr) ? *message : "");
				throw TestFailure(oss.str());
			}
		}

		class Test {
		public:
			enum Status {
				PASSED, FAILED, ERROR, SKIPPED, NONE
			};

			Test() = delete;

			Test(const std::string& label, const std::function<void(void)>& test)
				: label_(label), status_(ERROR) {
				try {
					test();
					status_ = PASSED;
				}
				catch (const TestFailure& failure) {
					status_ = FAILED;
					failureReason_ = failure.what();
				}
				catch (const std::exception& e) {
					status_ = ERROR;
					error_ = e.what();
				}
			}

			virtual ~Test() {}

			virtual const std::string& getLabel() const {
				return label_;
			}

			virtual const std::string& getFailureReason() const {
				return failureReason_;
			}

			virtual const std::string& getError() const {
				return error_;
			}

			virtual Status getStatus() const {
				return status_;
			}

		protected:
			const std::string label_;
			std::string failureReason_;
			std::string error_;
			Status status_;
		};

		class SkippedTest : public Test {
		public:

			SkippedTest(const std::string& label, const std::function<void(void)>& /*test*/)
				: Test{ label, [](){} } {
				status_ = SKIPPED;
			}

		};

		class TestUnit : public Test {
		public:

			TestUnit(const std::string& label, std::initializer_list<Test> tests)
				: Test{ label, [](){} }, tests_(tests) {
				status_ = NONE;
				for (auto& test : tests_){
					switch (test.getStatus()){
					case PASSED: testsPassed_.push_back(&test);   break;
					case FAILED: testsFailed_.push_back(&test);   break;
					case SKIPPED:testsSkipped_.push_back(&test);  break;
					case ERROR:  testsWithError_.push_back(&test);break;
					default: break;
					}
				}
			}

			virtual const std::string& getLabel(bool verbose) const {
				std::ostringstream oss;
				oss << label_ << " UNIT TEST SUMMARY:" << std::endl;

				oss << "\tPASSED:" << testsPassed_.size() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsPassed_){
						oss << "\t\t" << test->getLabel() << " PASSED" << std::endl;
					}
				}

				oss << "\tFAILED:" << testsFailed_.size() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsFailed_){
						oss << "\t\t" << test->getLabel() << " FAILED: " << test->getFailureReason() << std::endl;
					}
				}

				oss << "\tSKIPPED:" << testsSkipped_.size() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsSkipped_){
						oss << "\t\t" << test->getLabel() << " SKIPPED" << std::endl;
					}
				}

				oss << "\tERRORS:" << testsWithError_.size() << "/" << tests_.size() << std::endl;
				if (verbose){
					for (const auto test : testsWithError_){
						oss << "\t\t" << test->getLabel() << " WITH ERROR: " << test->getError() << std::endl;
					}
				}

				return oss.str();
			}

		private:

			std::vector<Test> tests_;

			std::vector<Test*> testsPassed_;
			std::vector<Test*> testsFailed_;
			std::vector<Test*> testsSkipped_;
			std::vector<Test*> testsWithError_;
		};
	}


	/*
	*
	* Public interface
	*
	*/

	detail::Test test(const std::string& label, const std::function<void(void)>& test){
		return detail::Test{ label, test };
	}

	detail::Test skip_test(const std::string& label, const std::function<void(void)>& test){
		return detail::SkippedTest{ label, test };
	}

	void test_unit(const std::string& label, std::initializer_list<detail::Test> tests){
		*detail::default_oss << detail::TestUnit{ label, tests }.getLabel(detail::verbose);
	}

#define LINE_INFO() detail::LineInfo(__FILE__, __FUNCTION__, __LINE__)

	static void setOss(std::ostream* oss){ detail::default_oss = oss; }
	static void setIss(std::istream* iss){ detail::default_iss = iss; }
	static void setErr(std::ostream* err){ detail::default_err = err; }
	static void setVerbose(bool verbose) { detail::verbose = verbose; }

	class Assert
	{
	public:

		// Verify that two objects are equal.
		template<typename T>
		static void AreEqual(const T& expected, const T& actual,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(expected == actual, message, lineInfo);
		}

		// double equality comparison:
		static void AreEqual(double expected, double actual, double tolerance,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			double diff = expected - actual;
			FailOnCondition(fabs(diff) <= fabs(tolerance), message, lineInfo);
		}

		// float equality comparison:
		static void AreEqual(float expected, float actual, float tolerance,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			float diff = expected - actual;
			FailOnCondition(fabs(diff) <= fabs(tolerance), message, lineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(const char* expected, const char* actual, bool ignoreCase = false,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			AreEqual(std::string{ expected }, std::string{ actual }, ignoreCase, message, lineInfo);
		}

		// char* string equality comparison:
		static void AreEqual(const std::string& expected, const std::string& actual, bool ignoreCase = false,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			if (ignoreCase){
				std::transform(expected.begin(), expected.end(), expected.begin(), ::tolower);
				std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
			}
			FailOnCondition(expected == actual, message, lineInfo);
		}

		// Verify that two references refer to the same object instance (identity):
		template<typename T>
		static void AreSame(const T& expected, const T& actual,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(&expected == &actual, message, lineInfo);
		}

		// Generic AreNotEqual comparison:
		template<typename T> static void AreNotEqual(const T& notExpected, const T& actual,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(!(notExpected == actual), message, lineInfo);
		}

		// double AreNotEqual comparison:
		static void AreNotEqual(double notExpected, double actual, double tolerance,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			double diff = notExpected - actual;
			FailOnCondition(fabs(diff) > fabs(tolerance), message, lineInfo);
		}

		// float AreNotEqual comparison:
		static void AreNotEqual(float notExpected, float actual, float tolerance,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			float diff = notExpected - actual;
			FailOnCondition(fabs(diff) > fabs(tolerance), message, lineInfo);
		}

		// char* string AreNotEqual comparison:
		static void AreNotEqual(const char* notExpected, const char* actual, bool ignoreCase = false,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			AreNotEqual(std::string{ notExpected }, std::string{ actual }, ignoreCase, message, lineInfo);
		}

		// wchar_t* string AreNotEqual comparison with char* message:
		static void AreNotEqual(const std::string& notExpected, const std::string& actual, bool ignoreCase = false,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			if (ignoreCase){
				std::transform(notExpected.begin(), notExpected.end(), notExpected.begin(), ::tolower);
				std::transform(actual.begin(), actual.end(), actual.begin(), ::tolower);
			}
			FailOnCondition(notExpected != actual, message, lineInfo);
		}


		// Verify that two references do not refer to the same object instance (identity):
		template<typename T>
		static void AreNotSame(const T& notExpected, const T& actual,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(!(&notExpected == &actual), message, lineInfo);
		}

		// Verify that a pointer is NULL:
		template<typename T> static void IsNull(const T* actual,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(actual == nullptr, message, lineInfo);
		}

		// Verify that a pointer is not NULL:
		template<typename T> static void IsNotNull(const T* actual,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(actual != nullptr, message, lineInfo);
		}

		// Verify that a condition is true:
		static void IsTrue(bool condition,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(condition, message, lineInfo);
		}

		// Verify that a conditon is false:
		static void IsFalse(bool condition,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(!condition, message, lineInfo);
		}

		// Force the test case result to be Failed:
		static void Fail(const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
			FailOnCondition(false, message, lineInfo);
		}


		// Verify that a function raises an exception:
		template<typename ExpectedException, typename Functor>
		static void ExpectException(Functor functor,
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
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
			const std::string* message = nullptr, const detail::LineInfo* lineInfo = nullptr){
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