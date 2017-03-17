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
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "H2OFastTests_config.hpp"

namespace H2OFastTests {
// Implementation details
namespace detail {

    template<class Type>
    struct type_helper {
        using type = Type;
        static bool before(const std::type_info& rhs) { return typeid(Type).before(rhs); }
        static const char*                       raw_name() { return typeid(Type).raw_name(); }
        static const char*                       name() { return typeid(Type).name(); }
        static size_t                            hash_code() { return typeid(Type).hash_code(); }
        static std::type_index                   type_index() { return std::type_index(typeid(Type)); }
    };

    // Line info struct
    // Holds line number, file name and function name if relevant
    class LineInfo
    {
        public:
        LineInfo() : init_(false) {}

        LineInfo(const char* file, const char* func, int line) : file_(file), func_(func), line_(line), init_(true) {}

        ~LineInfo() {}

        bool isInit() const { return init_; }

        // Basic display
        friend std::ostream& operator<<(std::ostream& os, const LineInfo& lineInfo) {
            if (lineInfo.isInit()) os << lineInfo.file_ << ":" << lineInfo.line_ << " " << lineInfo.func_;
            return os;
        }

        private:
        std::string file_;
        std::string func_;
        int         line_;
        bool        init_;
    };

    // Internal exception raised when a test failed
    // Used by internal test runner and assert tool to communicate over the test
    template<class S, class T>
    class is_streamable
    {
        template<class SS, class TT>
        static auto test(int) -> decltype(std::declval<SS&>() << std::declval<TT>(), std::true_type());

        template<class, class>
        static auto test(...) -> std::false_type;

        public:
        static const bool value = decltype(test<S, T>(0))::value;
    };

    enum class FailureType { equal, different, exception };

    template<bool Streamable, bool Exception>
    struct additionalInfos {
        template<class ValueTypeL, class ValueTypeR>
        static std::string get(FailureType failure_type, ValueTypeL reached, ValueTypeR expected,
                               const std::string& exception_name);
    };

    template<>
    struct additionalInfos<true, false> {
        template<class ValueTypeL, class ValueTypeR>
        static std::string get(FailureType failure_type, ValueTypeL reached, ValueTypeR expected,
                               const std::string&) noexcept {
            std::ostringstream oss;
            oss << "\t\t\t[REACHED] " << reached << std::endl;
            switch (failure_type) {
                case FailureType::equal: {
                    oss << "\t\t\t[EXPECTED EQUAL TO] " << expected << std::endl;
                    break;
                }
                case FailureType::different: {
                    oss << "\t\t\t[EXPECTED DIFFERENT FROM] " << expected << std::endl;
                    break;
                }
                case FailureType::exception:
                default: {
                    oss << "\t\t\t[ERROR] " << std::endl;
                    break;
                }
            }
            return oss.str();
        }
    };

    template<>
    struct additionalInfos<false, false> {
        template<class ValueTypeL, class ValueTypeR>
        static std::string get(FailureType failure_type, ValueTypeL, ValueTypeR, const std::string&) noexcept {
            std::ostringstream oss;
            switch (failure_type) {
                case FailureType::equal: {
                    oss << "\t\t\t[REACHED] is different from [EXPECTED]. Expected [EQUAL TO]" << std::endl;
                    break;
                }
                case FailureType::different: {
                    oss << "\t\t\t[REACHED] is equal to [EXPECTED]. Expected [DIFFERENT FROM]" << std::endl;
                    break;
                }
                case FailureType::exception:
                default: {
                    oss << "\t\t\t[ERROR] " << std::endl;
                    break;
                }
            }
            return oss.str();
        }
    };

    template<bool Streamable>
    struct additionalInfos<Streamable, true> {
        template<class ValueTypeL, class ValueTypeR>
        static std::string get(FailureType        failure_type, ValueTypeL, ValueTypeR,
                               const std::string& exception_name) noexcept {
            std::ostringstream oss;
            switch (failure_type) {
                case FailureType::exception: {
                    oss << "\t\t[EXPECTED Exception] " << exception_name << std::endl;
                    break;
                }
                case FailureType::equal:
                case FailureType::different:
                default: {
                    oss << "\t\t\t[ERROR] " << std::endl;
                    break;
                }
            }
            return oss.str();
        }
    };

