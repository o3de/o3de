/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
#include <AzTest_Traits_Platform.h>
#include <list>
#include <array>

AZ_PUSH_DISABLE_WARNING(4389 4800, "-Wunknown-warning-option"); // 'int' : forcing value to bool 'true' or 'false' (performance warning).
#undef strdup // This define is required by googletest
#include <gtest/gtest.h>
#include <gmock/gmock.h>
AZ_POP_DISABLE_WARNING;

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif

#include <AzCore/Memory/OSAllocator.h>

#define AZTEST_DLL_PUBLIC AZ_DLL_EXPORT
#define AZTEST_EXPORT extern "C" AZTEST_DLL_PUBLIC

namespace AZ
{
    namespace Test
    {
        //! Forward declarations
        class Platform;

        /*!
        * Implement this interface to define the environment setup and teardown functions.
        */
        class ITestEnvironment
            : public ::testing::Environment
        {
        public:
            virtual ~ITestEnvironment()
            {}

            void SetUp() final
            {
                SetupEnvironment();
            }

            void TearDown() final
            {
                TeardownEnvironment();
            }

        protected:
            virtual void SetupEnvironment() = 0;
            virtual void TeardownEnvironment() = 0;
        };

        extern ::testing::Environment* sTestEnvironment;

        /*!
        * Monolithic builds will have all the environments available.  Keep a mapping to run the desired envs.
        */
        class TestEnvironmentRegistry
        {
        public:
            TestEnvironmentRegistry(std::vector<ITestEnvironment*> a_envs, const std::string& a_module_name, bool a_unit)
                : m_module_name(a_module_name)
                , m_envs(a_envs)
                , m_unit(a_unit)
            {
                s_envs.push_back(this);
            }

            const std::string m_module_name;
            std::vector<ITestEnvironment*> m_envs;
            bool m_unit;

            static std::vector<TestEnvironmentRegistry*> s_envs;

        private:
            TestEnvironmentRegistry& operator=(const TestEnvironmentRegistry& tmp);
        };

        /*!
        * Empty implementation of ITestEnvironment.
        */
        class EmptyTestEnvironment final
            : public ITestEnvironment
        {
        public:
            virtual ~EmptyTestEnvironment()
            {}

        protected:
            void SetupEnvironment() override
            {}
            void TeardownEnvironment() override
            {}
        };

        void addTestEnvironment(ITestEnvironment* env);
        void addTestEnvironments(std::vector<ITestEnvironment*> envs);
        
        //! A hook that can be used to read any other misc parameters and remove them before google sees them.
        //! Note that this modifies argc and argv to delete the parameters it consumes.
        void ApplyGlobalParameters(int* argc, char** argv);
        void printUnusedParametersWarning(int argc, char** argv);

        /*!
        * Main method for running tests from an executable.
        */
        class AzUnitTestMain final
        {
        public:
            AzUnitTestMain(std::vector<AZ::Test::ITestEnvironment*> envs)
                : m_returnCode(0)
                , m_envs(envs)
            {}

            bool Run(int argc, char** argv);
            bool Run(const char* commandLine);
            int ReturnCode() const { return m_returnCode; }

        private:
            int m_returnCode;
            std::vector<ITestEnvironment*> m_envs;
        };

        //! Run tests in a single library by loading it dynamically and executing the exported symbol,
        //! passing main-like parameters (argc, argv) from the (real or artificial) command line.
        int RunTestsInLib(Platform& platform, const std::string& lib, const std::string& symbol, int& argc, char** argv);

#if defined(HAVE_BENCHMARK)
        static constexpr const char* s_benchmarkEnvironmentName = "BenchmarkEnvironment";

        // BenchmarkEnvironment is a base that can be implemented to used to perform global initialization and teardown
        // for a module
        class BenchmarkEnvironmentBase
        {
        public:
            virtual ~BenchmarkEnvironmentBase() = default;

            virtual void SetUpBenchmark()
            {
            }
            virtual void TearDownBenchmark()
            {
            }
        };

        class BenchmarkEnvironmentRegistry
        {
        public:
            BenchmarkEnvironmentRegistry() = default;
            BenchmarkEnvironmentRegistry(const BenchmarkEnvironmentRegistry&) = delete;
            BenchmarkEnvironmentRegistry& operator=(const BenchmarkEnvironmentRegistry&) = delete;

            void AddBenchmarkEnvironment(std::unique_ptr<BenchmarkEnvironmentBase> env)
            {
                m_envs.push_back(std::move(env));
            }

            std::vector<std::unique_ptr<BenchmarkEnvironmentBase>>& GetBenchmarkEnvironments()
            {
                return m_envs;
            }

        private:
            std::vector<std::unique_ptr<BenchmarkEnvironmentBase>> m_envs;
        };

