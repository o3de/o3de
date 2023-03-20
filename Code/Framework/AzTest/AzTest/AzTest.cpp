/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/Platform.h>
#include <AzCore/UnitTest/UnitTest.h>

namespace AZ
{
    namespace Test
    {
        ::testing::Environment* sTestEnvironment = nullptr;

        //! Add a single test environment to the framework
        void addTestEnvironment(ITestEnvironment* env)
        {
            sTestEnvironment = ::testing::AddGlobalTestEnvironment(env);
        }

        //! Add a list of test environments to the framework
        void addTestEnvironments(std::vector<ITestEnvironment*> envs)
        {
            //! If nothing is supplied, add the default hook
            if (envs.empty())
            {
                addTestEnvironment(AZ::Test::DefaultTestEnv());
            }

            else
            {
                for (auto env : envs)
                {
                    //! Skip over nullptr to allow callers to avoid the default hook
                    if (env != nullptr)
                    {
                        addTestEnvironment(env);
                    }
                }
            }
        }

        std::vector<TestEnvironmentRegistry*> TestEnvironmentRegistry::s_envs;

        void AddExcludeFilter(const char* name)
        {
            std::string currentFilter = ::testing::GTEST_FLAG(filter);

            if (currentFilter.compare("*") == 0)  // is the current filter already the wildcard '*' ?
            {
                // in which case, change it from wildcard to be everything except the filter ('-thing')
                ::testing::GTEST_FLAG(filter) = "-";
            }
            else
            {
                // otherwise, there is some sort of filter already. Is it a negation filter?
                if (currentFilter.find("-") == std::string::npos)
                {
                    // no, add a negation.  Only one negation (minus symbol) can appear and everything
                    // after that negation is negated.
                    ::testing::GTEST_FLAG(filter).append(":-");
                }
                else
                {
                    // if there's a negation filter already, we just append to the end, and we dont negate again
                    ::testing::GTEST_FLAG(filter).append(":");
                }
            }
            ::testing::GTEST_FLAG(filter).append(name);
        }

        void AddIncludeFilter(const char* name)
        {
            std::string currentFilter = ::testing::GTEST_FLAG(filter);
            if (currentFilter.compare("*") == 0)  // is the current filter already the wildcard '*' only
            {
                // replace it to filter the name
                ::testing::GTEST_FLAG(filter) = name;
            }
            else
            {
                // prepend to the exisiting filter.
                std::stringstream additionalFilter;
                additionalFilter << name << ":" << currentFilter;
                ::testing::GTEST_FLAG(filter) = additionalFilter.str();
            }
        }

        void ApplyGlobalParameters(int* argc, char** argv)
        {
            // this is a hook that can be used to apply any other global parameters that we use.
            AZ_UNUSED(argc);
            AZ_UNUSED(argv);

            // Disable gtest catching unhandled exceptions, instead, AzTestRunner will do it through:
            // AZ::Debug::Trace::HandleExceptions(true). This gives us a stack trace when the exception
            // is thrown (googletest does not).
            testing::FLAGS_gtest_catch_exceptions = false;
        }

        //! Print out parameters that are not used by the framework
        void printUnusedParametersWarning(int argc, char** argv)
        {
            //! argv[0] is the runner executable name, which we expect and need to keep around
            if (argc > 1)
            {
                std::cerr << "WARNING: unrecognized parameters: ";
                for (int i = 1; i < argc; i++)
                {
                    std::cerr << argv[i] << ", ";
                }
                std::cerr << std::endl;
            }
        }