    class GenericTestFailure : public std::exception
    {};

    template<class ValueTypeL, class ValueTypeR, class ExceptionType>
    class TestFailure : public GenericTestFailure
    {
        public:
        TestFailure(const std::string& message, ValueTypeL reached, ValueTypeR expected, FailureType failure_type)
            : message_(message), reached_(reached), expected_(expected), failure_type_(failure_type) {}

        virtual const char* what() const noexcept override {
            message_ += '\n' + additionalInfos < is_streamable<std::stringstream, ValueTypeL>::value &&
                        is_streamable<std::stringstream, ValueTypeR>::value,
            !std::is_same_v<void, ExceptionType>>
            ::get(failure_type_, reached_, expected_, type_helper<ExceptionType>::name());
            return message_.c_str();
        }

        TestFailure& operator=(const TestFailure&) = delete;

        private:
        mutable std::string message_;
        const ValueTypeL    reached_;
        const ValueTypeR    expected_;
        const FailureType   failure_type_;
    };

    // Internal impl for processing an assert and raise the TestFailure Exception
    template<class ValueTypeL, class ValueTypeR, class ExceptionType = void,
             typename = std::enable_if_t<std::is_convertible_v<ValueTypeL, ValueTypeR> ||
                                         std::is_convertible_v<ValueTypeR, ValueTypeL>>>
    void FailureTest(bool condition, ValueTypeL reached, ValueTypeR expected, FailureType failure_type,
                     const std::string& message, const LineInfo& lineInfo) {
        std::ostringstream oss;
        if (!condition) {
            if (lineInfo.isInit()) { oss << message << "\t(" << lineInfo << ")"; }
            else {
                oss << message;
            }

            throw TestFailure<ValueTypeL, ValueTypeR, ExceptionType>(oss.str(), reached, expected, failure_type);
        }
    }

    // Assert test class to help verbosing test logic into lambda's impl
    template<class Expr>
    class AsserterExpression
    {
        public:
        using EmptyExpression = AsserterExpression<std::nullptr_t>;

        AsserterExpression() : expr_(nullptr) {}

        AsserterExpression(Expr&& expr) // Expr must be copyable or movable
            : expr_(std::forward<Expr>(expr)) {}

        AsserterExpression& operator=(const AsserterExpression& rhs) {
            static_assert(std::is_same<Expr, decltype(rhs.expr_)>::value, "Cannot assign between different types.");
            expr_ = rhs.expr_;
        }

        template<class NewExpr>
        AsserterExpression<NewExpr> andThat(NewExpr&& expr) {
            return {std::forward<NewExpr>(expr)};
        }

        // True condition
        EmptyExpression isTrue(const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(expr_, expr_, true, FailureType::equal, message, lineInfo);
            return {};
        }

        // False condition
        EmptyExpression isFalse(const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(!expr_, !expr_, false, FailureType::equal, message, lineInfo);
            return {};
        }

        // Verify that two references refer to the same object instance (identity):
        template<class T>
        EmptyExpression isSameAs(const T& actual, const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(&expr_ == &actual, &expr_, &actual, FailureType::equal, message, lineInfo);
            return {};
        }

        // Verify that two references do not refer to the same object instance (identity):
        template<class T>
        EmptyExpression isNotSameAs(const T& actual, const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(!(&expr_ == &actual), &expr_, &actual, FailureType::different, message, lineInfo);
            return {};
        }

        // Verify that a pointer is nullptr:
        EmptyExpression isNull(const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(expr_ == nullptr, expr_, nullptr, FailureType::equal, message, lineInfo);
            return {};
        }

        // Verify that a pointer is not nullptr:
        EmptyExpression isNotNull(const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(expr_ != nullptr, expr_, nullptr, FailureType::different, message, lineInfo);
            return {};
        }

        // Force the test case result to be fail:
        EmptyExpression fail(const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(false, false, false, FailureType::equal, message, lineInfo);
            return {};
        }