        /*
         * Creates a BenchmarkEnvironment using the specified template type and registers it with the BenchmarkEnvironmentRegister
         * @param T template argument that must have BenchmarkEnvironmentBase as a base class
         * @return returns a reference to the created BenchmarkEnvironment
         */
        template<typename T>
        T& RegisterBenchmarkEnvironment()
        {
            static_assert(std::is_base_of<BenchmarkEnvironmentBase, T>::value, "Supplied benchmark environment must be derived from BenchmarkEnvironmentBase");

            static AZ::EnvironmentVariable<AZ::Test::BenchmarkEnvironmentRegistry> s_benchmarkRegistry;
            if (!s_benchmarkRegistry)
            {
                s_benchmarkRegistry = AZ::Environment::CreateVariable<AZ::Test::BenchmarkEnvironmentRegistry>(s_benchmarkEnvironmentName);
            }

            auto benchmarkEnv{ new T };
            s_benchmarkRegistry->AddBenchmarkEnvironment(std::unique_ptr<BenchmarkEnvironmentBase>{ benchmarkEnv });
            return *benchmarkEnv;
        }

        template<typename... Ts>
        std::array<BenchmarkEnvironmentBase*, sizeof...(Ts)> RegisterBenchmarkEnvironments()
        {
            constexpr size_t EnvironmentCount{ sizeof...(Ts) };
            if constexpr (EnvironmentCount)
            {
                std::array<BenchmarkEnvironmentBase*, EnvironmentCount> benchmarkEnvs{ { &RegisterBenchmarkEnvironment<Ts>()... } };
                return benchmarkEnvs;
            }
            else
            {
                std::array<BenchmarkEnvironmentBase*, EnvironmentCount> benchmarkEnvs{};
                return benchmarkEnvs;
            }
        }
#endif
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //! listener class to capture and print test output for embedded platforms
        class OutputEventListener : public ::testing::EmptyTestEventListener
        {
        public:
            std::list<std::string> resultList;
            
            void OnTestEnd(const ::testing::TestInfo& test_info) override
            {
                std::string result;
                if (test_info.result()->Failed())
                {
                    result = "Fail";
                }
                else
                {
                    result = "Pass";
                }
                std::string formattedResult = "[GTEST][" + result + "] " + test_info.test_case_name() + " " + test_info.name() + "\n";
                resultList.emplace_back(formattedResult);
            }

            void OnTestProgramEnd(const ::testing::UnitTest& unit_test) override
            {
                for (std::string testResults : resultList)
                {
                    AZ_Printf("", testResults.c_str());
                }
                if (unit_test.current_test_info())
                {
                    AZ_Printf("", "[GTEST] %s completed %u tests with u% failed test cases.", unit_test.current_test_info()->name(), unit_test.total_test_count(), unit_test.failed_test_case_count());
                }
            }
        };
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        ITestEnvironment* DefaultTestEnv();

    } // Test
} // AZ

#define AZ_UNIT_TEST_HOOK_NAME AzRunUnitTests

#if !defined(AZ_MONOLITHIC_BUILD)

// Environments should be declared dynamically, framework will handle deletion of resources
#define AZ_UNIT_TEST_HOOK_ENV(TEST_ENV)                                                                 \
    AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)                                     \
    {                                                                                                   \
        ::testing::InitGoogleMock(&argc, argv);                                                         \
        if (AZ_TRAIT_AZTEST_ATTACH_RESULT_LISTENER)                                                     \
        {                                                                                               \
            ::testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();   \
            listeners.Append(new AZ::Test::OutputEventListener);                                        \
        }                                                                                               \
        AZ::Test::ApplyGlobalParameters(&argc, argv);                                                   \
        AZ::Test::printUnusedParametersWarning(argc, argv);                                             \
        AZ::Test::addTestEnvironments({TEST_ENV});                                                      \
        int result = RUN_ALL_TESTS();                                                                   \
        return result;                                                                                  \
    }

