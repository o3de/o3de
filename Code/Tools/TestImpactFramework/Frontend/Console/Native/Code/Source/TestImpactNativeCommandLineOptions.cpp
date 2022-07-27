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
            TestTargetTimeoutKey,
            SafeModeKey,
        };

        constexpr const char* OptionKeys[] = {
            // Options
            "shard",
            "maxconcurrency",
            "ttimeout",
            "safemode",
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

        AZStd::optional<AZStd::chrono::milliseconds> ParseTestTargetTimeout(const AZ::CommandLine& cmd)
        {
            return ParseSecondsOption(OptionKeys[TestTargetTimeoutKey], cmd);
        }

        bool ParseSafeMode(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<bool> states = { false, true };
            return ParseOnOffOption(OptionKeys[SafeModeKey], states, cmd).value_or(false);
        }
    } // namespace

    NativeCommandLineOptions::NativeCommandLineOptions(int argc, char** argv)
        : CommandLineOptions(argc, argv)
    {
        AZ::CommandLine cmd;
        cmd.Parse(argc, argv);

        m_testShardingPolicy = ParseTestShardingPolicy(cmd);
        m_maxConcurrency = ParseMaxConcurrency(cmd);
        m_testTargetTimeout = ParseTestTargetTimeout(cmd);
        m_safeMode = ParseSafeMode(cmd);
    }

    bool NativeCommandLineOptions::HasSafeMode() const
    {
        return m_safeMode;
    }

    Policy::TestSharding NativeCommandLineOptions::GetTestShardingPolicy() const
    {
        return m_testShardingPolicy;
    }

    const AZStd::optional<size_t>& NativeCommandLineOptions::GetMaxConcurrency() const
    {
        return m_maxConcurrency;
    }

    const AZStd::optional<AZStd::chrono::milliseconds>& NativeCommandLineOptions::GetTestTargetTimeout() const
    {
        return m_testTargetTimeout;
    }

    AZStd::string NativeCommandLineOptions::GetCommandLineUsageString()
    {
        AZStd::string help = CommandLineOptions::GetCommandLineUsageString();
        help +=
            "    -ttimeout=<seconds>                                         Timeout value to terminate individual test targets should it be \n"
            "    -safemode=<on,off>                                          Flag to specify a safe mode sequence where the set of unselected \n"
            "                                                                tests is run without instrumentation after the set of selected \n"
            "                                                                instrumented tests is run (this has the effect of ensuring all \n"
            "                                                                tests are run regardless).\n"
            "    -shard=<on,off>                                             Break any test targets with a sharding policy into the number of \n"
            "                                                                shards according to the maximum concurrency value.\n"
            "    -maxconcurrency=<number>                                    The maximum number of concurrent test targets/shards to be in flight at \n"
            "                                                                any given moment.\n";

        return help;
    }
} // namespace TestImpact