        // Verify that a function raises an exception:
        template<class ExpectedException>
        EmptyExpression expectException(const std::string& message = {}, const LineInfo& lineInfo = {}) {
            try {
                expr_();
            }
            catch (ExpectedException) {
                return {};
            }
            catch (...) {
            }

            return fail_exception<ExpectedException>(message, lineInfo);
        }

        // Invoque operator == on T
        template<class T>
        EmptyExpression isEqualTo(const T& expected, const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest(expr_ == expected, expr_, expected, FailureType::equal, message, lineInfo);
            return {};
        }

        // Check if 2 doubles are almost equals (tolerance given)
        EmptyExpression isEqualTo(double expected, double tolerance, const std::string& message = {},
                                  const LineInfo& lineInfo = {}) {
            double diff = expected - expr_;
            FailureTest(std::abs(diff) <= std::abs(tolerance), expr_, expected, FailureType::equal, message, lineInfo);
            return {};
        }

        // Check if 2 floats are almost equals (tolerance given)
        EmptyExpression isEqualTo(float expected, float tolerance, const std::string& message = {},
                                  const LineInfo& lineInfo = {}) {
            float diff = expected - expr_;
            FailureTest(std::abs(diff) <= std::abs(tolerance), expr_, expected, FailureType::equal, message, lineInfo);
            return {};
        }

        // Check if 2 char* are equals, considering the case by default
        EmptyExpression isEqualTo(const char* expected, bool ignoreCase, const std::string& message = {},
                                  const LineInfo& lineInfo = {}) {
            auto expected_str = std::string{expected};
            auto expr_str     = std::string{expr_};
            if (ignoreCase) {
                tolower_internal(expected_str);
                tolower_internal(expr_str);
            }
            FailureTest(expected_str == expr_str, expr_str, expected_str, FailureType::equal, message, lineInfo);
            return {};
        }

        // Check if 2 strings are equals, considering the case by default
        EmptyExpression isEqualTo(std::string expected, bool ignoreCase, const std::string& message = {},
                                  const LineInfo& lineInfo = {}) {
            if (ignoreCase) {
                tolower_internal(expected);
                tolower_internal(expr_);
            }
            FailureTest(expected == expr_, expr_, expected, FailureType::equal, message, lineInfo);
            return {};
        }

        // Invoque !operator == on T
        template<class T>
        EmptyExpression isNotEqualTo(const T& notExpected, const std::string& message = {},
                                     const LineInfo& lineInfo = {}) {
            FailureTest(!(notExpected == expr_), expr_, notExpected, FailureType::different, message, lineInfo);
            return {};
        }

        // Check if 2 doubles are not almost equals (tolerance given)
        EmptyExpression isNotEqualTo(double notExpected, double tolerance, const std::string& message = {},
                                     const LineInfo& lineInfo = {}) {
            double diff = notExpected - expr_;
            FailureTest(std::abs(diff) > std::abs(tolerance), expr_, notExpected, FailureType::different, message,
                        lineInfo);
            return {};
        }

        // Check if 2 floats are not almost equals (tolerance given)
        EmptyExpression isNotEqualTo(float notExpected, float expr_, float tolerance, const std::string& message = {},
                                     const LineInfo& lineInfo = {}) {
            float diff = notExpected - expr_;
            FailureTest(std::abs(diff) > std::abs(tolerance), expr_, notExpected, FailureType::different, message,
                        lineInfo);
            return {};
        }

        // Check if 2 char* are not equals, considering the case by default
        EmptyExpression isNotEqualTo(const char* notExpected, bool ignoreCase, const std::string& message = {},
                                     const LineInfo& lineInfo = {}) {
            auto notExpected_str = std::string{notExpected};
            auto expr_str        = std::string{expr_};
            if (ignoreCase) {
                tolower_internal(notExpected_str);
                tolower_internal(expr_str);
            }
            FailureTest(notExpected_str != expr_str, expr_str, notExpected_str, FailureType::different, message,
                        lineInfo);
            return {};
        }

