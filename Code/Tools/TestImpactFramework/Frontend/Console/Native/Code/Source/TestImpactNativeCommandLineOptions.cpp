/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactNativeCommandLineOptions.h>
#include <TestImpactCommandLineOptionsUtils.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <AzCore/Settings/CommandLine.h>

namespace TestImpact
{
    namespace NativeOptions
    {
        enum Fields
        {
            // Options
            MaxConcurrencyKey,
            // Checksum
            _CHECKSUM_
        };

        constexpr const char* Keys[] = {
            // Options
            "maxconcurrency",
        };

    } // namespace NativeOptions

    AZStd::optional<size_t> ParseMaxConcurrency(const AZ::CommandLine& cmd)
    {
        return ParseUnsignedIntegerOption(NativeOptions::Keys[NativeOptions::Fields::MaxConcurrencyKey], cmd);
    }

    NativeCommandLineOptions::NativeCommandLineOptions(int argc, char** argv)
        : CommandLineOptions(argc, argv)
    {
        static_assert(NativeOptions::_CHECKSUM_ == AZStd::size(NativeOptions::Keys));
        AZ::CommandLine cmd;
        cmd.Parse(argc, argv);

        m_maxConcurrency = ParseMaxConcurrency(cmd);
    }

    const AZStd::optional<size_t>& NativeCommandLineOptions::GetMaxConcurrency() const
    {
        return m_maxConcurrency;
    }

    AZStd::string NativeCommandLineOptions::GetCommandLineUsageString()
    {
        AZStd::string help = CommandLineOptions::GetCommandLineUsageString();
        help +=
            "    -maxconcurrency=<number>                                    The maximum number of concurrent test targets/shards to be in flight at \n"
            "                                                                any given moment.\n";

        return help;
    }
} // namespace TestImpact