#if defined(HAVE_BENCHMARK)
#define AZ_BENCHMARK_HOOK_ENV(TEST_ENV) \
AZTEST_EXPORT int AzRunBenchmarks(int argc, char** argv) \
{ \
    AZ::Test::RegisterBenchmarkEnvironments<TEST_ENV>(); \
    auto benchmarkEnvRegistry = AZ::Environment::FindVariable<AZ::Test::BenchmarkEnvironmentRegistry>(AZ::Test::s_benchmarkEnvironmentName); \
    std::vector<std::unique_ptr<AZ::Test::BenchmarkEnvironmentBase>>* benchmarkEnvs = benchmarkEnvRegistry ? &(benchmarkEnvRegistry->GetBenchmarkEnvironments()) : nullptr; \
    if (benchmarkEnvs != nullptr) \
    { \
        for (std::unique_ptr<AZ::Test::BenchmarkEnvironmentBase>& benchmarkEnv : *benchmarkEnvs) \
        { \
            if (benchmarkEnv) \
            { \
                benchmarkEnv->SetUpBenchmark(); \
            } \
        }\
    } \
    ::benchmark::Initialize(&argc, argv); \
    ::benchmark::RunSpecifiedBenchmarks(); \
    if (benchmarkEnvs != nullptr) \
    { \
        for (auto benchmarkEnvIter = benchmarkEnvs->rbegin(); benchmarkEnvIter != benchmarkEnvs->rend(); ++benchmarkEnvIter) \
        { \
            std::unique_ptr<AZ::Test::BenchmarkEnvironmentBase>& benchmarkEnv = *benchmarkEnvIter; \
            if (benchmarkEnv) \
            { \
                benchmarkEnv->TearDownBenchmark(); \
            } \
        }\
    } \
    return 0; \
}
#define AZ_BENCHMARK_HOOK() \
AZTEST_EXPORT int AzRunBenchmarks(int argc, char** argv) \
{ \
    AZ::Test::RegisterBenchmarkEnvironments<>(); \
    auto benchmarkEnvRegistry = AZ::Environment::FindVariable<AZ::Test::BenchmarkEnvironmentRegistry>(AZ::Test::s_benchmarkEnvironmentName); \
    std::vector<std::unique_ptr<AZ::Test::BenchmarkEnvironmentBase>>* benchmarkEnvs = benchmarkEnvRegistry ? &(benchmarkEnvRegistry->GetBenchmarkEnvironments()) : nullptr; \
    if (benchmarkEnvs != nullptr) \
    { \
        for (std::unique_ptr<AZ::Test::BenchmarkEnvironmentBase>& benchmarkEnv : *benchmarkEnvs) \
        { \
            if (benchmarkEnv) \
            { \
                benchmarkEnv->SetUpBenchmark(); \
            } \
        }\
    } \
    ::benchmark::Initialize(&argc, argv); \
    ::benchmark::RunSpecifiedBenchmarks(); \
    if (benchmarkEnvs != nullptr) \
    { \
        for (auto benchmarkEnvIter = benchmarkEnvs->rbegin(); benchmarkEnvIter != benchmarkEnvs->rend(); ++benchmarkEnvIter) \
        { \
            std::unique_ptr<AZ::Test::BenchmarkEnvironmentBase>& benchmarkEnv = *benchmarkEnvIter; \
            if (benchmarkEnv) \
            { \
                benchmarkEnv->TearDownBenchmark(); \
            } \
        }\
    } \
    return 0; \
}


#else // !HAVE_BENCHMARK
#define AZ_BENCHMARK_HOOK_ENV(TEST_ENV) \
int AzRunBenchmarks(int argc, char** argv) \
{                                                                           \
    std::cerr << "'AzRunBenchmarks' Not supported" << std::endl;            \
    return 1;                                                               \
}
#define AZ_BENCHMARK_HOOK() \
int AzRunBenchmarks(int argc, char** argv) \
{                                                                           \
    std::cerr << "'AzRunBenchmarks' Not supported" << std::endl;            \
    return 1;                                                               \
}
#endif // HAVE_BENCHMARK

#if defined(AZ_TEST_EXECUTABLE)

#define IMPLEMENT_TEST_EXECUTABLE_MAIN() \
int main(int argc, char** argv)                                                                             \
{                                                                                                           \
    const bool isUnitTestCmd = (argc<2)?true:(strcmp(argv[1], AZ_STRINGIZE(AZ_UNIT_TEST_HOOK_NAME))==0);  \
    const bool isBenchmarkCmd = (argc<2)?false:(strcmp(argv[1], "AzRunBenchmarks")==0);                   \
    if (isUnitTestCmd)                                                                                    \
    {                                                                                                       \
        if (argc<2)                                                                                         \
        {                                                                                                   \
            return AZ_UNIT_TEST_HOOK_NAME(argc, argv);                                                      \
        }                                                                                                   \
        else                                                                                                \
        {                                                                                                   \
            argv[1] = argv[0];                                                                              \
            return AZ_UNIT_TEST_HOOK_NAME(argc-1, &argv[1]);                                                \
        }                                                                                                   \
    }                                                                                                       \
    else if (isBenchmarkCmd)                                                                              \
    {                                                                                                       \
        argv[1] = argv[0];                                                                                  \
        return AzRunBenchmarks(argc-1, &argv[1]);                                                           \
    }                                                                                                       \
    else                                                                                                    \
    {                                                                                                       \
        std::cerr << "Invalid arguments for test" << std::endl;                                             \
        return 1;                                                                                           \
    }                                                                                                       \
}

#else

#define IMPLEMENT_TEST_EXECUTABLE_MAIN()

#endif // defined(AZ_TEST_EXECUTABLE)