        // Check if 2 strings are not equals, considering the case by default
        EmptyExpression isNotEqualTo(std::string notExpected, bool ignoreCase, const std::string& message = {},
                                     const LineInfo& lineInfo = {}) {
            if (ignoreCase) {
                tolower_internal(notExpected);
                tolower_internal(expr_);
            }
            FailureTest(notExpected != expr_, expr_, notExpected, FailureType::different, message, lineInfo);
            return {};
        }

        private:
        // Lower case function
        void tolower_internal(std::string& s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](char c) { return static_cast<char>(::tolower(static_cast<int>(c))); });
        }

        // Force the test case result to be fail:
        template<class ExpectedException>
        EmptyExpression fail_exception(const std::string& message = {}, const LineInfo& lineInfo = {}) {
            FailureTest<bool, bool, ExpectedException>(false, false, false, FailureType::exception, message, lineInfo);
            return {};
        }

        Expr expr_; // Current value stored
    };

    // Functor to build an Asserter with template deduction
    template<class Expr>
    AsserterExpression<Expr> AssertThat(Expr&& expr) {
        return {std::forward<Expr>(expr)};
    }

    using TestFunctor     = std::function<void(void)>;
    using SetUpFunctor    = std::function<void(void)>;
    using TearDownFunctor = std::function<void(void)>;
    using Duration        = std::chrono::duration<double, std::milli>; // ms

    // Standard class discribing a test
    class Test
    {
        public:
        // States of the test
        enum class Status {
            PASSED, // test successfuly passed
            FAILED, // test failed to pass (an assert failed)
            ERROR,  // an error occured during the test :
            // any exception was catched like bad_alloc
            SKIPPED, // test was skipped and not run
            NONE     // the run_scenario function wasn't run yet
            // for the scenario holding the test
        };

        // All available constructors
        Test() : Test({}, []() {}) {}
        Test(const TestFunctor&& test) : Test("", std::move(test)) {}
        Test(const std::string& label) : Test(label, []() {}) {}
        Test(const std::string& label, const TestFunctor&& test)
            : test_holder_(std::make_unique<TestFunctor>(std::move(test))), label_(label), status_(Status::NONE) {}

        // Copy forbidden
        Test(const Test&) = delete;
        Test& operator=(const Test&) = delete;

        // Default move impl (for VC2013)
        Test(Test&& test)
            : exec_time_ms_(test.exec_time_ms_), test_holder_(std::move(test.test_holder_)), label_(test.label_),
              failure_reason_(test.failure_reason_), skipped_reason_(test.skipped_reason_), error_(test.error_),
              status_(test.status_) {}
        Test&& operator=(Test&& test) {
            test_holder_    = std::move(test.test_holder_);
            label_          = test.label_;
            status_         = test.status_;
            exec_time_ms_   = test.exec_time_ms_;
            failure_reason_ = test.failure_reason_;
            skipped_reason_ = test.skipped_reason_;
            error_          = test.error_;
            return std::move(*this);
        }

        Test(std::reference_wrapper<Test> test) : Test(std::move(test.get())) {}

        // Virtual destructor impl
        virtual ~Test() {}

        // Information getters
        const std::string& getLabel(bool verbose) const { return getLabel_private(verbose); }
        const std::string&               getFailureReason() const { return getFailureReason_private(); }
        const std::string&               getSkippedReason() const { return getSkippedReason_private(); }
        const std::string&               getError() const { return getError_private(); }
        Duration                         getExecTimeMs() const { return getExecTimeMs_private(); }
        Status                           getStatus() const { return getStatus_private(); }

        protected:
        // Called by RegistryManager
        void run(const SetUpFunctor& setup, const TearDownFunctor& teardown) {
            setup();
            run_private();
            teardown();
        }

        // Run the test and capture and set the state
        virtual void run_private() {
            auto start = std::chrono::high_resolution_clock::now();
            try {
                (*test_holder_)(); /* /!\ Here is the test call /!\ */
                status_ = Status::PASSED;
            }
            catch (const GenericTestFailure& failure) {
                status_         = Status::FAILED;
                failure_reason_ = failure.what();
            }
            catch (const std::exception& e) {
                status_ = Status::ERROR;
                error_  = e.what();
            }
            catch (...) {
                status_ = Status::ERROR;
                error_  = "Unkown error";
            }
            exec_time_ms_ = std::chrono::high_resolution_clock::now() - start;
        }

        // Informations getters impl
        virtual const std::string& getLabel_private(bool /*verbose*/) const { return label_; }
        virtual const std::string& getFailureReason_private() const { return failure_reason_; }
        virtual const std::string& getSkippedReason_private() const { return skipped_reason_; }
        virtual const std::string& getError_private() const { return error_; }
        virtual Duration           getExecTimeMs_private() const { return exec_time_ms_; }
        virtual Status             getStatus_private() const { return status_; }

        protected:
        Duration                     exec_time_ms_;
        std::unique_ptr<TestFunctor> test_holder_;
        std::string                  label_;
        std::string                  failure_reason_;
        std::string                  skipped_reason_;
        std::string                  error_;
        Status                       status_;

        template<class ScenarioName>
        friend class RegistryManager;
    };

    std::ostream& operator<<(std::ostream& os, Test::Status status) {
        switch (status) {
            case Test::Status::PASSED: os << "PASSED"; break;
            case Test::Status::FAILED: os << "FAILED"; break;
            case Test::Status::ERROR: os << "ERROR"; break;
            case Test::Status::SKIPPED: os << "SKIPPED"; break;
            case Test::Status::NONE:
            default: os << "NOT RUN YET"; break;
        }
        return os;
    }

    std::string to_string(Test::Status status) {
        std::ostringstream os;
        os << status;
        return os.str();
    }

    // This class wrap a test and make it so it's skipped (never run)
    class SkippedTest : public Test
    {
        public:
        SkippedTest(TestFunctor&& func) : Test{std::move(func)} {}
        SkippedTest(const std::string& label, TestFunctor&& func) : Test{label, std::move(func)} {}
        SkippedTest(const std::string& reason, const std::string& label, TestFunctor&& func)
            : SkippedTest{label, std::move(func)} {
            skipped_reason_ = reason;
        }

        protected:
        // Put state to skipped and don't run the test
        virtual void run_private() override { status_ = Test::Status::SKIPPED; }
    };

    // Helper functions to build/skip a test case
    std::unique_ptr<Test> make_test(TestFunctor&& func) { return std::make_unique<Test>(std::move(func)); }
    std::unique_ptr<Test> make_test(const std::string& label, TestFunctor&& func) {
        return std::make_unique<Test>(label, std::move(func));
    }
    std::unique_ptr<Test> make_skipped_test(TestFunctor&& test) {
        return std::make_unique<SkippedTest>(std::move(test));
    }
    std::unique_ptr<Test> make_skipped_test(const std::string& label, TestFunctor&& func) {
        return std::make_unique<SkippedTest>(label, std::move(func));
    }
    std::unique_ptr<Test> make_skipped_test(const std::string& reason, const std::string& label, TestFunctor&& func) {
        return std::make_unique<SkippedTest>(reason, label, std::move(func));
    }

    // POD containing informations about a test
    using TestInfo = std::reference_wrapper<const Test>;

    // Interface for making an observer
    class IRegistryObserver
    {
        public:
        virtual void update(TestInfo infos) const = 0;
    };

    // Implementation of the observable part of the DP observer
    class IRegistryObservable
    {
        public:
        void notify(TestInfo infos) const {
            for (auto& observer : list_observers_) { observer->update(infos); }
        }
        void addObserver(const std::shared_ptr<IRegistryObserver>& observer) { list_observers_.insert(observer); }
        void removeObserver(const std::shared_ptr<IRegistryObserver>& observer) { list_observers_.erase(observer); }

        private:
        std::set<std::shared_ptr<IRegistryObserver>> list_observers_;
    };

    // Global static registry storage object
    using TestList        = std::vector<std::unique_ptr<Test>>;
    using TestStorage     = std::map<std::type_index, TestList>;
    using SetUpStorage    = std::map<std::type_index, SetUpFunctor>;
    using TearDownStorage = std::map<std::type_index, TearDownFunctor>;

    class RegistryStorage
    {
        public:
        TestList& getTests(std::type_index index) { return getAllTests()[index]; }
        SetUpFunctor& getSetUp(std::type_index index) { return getAllSetUps()[index]; }
        TearDownFunctor& getTearDown(std::type_index index) { return getAllTearDowns()[index]; }

        TestStorage&  getAllTests() { return tests_; }
        SetUpStorage& getAllSetUps() { return setups_; }
        SetUpStorage& getAllTearDowns() { return teardowns_; }

        private:
        TestStorage     tests_;
        SetUpStorage    setups_;
        TearDownStorage teardowns_;
    };

    RegistryStorage& get_registry() {
        static RegistryStorage registry;
        return registry;
    }

    // Manage a registry in a static context
    template<class ScenarioName>
    class RegistryManager : public IRegistryObservable
    {
        public:
        using FeederFunctor = std::function<void(void)>;

        RegistryManager(FeederFunctor feeder) : run_(false), exec_time_ms_accumulator_(Duration{0}) { feeder(); }

        // Recursive variadic to iterate over the test pack
        void add_test(Test&& test) {
            get_registry().getTests(type_helper<ScenarioName>::type_index()).push_back(std::move(test));
        }

        void add_test(TestFunctor&& func) {
            get_registry()
            .getTests(type_helper<ScenarioName>::type_index())
            .push_back(std::move(make_test(std::move(func))));
        }

        void add_test(const std::string& label, TestFunctor&& func) {
            get_registry()
            .getTests(type_helper<ScenarioName>::type_index())
            .push_back(std::move(make_test(label, std::move(func))));
        }
        void skip_test(TestFunctor&& func) {
            get_registry()
            .getTests(type_helper<ScenarioName>::type_index())
            .push_back(std::move(make_skipped_test(std::move(func))));
        }

        void skip_test(const std::string& label, TestFunctor&& func) {
            get_registry()
            .getTests(type_helper<ScenarioName>::type_index())
            .push_back(std::move(make_skipped_test(label, std::move(func))));
        }

        void skip_test(const std::string& reason, const std::string& label, TestFunctor&& func) {
            get_registry()
            .getTests(type_helper<ScenarioName>::type_index())
            .push_back(std::move(make_skipped_test(reason, label, std::move(func))));
        }

        void set_up(SetUpFunctor&& func) {
            get_registry().getSetUp(type_helper<ScenarioName>::type_index()) = std::move(func);
        }

        void tear_down(TearDownFunctor&& func) {
            get_registry().getTearDown(type_helper<ScenarioName>::type_index()) = std::move(func);
        }

        // Run all the tests
        void run_tests() {
            const auto& setup    = get_registry().getSetUp(type_helper<ScenarioName>::type_index());
            const auto& teardown = get_registry().getTearDown(type_helper<ScenarioName>::type_index());
            auto&       tests    = get_registry().getTests(type_helper<ScenarioName>::type_index());
            for (auto& test : tests) {
                test->run(setup, teardown);
                exec_time_ms_accumulator_ += test->getExecTimeMs();
                notify(TestInfo{*test});
                switch (test->getStatus()) {
                    case Test::Status::PASSED: tests_passed_.push_back(std::cref(*test)); break;
                    case Test::Status::FAILED: tests_failed_.push_back(std::cref(*test)); break;
                    case Test::Status::SKIPPED: tests_skipped_.push_back(std::cref(*test)); break;
                    case Test::Status::ERROR: tests_with_error_.push_back(std::cref(*test)); break;
                    default: break;
                }
            }
            run_ = true;
        }

        // describe test suite
        virtual void describe() {}

        // Get informations

        size_t getPassedCount() const { return run_ ? tests_passed_.size() : 0; }
        const std::vector<std::reference_wrapper<const Test>>& getPassedTests() const { return tests_passed_; }

        size_t getFailedCount() const { return run_ ? tests_failed_.size() : 0; }
        const std::vector<std::reference_wrapper<const Test>>& getFailedTests() const { return tests_failed_; }

        size_t getSkippedCount() const { return run_ ? tests_skipped_.size() : 0; }
        const std::vector<std::reference_wrapper<const Test>>& getSkippedTests() const { return tests_skipped_; }

        size_t getWithErrorCount() const { return run_ ? tests_with_error_.size() : 0; }
        const std::vector<std::reference_wrapper<const Test>>& getWithErrorTests() const { return tests_with_error_; }

        size_t getAllTestsCount() const {
            return run_ ? get_registry().getTests(type_helper<ScenarioName>::type_index()).size() : 0;
        }
        const TestList& getAllTests() const { return get_registry().getTests(type_helper<ScenarioName>::type_index()); }
        Duration        getAllTestsExecTimeMs() const { return run_ ? exec_time_ms_accumulator_ : Duration{0}; }

        private:
        bool                                            run_;
        Duration                                        exec_time_ms_accumulator_;
        std::vector<std::reference_wrapper<const Test>> tests_passed_;
        std::vector<std::reference_wrapper<const Test>> tests_failed_;
        std::vector<std::reference_wrapper<const Test>> tests_skipped_;
        std::vector<std::reference_wrapper<const Test>> tests_with_error_;
    };
}

