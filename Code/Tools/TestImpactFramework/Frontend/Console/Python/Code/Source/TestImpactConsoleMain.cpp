/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConsoleMain.h>
#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactChangeListException.h>
#include <TestImpactFramework/TestImpactChangeListSerializer.h>
#include <TestImpactFramework/TestImpactConfigurationException.h>
#include <TestImpactFramework/TestImpactSequenceReportException.h>

#include <TestImpactPythonCommandLineOptions.h>
#include <TestImpactCommandLineOptionsException.h>
#include <TestImpactConsoleUtils.h>

namespace TestImpact::Console
{
    //! Entry point for the test impact analysis framework console front end application.
    ReturnCode Main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
    {
        try
        {
            PythonCommandLineOptions options(argc, argv);
            AZStd::optional<ChangeList> changeList;

            if (options.HasChangeListFilePath())
            {
                changeList = DeserializeChangeList(ReadFileContents<CommandLineOptionsException>(*options.GetChangeListFilePath()));
            }

            // As of now, there are no non-test operations but leave this door open for the future
            if (options.GetTestSequenceType() == TestSequenceType::None)
            {
                std::cout << "No test operations specified.";
                return ReturnCode::Success;
            }

            return ReturnCode::Success;
        }
        catch (const CommandLineOptionsException& e)
        {
            std::cout << e.what() << std::endl;
            std::cout << PythonCommandLineOptions::GetCommandLineUsageString().c_str() << std::endl;
            return ReturnCode::InvalidArgs;
        }
        catch (const ChangeListException& e)
        {
            std::cout << e.what() << std::endl;
            return ReturnCode::InvalidChangeList;
        }
        catch (const ConfigurationException& e)
        {
            std::cout << e.what() << std::endl;
            return ReturnCode::InvalidConfiguration;
        }
        catch (const RuntimeException& e)
        {
            std::cout << e.what() << std::endl;
            return ReturnCode::RuntimeError;
        }
        catch (const Exception& e)
        {
            std::cout << e.what() << std::endl;
            return ReturnCode::UnhandledError;
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
            return ReturnCode::UnknownError;
        }
        catch (...)
        {
            std::cout << "An unknown error occurred" << std::endl;
            return ReturnCode::UnknownError;
        }
    }
} // namespace TestImpact::Console
