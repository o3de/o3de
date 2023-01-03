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
    namespace
    {
        enum NativeOptions
        {
            // Options
            TestShardingPolicyKey,
            MaxConcurrencyKey,
        };

        constexpr const char* OptionKeys[] = {
            // Options
            "shard",
            "maxconcurrency",
        };

        Policy::TestSharding ParseTestShardingPolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::TestSharding> states = { Policy::TestSharding::Never, Policy::TestSharding::Always };

            return ParseOnOffOption(OptionKeys[TestShardingPolicyKey], states, cmd).value_or(Policy::TestSharding::Never);
        }

        AZStd::optional<size_t> ParseMaxConcurrency(const AZ::CommandLine& cmd)
        {
            return ParseUnsignedIntegerOption(OptionKeys[MaxConcurrencyKey], cmd);
        }
    } // namespace

    NativeCommandLineOptions::NativeCommandLineOptions(int argc, char** argv)
        : CommandLineOptions(argc, argv)
    {
        AZ::CommandLine cmd;
        cmd.Parse(argc, argv);

        m_testShardingPolicy = ParseTestShardingPolicy(cmd);
        m_maxConcurrency = ParseMaxConcurrency(cmd);
    }

    Policy::TestSharding NativeCommandLineOptions::GetTestShardingPolicy() const
    {
        return m_testShardingPolicy;
    }

    const AZStd::optional<size_t>& NativeCommandLineOptions::GetMaxConcurrency() const
    {
        return m_maxConcurrency;
    }

    AZStd::string NativeCommandLineOptions::GetCommandLineUsageString()
    {
        AZStd::string help = CommandLineOptions::GetCommandLineUsageString();
        help +=
            "    -shard=<on,off>                                             Break any test targets with a sharding policy into the number of \n"
            "                                                                shards according to the maximum concurrency value.\n"
            "    -maxconcurrency=<number>                                    The maximum number of concurrent test targets/shards to be in flight at \n"
            "                                                                any given moment.\n";

        return help;
    }
} // namespace TestImpact