/*
 *
 * Public interface
 *
 */

// Public accessible types
using detail::LineInfo;
using detail::TestInfo;
using detail::Test;
using detail::RegistryStorage;
using detail::IRegistryObserver;
template<class ScenarioName>
using RegistryManager = detail::RegistryManager<ScenarioName>;

// Asserter exposition
namespace Asserter {
    using detail::AsserterExpression;
    using detail::AssertThat;
}

// Interface to Implement to access access a registry information
// Possibility to export services into a DLL for forther customization
template<class ScenarioName>
class IRegistryTraversal
{
    public:
    IRegistryTraversal(const RegistryManager<ScenarioName>& registry) : registry_(registry) {}
    virtual ~IRegistryTraversal() {}

    protected:
    const RegistryManager<ScenarioName>& getRegistryManager() const { return registry_; }

    private:
    RegistryManager<ScenarioName> registry_;
};

// Trivial impl for console display results
template<class ScenarioName>
class RegistryTraversal_ConsoleIO : private IRegistryTraversal<ScenarioName>
{
    public:
    RegistryTraversal_ConsoleIO(const RegistryManager<ScenarioName>& registry)
        : IRegistryTraversal<ScenarioName>(registry) {}
    void print(bool verbose) const {
        auto&      registry_manager = this->getRegistryManager();
        const auto test_name        = std::string{H2OFastTests::detail::type_helper<ScenarioName>::name()};
        ColoredPrintf(COLOR_CYAN, "UNIT TEST SUMMARY [%s] [%.6f ms] : \n",
                      test_name.substr(test_name.find(' ') + 1).c_str(),
                      registry_manager.getAllTestsExecTimeMs().count());

        if (registry_manager.getPassedCount() > 0) {
            ColoredPrintf(COLOR_GREEN, "\tPASSED: %d/%d\n", registry_manager.getPassedCount(),
                          registry_manager.getAllTestsCount());
            if (verbose) {
                for (const auto& test : registry_manager.getPassedTests()) {
                    ColoredPrintf(COLOR_GREEN, "\t\t[%s] [%.6f ms]\n", test.get().getLabel(verbose).c_str(),
                                  test.get().getExecTimeMs().count());
                }
            }
        }

        if (registry_manager.getFailedCount() > 0) {
            ColoredPrintf(COLOR_RED, "\tFAILED: %d/%d\n", registry_manager.getFailedCount(),
                          registry_manager.getAllTestsCount());
            // Always print failed tests
            for (const auto& test : registry_manager.getFailedTests()) {
                ColoredPrintf(COLOR_RED, "\t\t[%s] [%.6f ms]\n\t\tMessage: %s\n", test.get().getLabel(verbose).c_str(),
                              test.get().getExecTimeMs().count(), test.get().getFailureReason().c_str());
            }
        }

        if (registry_manager.getSkippedCount() > 0) {
            ColoredPrintf(COLOR_YELLOW, "\tSKIPPED: %d/%d\n", registry_manager.getSkippedCount(),
                          registry_manager.getAllTestsCount());
            if (verbose) {
                for (const auto& test : registry_manager.getSkippedTests()) {
                    ColoredPrintf(COLOR_YELLOW, "\t\t[%s] [%.6f ms]\n\t\tMessage: %s\n",
                                  test.get().getLabel(verbose).c_str(), test.get().getExecTimeMs().count(),
                                  test.get().getSkippedReason().c_str());
                }
            }
        }

        if (registry_manager.getWithErrorCount() > 0) {
            ColoredPrintf(COLOR_PURPLE, "\tERRORS: %d/%d\n", registry_manager.getWithErrorCount(),
                          registry_manager.getAllTestsCount());
            // Always print error tests
            for (const auto& test : registry_manager.getWithErrorTests()) {
                ColoredPrintf(COLOR_PURPLE, "\t\t[%s] [%.6f ms]\n\t\tMessage: %s\n",
                              test.get().getLabel(verbose).c_str(), test.get().getExecTimeMs().count(),
                              test.get().getError().c_str());
            }
        }
    }
};

