/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/Platform.h>

#include "aztestrunner.h"


namespace AzTestRunner
{
    const int INCORRECT_USAGE = 101;
    const int LIB_NOT_FOUND = 102;
    const int SYMBOL_NOT_FOUND = 103;

    //! display proper usage of the application
    void usage([[maybe_unused]] AZ::Test::Platform& platform)
    {
        std::stringstream ss;
        ss <<
            "AzTestRunner\n"
            "Runs AZ tests. Exit code is the result from GoogleTest.\n"
            "\n"
            "Usage:\n"
            "   AzTestRunner.exe <lib> (AzRunUnitTests|AzRunBenchmarks) [--wait-for-debugger] [--pause-on-completion] [google-test-args]\n"
            "\n"
            "Options:\n"
            "   <lib>: the module to test\n"
            "   <hook>: the name of the aztest hook function to run in the <lib>\n"
            "           'AzRunUnitTests' will hook into unit tests\n"
            "           'AzRunBenchmarks' will hook into benchmark tests\n"
            "   --wait-for-debugger: tells runner to wait for debugger to attach to process (on supported platforms)\n"
            "   --pause-on-completion: tells the runner to pause after running the tests\n"
            "   --quiet: disables stdout for minimal output while running tests\n"
            "\n"
            "Example:\n"
            "   AzTestRunner.exe AzCore.Tests.dll AzRunUnitTests --pause-on-completion\n"
            "\n"
            "Exit Codes:\n"
            "   0 - all tests pass\n"
            "   1 - test failure\n"
            << "   " << INCORRECT_USAGE << " - incorrect usage (see above)\n"
            << "   " << LIB_NOT_FOUND << " - library/dll could not be loaded\n"
            << "   " << SYMBOL_NOT_FOUND << " - export symbol not found\n";

        std::cerr << ss.str() << std::endl;
    }

    //! attempt to run the int X() method exported by the specified library
    int wrapped_command_arg_main(int argc, char** argv)
    {
        AZ::Test::Platform& platform = AZ::Test::GetPlatform();

        if (argc < 3)
        {
            usage(platform);
            return INCORRECT_USAGE;
        }

        // capture positional arguments
        // [0] is the program name
        std::string lib = argv[1];
        std::string symbol = argv[2];

        // shift argv parameters down as we don't need lib or symbol anymore
        AZ::Test::RemoveParameters(argc, argv, 1, 2);

        // capture optional arguments
        bool waitForDebugger = false;
        bool pauseOnCompletion = false;
        bool quiet = false;
        for (int i = 0; i < argc; i++)
        {
            if (strcmp(argv[i], "--wait-for-debugger") == 0)
            {
                waitForDebugger = true;
                AZ::Test::RemoveParameters(argc, argv, i, i);
                i--;
            }
            else if (strcmp(argv[i], "--pause-on-completion") == 0)
            {
                pauseOnCompletion = true;
                AZ::Test::RemoveParameters(argc, argv, i, i);
                i--;
            }
            else if (strcmp(argv[i], "--quiet") == 0)
            {
                quiet = true;
                AZ::Test::RemoveParameters(argc, argv, i, i);
                i--;
            }
        }

        if (quiet)
        {
            AzTestRunner::set_quiet_mode();
        }
        else
        {
            const char* cwd = AzTestRunner::get_current_working_directory();
            std::cout << "cwd = " << cwd << std::endl;
            std::cout << "LIB: " << lib << std::endl;
        }

        // Wait for debugger
        if (waitForDebugger)
        {
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

        // make sure the module actually has the expected entry point before proceeding.
        // it is very expensive to start the bootstrapper.

        std::shared_ptr<AZ::Test::IFunctionHandle> testMainFunction;

        std::cout << "Loading: " << lib << std::endl;
        std::shared_ptr<AZ::Test::IModuleHandle> module = platform.GetModule(lib);

        int result = 0;
        if (module->IsValid())
        {
            std::cout << "OKAY Library loaded: " << lib << std::endl;

            testMainFunction = module->GetFunction(symbol);
            if (!testMainFunction->IsValid())
            {
                std::cerr << "FAILED to find symbol: " << symbol << std::endl;
                result = SYMBOL_NOT_FOUND;
            }
            else
            {
                std::cout << "OKAY Symbol found: " << symbol << std::endl;
            }
        }
        else
        {
            std::cerr << "FAILED to load library: " << lib << std::endl;
            result = LIB_NOT_FOUND;
        }

        // bail out early if the module does not contain the expected entry point or the module could not be loaded.
        if (result != 0)
        {
            module.reset();
            return result;
        }

        platform.SuppressPopupWindows();

        // run the test main function.
        if (testMainFunction->IsValid())
        {
            result = (*testMainFunction)(argc, argv);
            std::cout << "OKAY " << symbol << "() returned " << result << std::endl;
            testMainFunction.reset();
        }

        // Construct a retry command if the test fails
        if (result != 0)
        {
            std::cout << "Retry command: " << std::endl << argv[0] << " " << lib << " " << symbol << std::endl;
        }

        // unload and reset the module here, because it needs to release resources that were used / activated in
        // system allocator / etc.
        module.reset();

        if (pauseOnCompletion)
        {
            AzTestRunner::pause_on_completion();
        }

        return result;
    }


    int wrapped_main(int argc/*=0*/, char** argv/*=nullptr*/)
    {
        AZ::Debug::Trace::HandleExceptions(true);

        if (argc>0 && argv!=nullptr)
        {
            return wrapped_command_arg_main(argc, argv);
        }
        else
        {
            return 0;
        }
    }
}
