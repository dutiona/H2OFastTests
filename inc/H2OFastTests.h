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
#include <vector>
#include <typeinfo>
#include <typeindex>

#include "H2OFastTests_config.h"

namespace H2OFastTests {

    // Implementation details
    namespace detail {

        //XCode workaround
#if __APPLE__
        template<typename T, typename ... Args>
        std::unique_ptr<T> make_unique(Args&& ...args) {
            return std::unique_ptr<T>{ new T{ std::forward<Args>(args)... } };
        }
#else
        using std::make_unique;
#endif

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
            virtual const char * what() const throw() override { return message_.c_str(); }
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

            using empty_expression_t = AsserterExpression<std::nullptr_t>;

            template<class E>
            friend AsserterExpression<E> AssertThat(E&& expr);

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
                catch (...) {}

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
                FailureTest(std::abs(diff) <= std::abs(tolerance), message, lineInfo);
                return{};
            }

            // Check if 2 floats are almost equals (tolerance given)
            empty_expression_t isEqualTo(float expected, float tolerance,
                const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
                float diff = expected - expr_;
                FailureTest(std::abs(diff) <= std::abs(tolerance), message, lineInfo);
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
                FailureTest(std::abs(diff) > std::abs(tolerance), message, lineInfo);
                return{};
            }