// Observer impl example
class ConsoleIO_Observer : public IRegistryObserver
{
    virtual void update(TestInfo infos) const override {
        std::cout << (infos.get().getStatus() == Test::Status::SKIPPED ? "SKIPPING TEST [" : "RUNNING TEST [")
                  << infos.get().getLabel(false) << "] [" << infos.get().getExecTimeMs().count() << "ms]:" << std::endl
                  << "Status: " << infos.get().getStatus() << std::endl;
    }
};
}

// Helper macros to use the unit test suit
#define register_scenario(ScenarioName)                                                                                \
    struct ScenarioName : H2OFastTests::template RegistryManager<ScenarioName> {                                       \
        ScenarioName(H2OFastTests::template RegistryManager<ScenarioName>::FeederFunctor feeder);                      \
        virtual void describe();                                                                                       \
    };                                                                                                                 \
    static ScenarioName ScenarioName##_registry_manager{[]() {                                                         \
        H2OFastTests::detail::get_registry().getAllTests().emplace(                                                    \
        H2OFastTests::detail::template type_helper<ScenarioName>::type_index(), H2OFastTests::detail::TestList{});     \
        H2OFastTests::detail::get_registry().getAllSetUps().emplace(                                                   \
        H2OFastTests::detail::template type_helper<ScenarioName>::type_index(), []() {});                              \
        H2OFastTests::detail::get_registry().getAllTearDowns().emplace(                                                \
        H2OFastTests::detail::template type_helper<ScenarioName>::type_index(), []() {});                              \
    }};                                                                                                                \
    ScenarioName::ScenarioName(H2OFastTests::template RegistryManager<ScenarioName>::FeederFunctor feeder)             \
        : RegistryManager<ScenarioName>{feeder} {                                                                      \
        describe();                                                                                                    \
    }                                                                                                                  \
    void ScenarioName::describe()

#define run_scenario(ScenarioName) ScenarioName##_registry_manager.run_tests();

#define register_observer(ScenarioName, class_name)                                                                    \
    ScenarioName##_registry_manager.addObserver(std::make_shared<class_name>())

#define register_custom_observer(ScenarioName, class_name, instance_ptr)                                               \
    ScenarioName##_registry_manager.addObserver(std::shared_ptr<class_name>(instance_ptr))

#define print_result_verbose(ScenarioName)                                                                             \
    H2OFastTests::RegistryTraversal_ConsoleIO<ScenarioName>(ScenarioName##_registry_manager).print(true)

#define print_result(ScenarioName)                                                                                     \
    H2OFastTests::RegistryTraversal_ConsoleIO<ScenarioName>(ScenarioName##_registry_manager).print(false)

#define line_info() H2OFastTests::LineInfo(__FILE__, "", __LINE__)
#define line_info_f() H2OFastTests::LineInfo(__FILE__, __FUNCTION__, __LINE__)

#endif