        bool AzUnitTestMain::Run(int argc, char** argv)
        {
            using namespace AZ::Test;
            m_returnCode = 0;
            if (ContainsParameter(argc, argv, "--unittest") || ContainsParameter(argc, argv, "--unittests"))
            {
                // the --unittest parameter makes us run tests built inside this executable

                // first, remove the unit test parameter so that it doesn't get passed into google
                // test, which would potentially generate warnings since its a non standard param:
                int unitTestIndex = GetParameterIndex(argc, argv, "--unittest");
                if (unitTestIndex != -1)
                {
                    RemoveParameters(argc, argv, unitTestIndex, unitTestIndex);
                }
                unitTestIndex = GetParameterIndex(argc, argv, "--unittests");
                if (unitTestIndex != -1)
                {
                    RemoveParameters(argc, argv, unitTestIndex, unitTestIndex);
                }

                int waitForDebbugerIndex = GetParameterIndex(argc, argv, "--wait-for-debugger");
                if (waitForDebbugerIndex != -1)
                {
                    RemoveParameters(argc, argv, waitForDebbugerIndex, waitForDebbugerIndex);

                    AZ::Test::Platform& platform = AZ::Test::GetPlatform();

                    if (platform.SupportsWaitForDebugger())
                    {
                        std::cout << "Waiting for debugger..." << std::endl;
                        platform.WaitForDebugger();
                    }
                    else
                    {
                        std::cerr << "Warning - platform does not support --wait-for-debugger feature" << std::endl;
                    }
                }

                ::testing::InitGoogleMock(&argc, argv);
                AZ::Test::ApplyGlobalParameters(&argc, argv);
                AZ::Test::printUnusedParametersWarning(argc, argv);
                AZ::Test::addTestEnvironments(m_envs);
                m_returnCode = RUN_ALL_TESTS();
                if (::testing::UnitTest::GetInstance()->test_to_run_count() == 0)
                {
                    std::cerr << "No tests were found for last suite ran!" << std::endl;
                    m_returnCode = 1;
                }
                return true;
            }
            else if (ContainsParameter(argc, argv, "--loadunittests"))
            {
                // Run tests inside defined lib(s) (as runner would)
                Platform& platform = GetPlatform();
                platform.Printf("Running tests with arguments: \"");
                for (int i = 0; i < argc; i++)
                {
                    platform.Printf("%s", argv[i]);
                    if (i < argc - 1)
                    {
                        platform.Printf(" ");
                    }
                }
                platform.Printf("\"\n");

                int testFlagIndex = GetParameterIndex(argc, argv, "--loadunittests");
                RemoveParameters(argc, argv, testFlagIndex, testFlagIndex);

                // Grab the test symbol to call
                std::string symbol = GetParameterValue(argc, argv, "--symbol", true);
#if !defined(AZ_MONOLITHIC_BUILD)
                if (symbol.empty())
                {
                    platform.Printf("ERROR: Must provide --symbol to run tests inside libs!\n");
                    return false;
                }
#endif // AZ_MONOLITHIC_BUILD

                // Get the lib information
                if (ContainsParameter(argc, argv, "--libs"))
                {
                    // Multiple libs have been given, so grab them all (as a list)
                    auto libs = GetParameterList(argc, argv, "--libs", true);

                    // Since we have multiple libs, check for a path for the XML files (do NOT pass in --gtest_output directly)
                    std::string outputPath = GetParameterValue(argc, argv, "--output_path", true);
                    platform.Printf("Outputing XML files to: %s\n", outputPath.c_str());

                    // Now iterate through each lib, defining the output file each time
                    for (std::string lib : libs)
                    {
                        // Create the full output path for the XML file
                        std::string libName = platform.GetModuleNameFromPath(lib);
                        std::stringstream outputFileStream;
                        outputFileStream << "--gtest_output=xml:" << outputPath << "test_result_" << libName << ".xml";
                        std::string outputFile = outputFileStream.str();

                        // Create a new array since GTest removes parameters
                        int targc = argc + 1;
                        char** targv = new char*[targc];
                        CopyParameters(argc, targv, argv);

                        targv[targc - 1] = const_cast<char*>(outputFile.c_str());

                        // Run the tests
                        m_returnCode = RunTestsInLib(platform, lib, symbol, targc, targv);
                        platform.Printf("Test result from '%s': %d\n", lib.c_str(), m_returnCode);

                        // Cleanup
                        delete[] targv;
                    }
                }
                else if (ContainsParameter(argc, argv, "--lib"))
                {
                    // Only one lib has been given
                    std::string lib = GetParameterValue(argc, argv, "--lib", true);
                    m_returnCode = RunTestsInLib(platform, lib, symbol, argc, argv);
                    platform.Printf("Test result from '%s': %d\n", lib.c_str(), m_returnCode);
                }
                else
                {
                    platform.Printf("ERROR: Must specify either --lib or --libs to run tests!\n");
                    return false;
                }
                return true;
            }
            return false;
        }

        bool AzUnitTestMain::Run(const char* commandLine)
        {
            int tokenSize;
            char** commandTokens = AZ::Test::SplitCommandLine(tokenSize, const_cast<char*>(commandLine));
            bool result = Run(tokenSize, commandTokens);
            for (int i = 0; i < tokenSize; i++)
            {
                delete[] commandTokens[i];
            }
            delete[] commandTokens;
            return result;
        }

        int RunTestsInLib(AZ::Test::Platform& platform, const std::string& lib, const std::string& symbol, int& argc, char** argv)
        {
#if defined(AZ_MONOLITHIC_BUILD)
            ::testing::InitGoogleMock(&argc, argv);

            std::string lib_upper(lib);
            std::transform(lib_upper.begin(), lib_upper.end(), lib_upper.begin(), ::toupper);
            for (auto env : AZ::Test::TestEnvironmentRegistry::s_envs)
            {
                if (env->m_module_name == lib_upper)
                {
                    AZ::Test::addTestEnvironments(env->m_envs);
                    ::testing::GTEST_FLAG(module) = lib_upper;

                    platform.Printf("Found library: %s\n", lib_upper.c_str());
                }
                else
                {
                    platform.Printf("Available library %s does not match %s.\n", env->m_module_name.c_str(), lib_upper.c_str());
                }
            }

            AZ::Test::printUnusedParametersWarning(argc, argv);

            int returnCode = RUN_ALL_TESTS()
            if (::testing::UnitTest::GetInstance()->test_to_run_count() == 0)
            {
                std::cerr << "No tests were found for last suite ran!" << std::endl;
                m_returnCode = 1;
            }
            return returnCode;


#else // AZ_MONOLITHIC_BUILD
            int result = 0;
            std::shared_ptr<AZ::Test::IModuleHandle> module = platform.GetModule(lib);
            if (module->IsValid())
            {
                platform.Printf("OKAY Library loaded: %s\n", lib.c_str());

                auto fn = module->GetFunction(symbol);
                if (fn->IsValid())
                {
                    platform.Printf("OKAY Symbol found: %s\n", symbol.c_str());

                    result = (*fn)(argc, argv);

                    platform.Printf("OKAY %s() return %d\n", symbol.c_str(), result);
                }
                else
                {
                    platform.Printf("FAILED to find symbol: %s\n", symbol.c_str());
                    result = SYMBOL_NOT_FOUND;
                }
            }
            else
            {
                platform.Printf("FAILED to load library: %s\n", lib.c_str());
                result = LIB_NOT_FOUND;
            }
            return result;
#endif // AZ_MONOLITHIC_BUILD
        }

        ITestEnvironment* DefaultTestEnv()
        {
            return new UnitTest::TraceBusHook();
        }
    } // Test
} // AZ