            // Check if 2 floats are not almost equals (tolerance given)
            empty_expression_t isNotEqualTo(float notExpected, float expr_, float tolerance,
                const char* message = nullptr, const line_info_t* lineInfo = nullptr) {
                float diff = notExpected - expr_;
                FailureTest(std::abs(diff) > std::abs(tolerance), message, lineInfo);
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
                PASSED,    // test successfuly passed
                FAILED,    // test failed to pass (an assert failed)
                ERROR,    // an error occured during the test :
                // any exception was catched like bad_alloc
                SKIPPED,// test was skipped and not run
                NONE    // the run_scenario function wasn't run yet
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
                : test_holder_(make_unique<test_func_t>(std::move(test))), label_(label), status_(NONE)
            {}

            // Copy forbidden
            Test(const Test&) = delete;
            Test& operator=(const Test&) = delete;

            // Default move impl (for VC2013)
            Test(Test&& test)
                : exec_time_ms_(test.exec_time_ms_),
                test_holder_(std::move(test.test_holder_)), label_(test.label_),
                failure_reason_(test.failure_reason_), skipped_reason_(test.skipped_reason_),
                error_(test.error_), status_(test.status_)
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
                catch (...) {
                    status_ = ERROR;
                    error_ = "Unkown error";
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

            template<class ScenarioName>
            friend class RegistryManager;
        };

        using test_t = detail::Test;
        using test_status_t = detail::Test::Status;

        std::ostream& operator<<(std::ostream& os, test_t::Status status) {
            switch (status)    {
            case test_status_t::PASSED:
                os << "PASSED";
                break;
            case test_status_t::FAILED:
                os << "FAILED";
                break;
            case test_status_t::ERROR:
                os << "ERROR";
                break;
            case test_status_t::SKIPPED:
                os << "SKIPPED";
                break;
            case test_status_t::NONE:
            default:
                os << "NOT RUN YET";
                break;
            }
            return os;
        }

        std::string to_string(test_t::Status status) {
            std::ostringstream os;
            os << status;
            return os.str();
        }

        // This class wrap a test and make it so it's skipped (never run)
        class SkippedTest : public Test {
        public:

            SkippedTest(test_func_t&& func)
                : Test{ std::move(func) } {}
            SkippedTest(const std::string& label, test_func_t&& func)
                : Test{ label, std::move(func) } {}
            SkippedTest(const std::string& reason, const std::string& label, test_func_t&& func)
                : SkippedTest{ label, std::move(func) }
            {
                skipped_reason_ = reason;
            }

        protected:

            // Put state to skipped and don't run the test
            virtual void run_private() override { status_ = SKIPPED; }

        };

        using skipped_test_t = detail::SkippedTest;

        // Helper functions to build/skip a test case
        std::unique_ptr<test_t> make_test(test_func_t&& func) { return make_unique<test_t>(std::move(func)); }
        std::unique_ptr<test_t> make_test(const std::string& label, test_func_t&& func) { return make_unique<test_t>(label, std::move(func)); }
        std::unique_ptr<test_t> make_skipped_test(test_func_t&& test) { return make_unique<skipped_test_t>(std::move(test)); }
        std::unique_ptr<test_t> make_skipped_test(const std::string& label, test_func_t&& func) { return make_unique<skipped_test_t>(label, std::move(func)); }
        std::unique_ptr<test_t> make_skipped_test(const std::string& reason, const std::string& label, test_func_t&& func) { return make_unique<skipped_test_t>(reason, label, std::move(func)); }

        // POD containing informations about a test
        struct TestInfos {
            using status_t = test_status_t;

            TestInfos(const test_t& t) : test(t) {}
            TestInfos& operator=(const TestInfos&) = delete;

            const test_t& test;
        };

        using tests_infos_t = TestInfos;

        template<class Type>
        struct type_helper {
            static bool before(const std::type_info& rhs) { return typeid(Type).before(rhs); }
            static const char* raw_name() { return typeid(Type).raw_name(); }
            static const char* name() { return typeid(Type).name(); }
            static size_t hash_code() { return typeid(C).hash_code(); }
            static std::type_index type_index() { return std::type_index(typeid(Type)); }
            using type = Type;
            template<class LhsType, class RhsType>
            friend bool operator==(const type_helper<LhsType>& lhs, const type_helper<RhsType>& rhs) { return typeid(LhsType) == typeid(RhsType); }
            template<class LhsType, class RhsType>
            friend bool operator!=(const type_helper<LhsType>& lhs, const type_helper<RhsType>& rhs) { return typeid(LhsType) != typeid(RhsType); }
        };

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
        using test_list_t = std::vector<std::unique_ptr<test_t>>;
        using registry_storage_t = std::map<std::type_index, test_list_t>;

        registry_storage_t& get_registry() {
            static registry_storage_t registry;
            return registry;
        }

        // Manage a registry in a static context
        template<class ScenarioName>
        class RegistryManager : public registry_observable_t{
        public:

            RegistryManager(std::function<bool(void)> feeder)
                : run_(false), exec_time_ms_accumulator_(duration_t{ 0 }) {
                feeder();
            }

            //Recursive variadic to iterate over the test pack
            void add_test(test_t&& test) {
                get_registry()[type_helper<ScenarioName>::type_index()].push_back(std::move(test));
            }

            void add_test(test_func_t&& func) {
                get_registry()[type_helper<ScenarioName>::type_index()].push_back(std::move(make_test(std::move(func))));
            }

            void add_test(const std::string& label, test_func_t&& func) {
                get_registry()[type_helper<ScenarioName>::type_index()].push_back(std::move(make_test(label, std::move(func))));
            }
            void skip_test(test_func_t&& func) {
                get_registry()[type_helper<ScenarioName>::type_index()].push_back(std::move(make_skipped_test(std::move(func))));
            }

            void skip_test(const std::string& label, test_func_t&& func) {
                get_registry()[type_helper<ScenarioName>::type_index()].push_back(std::move(make_skipped_test(label, std::move(func))));
            }

            void skip_test(const std::string& reason, const std::string& label, test_func_t&& func) {
                get_registry()[type_helper<ScenarioName>::type_index()].push_back(std::move(make_skipped_test(reason, label, std::move(func))));
            }

            // Run all the tests
            void run_tests() {
                auto& tests = get_registry()[type_helper<ScenarioName>::type_index()];
                for (auto& test : tests) {
                    test->run();
                    exec_time_ms_accumulator_ += test->getExecTimeMs();
                    notify(tests_infos_t{ *test });
                    switch (test->getStatus()) {
                    case test_status_t::PASSED:
                        tests_passed_.push_back(test.get());
                        break;
                    case test_status_t::FAILED:
                        tests_failed_.push_back(test.get());
                        break;
                    case test_status_t::SKIPPED:
                        tests_skipped_.push_back(test.get());
                        break;
                    case test_status_t::ERROR:
                        tests_with_error_.push_back(test.get());
                        break;
                    default: break;
                    }
                }
                run_ = true;
            }


            // Get informations

            size_t getPassedCount() const { return run_ ? tests_passed_.size() : 0; }
            const std::vector<const test_t*>& getPassedTests() const { return tests_passed_; }

            size_t getFailedCount() const { return run_ ? tests_failed_.size() : 0; }
            const std::vector<const test_t*>& getFailedTests() const { return tests_failed_; }

            size_t getSkippedCount() const { return run_ ? tests_skipped_.size() : 0; }
            const std::vector<const test_t*>& getSkippedTests() const { return tests_skipped_; }

            size_t getWithErrorCount() const { return run_ ? tests_with_error_.size() : 0; }
            const std::vector<const test_t*>& getWithErrorTests() const { return tests_with_error_; }

            size_t getAllTestsCount() const { return run_ ? get_registry()[type_helper<ScenarioName>::type_index()].size() : 0; }
            const test_list_t& getAllTests() const { return get_registry()[type_helper<ScenarioName>::type_index()]; }
            duration_t getAllTestsExecTimeMs() const { return run_ ? exec_time_ms_accumulator_ : duration_t{ 0 }; }

        private:

            bool run_;
            duration_t exec_time_ms_accumulator_;
            std::vector<const test_t*> tests_passed_;
            std::vector<const test_t*> tests_failed_;
            std::vector<const test_t*> tests_skipped_;
            std::vector<const test_t*> tests_with_error_;

        };

        template<class ScenarioName>
        using registry_manager_t = RegistryManager<ScenarioName>;
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
    using registry_observer_t = detail::IRegistryObserver;
    template<class ScenarioName>
    using registry_manager_t = detail::RegistryManager<ScenarioName>;

    // Asserter exposition
    namespace Asserter {
        using detail::AsserterExpression;
        using detail::AssertThat;
    }

    // Interface to Implement to access access a registry information
    // Possibility to export services into a DLL for forther customization
    template<class ScenarioName>
    class IRegistryTraversal {
    public:
        IRegistryTraversal(const registry_manager_t<ScenarioName>& registry) : registry_(registry) {}
        virtual ~IRegistryTraversal() {}
    protected:
        const registry_manager_t<ScenarioName>& getRegistryManager() const { return registry_; }
    private:
        registry_manager_t<ScenarioName> registry_;
    };

    // Trivial impl for console display results
    template<class ScenarioName>
    class RegistryTraversal_ConsoleIO : private IRegistryTraversal<ScenarioName> {
    public:
        RegistryTraversal_ConsoleIO(const registry_manager_t<ScenarioName>& registry) : IRegistryTraversal<ScenarioName>(registry) {}
        void print(bool verbose) const {
            auto& registry_manager = getRegistryManager();
            const auto test_name = std::string{ H2OFastTests::detail::type_helper<ScenarioName>::name() };
            ColoredPrintf(COLOR_CYAN, "UNIT TEST SUMMARY [%s] [%f ms] : \n", test_name.substr(test_name.find(' ') + 1).c_str(), registry_manager.getAllTestsExecTimeMs().count());

            ColoredPrintf(COLOR_GREEN, "\tPASSED: %d/%d\n", registry_manager.getPassedCount(), registry_manager.getAllTestsCount());
            if (verbose) {
                for (const auto test : registry_manager.getPassedTests()) {
                    ColoredPrintf(COLOR_GREEN, "\t\t[%s] [%f ms]\n", test->getLabel(verbose).c_str(), test->getExecTimeMs().count());
                }
            }

            ColoredPrintf(COLOR_RED, "\tFAILED: %d/%d\n", registry_manager.getFailedCount(), registry_manager.getAllTestsCount());
            // Always print failed tests
            for (const auto test : registry_manager.getFailedTests()) {
                ColoredPrintf(COLOR_RED, "\t\t[%s] [%f ms]\n\t\tMessage: %s\n", test->getLabel(verbose).c_str(), test->getExecTimeMs().count(), test->getFailureReason().c_str());
            }

            ColoredPrintf(COLOR_YELLOW, "\tSKIPPED: %d/%d\n", registry_manager.getSkippedCount(), registry_manager.getAllTestsCount());
            if (verbose) {
                for (const auto test : registry_manager.getSkippedTests()) {
                    ColoredPrintf(COLOR_YELLOW, "\t\t[%s] [%f ms]\n\t\tMessage: %s\n", test->getLabel(verbose).c_str(), test->getExecTimeMs().count(), test->getSkippedReason().c_str());
                }
            }

            ColoredPrintf(COLOR_PURPLE, "\tERRORS: %d/%d\n", registry_manager.getWithErrorCount(), registry_manager.getAllTestsCount());
            // Always print error tests
            for (const auto test : registry_manager.getWithErrorTests()) {
                ColoredPrintf(COLOR_PURPLE, "\t\t[%s] [%f ms]\n\t\tMessage: %s\n", test->getLabel(verbose).c_str(), test->getExecTimeMs().count(), test->getError().c_str());
            }
        }
    };

    // Observer impl example
    class ConsoleIO_Observer : public registry_observer_t {
        virtual void update(const test_infos_t& infos) const override {
            std::cout << (infos.test.getStatus() == test_infos_t::status_t::SKIPPED ? "SKIPPING TEST [" : "RUNNING TEST [")
                << infos.test.getLabel(false) << "] [" << infos.test.getExecTimeMs().count() << "ms]:" << std::endl
                << "Status: " << infos.test.getStatus() << std::endl;
        }
    };

}

//Helper macros to use the unit test suit
#define register_scenario(ScenarioName) \
    struct ScenarioName : H2OFastTests::detail::RegistryManager<ScenarioName> { \
        ScenarioName(std::function<bool(void)> feeder); \
    }; \
    static ScenarioName ScenarioName ## _registry_manager{ []() { \
        return H2OFastTests::detail::get_registry().emplace(H2OFastTests::detail::type_helper<ScenarioName>::type_index(), H2OFastTests::detail::test_list_t{}).second; \
    } }; \
    ScenarioName::ScenarioName(std::function<bool(void)> feeder) : RegistryManager<ScenarioName>{ feeder }

#define run_scenario(ScenarioName) \
    ScenarioName ## _registry_manager.run_tests();

#define register_observer(ScenarioName, class_name, instance_ptr) \
    ScenarioName ## _registry_manager.addObserver(std::shared_ptr<class_name>(instance_ptr))

#define print_result(ScenarioName, verbose) \
    H2OFastTests::RegistryTraversal_ConsoleIO<ScenarioName>(ScenarioName ## _registry_manager).print(verbose)

#define line_info() \
    &H2OFastTests::line_info_t(__FILE__, "", __LINE__)
#define line_info_f() \
    &H2OFastTests::line_info_t(__FILE__, __FUNCTION__, __LINE__)

#endif
