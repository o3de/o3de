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

#include <fstream>

namespace AzTestRunner
{
    constexpr int INCORRECT_USAGE = 101;
    constexpr int LIB_NOT_FOUND = 102;
    constexpr int SYMBOL_NOT_FOUND = 103;
    constexpr char argFromFileSeparator = '\n';

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
            "   --args_from_file <filename>: reads additional arguments (newline separated) from the specified file (can be used in conjunction with regular command line arguments)\n"
            "\n"
            "Example:\n"
            "   AzTestRunner.exe AzCore.Tests.dll AzRunUnitTests --args_from_file args.txt\n"
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

        // copy over the command line args to a vector so we can expand with any arguments from file
        std::vector<std::string> arguments(argv, argv + argc);

        // capture positional arguments
        // [0] is the program name
        std::string lib = arguments[1];
        std::string symbol = arguments[2];

        // shift args parameters down as we don't need lib or symbol anymore
        arguments.erase(std::next(arguments.begin(), 1), std::next(arguments.begin(), 3));

        // capture optional arguments
        bool waitForDebugger = false;
        bool pauseOnCompletion = false;
        bool quiet = false;
        for (int i = 0; i < arguments.size(); i++)
        {
            if (arguments[i] == "--wait-for-debugger")
            {
                waitForDebugger = true;
                arguments.erase(arguments.begin() + i);
                i--;
            }
            else if (arguments[i] == "--pause-on-completion")
            {
                pauseOnCompletion = true;
                arguments.erase(arguments.begin() + i);
                i--;
            }
            else if (arguments[i] == "--quiet")
            {
                quiet = true;
                arguments.erase(arguments.begin() + i);
                i--;
            }
            else if (arguments[i] == "--args_from_file")
            {
                // check that the arg file path has been passed
                if (i + 1 >= arguments.size())
                {
                    std::cout << "Incorrect number of args_from_file arguments\n";
                    usage(platform);
                    return INCORRECT_USAGE;
                }

                // attempt to read the contents of the file
                std::ifstream infile(arguments[i + 1]);
                if (!infile.is_open())
                {
                    std::cout << "Couldn't open " << arguments[i + 1] << " for args input, exiting" << std::endl;
                    return INCORRECT_USAGE;
                }
            
                // remove the args_from_file argument and value from the arg list
                arguments.erase(std::next(arguments.begin(), i), std::next(arguments.begin(), i + 2));

                // Insert the args at the current position in the command line
                std::string arg;
                while (std::getline(infile, arg, argFromFileSeparator))
                {
                    arguments.insert(arguments.begin() + i, arg);
                }
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

        // wait for debugger
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
            // collapse the arguments vector into a c-style array of character pointers
            // note that the standard expects argc to be the number of actual arguments
            // but it also expects argv[argc] to be a valid access, and be nullptr.
            // as in, a real 3-argument program would have:
            // char* argv[3] = { "program", "arg1", "arg2", nullptr};
            // argc = 3;
            // notice that in the above example, argv[argc] is expected to be not an out-of-bounds-access, but in fact, okay to access.
            // this comes from the fact that the args to a program come from a block of memory passed to its process
            // as part of its environment, and the args sit in memory null-separated, with an extra null denoting the end
            // of the packed args.  This is relevant here because GoogleTest *absorbs* args (removing them from the command line as it parses them)
            // by looping over  { argv[n] = argv[n+1]; } for n...argc-1 and would otherwise trip over the end of our created array
            // if we didn't have that extra null at arg[n+1] (ASAN notices this).
            std::vector<const char*> charArguments;
            charArguments.reserve(arguments.size() + 1);
            for (const std::string_view argument : arguments)
            {
                charArguments.push_back(argument.data());
            }
            
            charArguments.push_back(nullptr);

            result = (*testMainFunction)(static_cast<int>(charArguments.size() - 1), const_cast<char**>(charArguments.data()));
            std::cout << "OKAY " << symbol << "() returned " << result << std::endl;
            testMainFunction.reset();
        }

        // construct a retry command if the test fails
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
        const AZ::Debug::Trace tracer;
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