#else // monolithic build

#undef GTEST_MODULE_NAME_
#define GTEST_MODULE_NAME_ AZ_MODULE_NAME
#define AZTEST_CONCAT_(a, b) a ## _ ## b
#define AZTEST_CONCAT(a, b) AZTEST_CONCAT_(a, b)

#define AZ_UNIT_TEST_HOOK_REGISTRY_NAME AZTEST_CONCAT(AZ_UNIT_TEST_HOOK_NAME, Registry)

#define AZ_UNIT_TEST_HOOK_ENV(TEST_ENV)                                                      \
            static AZ::Test::TestEnvironmentRegistry* AZ_UNIT_TEST_HOOK_REGISTRY_NAME =\
                new( AZ_OS_MALLOC(sizeof(AZ::Test::TestEnvironmentRegistry),           \
                                  alignof(AZ::Test::TestEnvironmentRegistry)))         \
              AZ::Test::TestEnvironmentRegistry({ TEST_ENV }, AZ_MODULE_NAME, true);         

#define AZ_BENCHMARK_HOOK_ENV(TEST_ENV)
#define AZ_BENCHMARK_HOOK()

#endif // AZ_MONOLITHIC_BUILD



// These macros are needed to implement unit test hooks necessary for running AzUnitTests or AzBenchmarks.
// 

/* For unit test modules that implement AzUnitTests and AzBenchmarkTests with either a custom environment for AzUnitTests or a custom environment class for AzBenchmarks
   the follow use the overloaded 'IMPLEMENT_AZ_UNIT_TEST_HOOKS' macro

   AZ_UNIT_TEST_HOOK(UNIT_TEST_ENV, BENCHMARK_ENV_CLASS)

   // Implement unit test hooks without a custom AzUnitTest or AzBenchmark environment,
   AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
   
   // Implement unit test hooks with a custom AzUnitTest environment
   AZ_UNIT_TEST_HOOK(new CustomEnvClass());

   // Implement unit test hooks with a custom AzBenchmark environment class only
   AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV, CustomBenchmarkEnvClass);

   // Implement unit test hooks with a custom AzUnitTest environment and a custom AzBenchmark environment class
   AZ_UNIT_TEST_HOOK(new CustomEnvClass(), CustomBenchmarkEnvClass);
*/

#define DEFAULT_UNIT_TEST_ENV AZ::Test::DefaultTestEnv()

#define AZ_UNIT_TEST_HOOK_1(_1) \
    AZ_UNIT_TEST_HOOK_ENV(_1) \
    AZ_BENCHMARK_HOOK() \
    IMPLEMENT_TEST_EXECUTABLE_MAIN()

#define AZ_UNIT_TEST_HOOK_2(_1, _2) \
    AZ_UNIT_TEST_HOOK_ENV(_1) \
    AZ_BENCHMARK_HOOK_ENV(_2) \
    IMPLEMENT_TEST_EXECUTABLE_MAIN()

#define AZ_UNIT_TEST_HOOK(...)           AZ_MACRO_SPECIALIZE(AZ_UNIT_TEST_HOOK_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))


// Declares a visible external symbol which identifies an executable as containing tests
#define DECLARE_AZ_UNIT_TEST_MAIN() AZTEST_EXPORT int ContainsAzUnitTestMain() { return 1; }

// Attempts to invoke the unit test main function if appropriate flags are present,
// otherwise simply continues launch as normal.
#define INVOKE_AZ_UNIT_TEST_MAIN(...)                                               \
    do {                                                                            \
        AZ::Test::AzUnitTestMain unitTestMain({__VA_ARGS__});                       \
        if (unitTestMain.Run(argc, argv))                                           \
        {                                                                           \
            return unitTestMain.ReturnCode();                                       \
        }                                                                           \
    } while (0); // safe multi-line macro - creates a single statement

// Some implementations use a commandLine rather than argc/argv
#define INVOKE_AZ_UNIT_TEST_MAIN_COMMAND_LINE(...)                                  \
    do {                                                                            \
        AZ::Test::AzUnitTestMain unitTestMain({__VA_ARGS__});                       \
        if (unitTestMain.Run(commandLine))                                          \
        {                                                                           \
            return unitTestMain.ReturnCode();                                       \
        }                                                                           \
    } while (0); // safe multi-line macro - creates a single statement

// Avoid problems with new/delete when AZ allocators are not ready or properly un/initialized.
#define AZ_TEST_CLASS_ALLOCATOR(Class_)                                 \
    void* operator new (size_t size)                                    \
    {                                                                   \
        return AZ_OS_MALLOC(size, AZStd::alignment_of<Class_>::value);  \
    }                                                                   \
    void operator delete(void* ptr)                                     \
    {                                                                   \
        AZ_OS_FREE(ptr);                                                \
    }

