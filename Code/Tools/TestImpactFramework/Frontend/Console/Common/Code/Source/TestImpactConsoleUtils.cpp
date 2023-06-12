/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactConsoleUtils.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/AnsiTerminalUtils.h>
#include <AzCore/IO/SystemFile.h>

namespace TestImpact::Console
{
    AZStd::string SetColor(Foreground foreground, Background background)
    {
        if (AZ::IO::Posix::SupportsAnsiEscapes(AZ::IO::Posix::Fileno(stdout)))
        {
            return AZStd::string::format("\033[%u;%um", aznumeric_cast<uint32_t>(foreground), aznumeric_cast<uint32_t>(background));
        }

        // ANSI escapes aren't supported
        return AZStd::string{};
    }

    AZStd::string SetColorForString(Foreground foreground, Background background, const AZStd::string& str)
    {
        if (AZ::IO::Posix::SupportsAnsiEscapes(AZ::IO::Posix::Fileno(stdout)))
        {
            return AZStd::string::format("%s%s%s", SetColor(foreground, background).c_str(), str.c_str(), ResetColor().c_str());
        }

        // Return the string without ANSI escapes when they are not supported
        return str;
    }

    AZStd::string ResetColor()
    {
        return AZ::IO::Posix::SupportsAnsiEscapes(AZ::IO::Posix::Fileno(stdout))
            ? AZStd::string("\033[0m")
            : AZStd::string{};
    }

    ReturnCode GetReturnCodeForTestSequenceResult(TestSequenceResult result)
    {
        switch (result)
        {
        case TestSequenceResult::Success:
            return ReturnCode::Success;
        case TestSequenceResult::Failure:
            return ReturnCode::TestFailure;
        case TestSequenceResult::Timeout:
            return ReturnCode::Timeout;
        default:
            std::cout << "Unexpected TestSequenceResult value: " << aznumeric_cast<size_t>(result) << std::endl;
            return ReturnCode::UnknownError;
        }
    }
} // namespace TestImpact::Console
