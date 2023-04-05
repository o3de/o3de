/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Target/Native/TestImpactNativeTestTarget.h>

namespace TestImpact
{
    namespace SupportedTestFrameworks
    {
        //! The CTest label for the GTest framework.
        inline constexpr const char* GTest = "FRAMEWORK_googletest";
    } // namespace SupportedTestFrameworks

    NativeTestTarget::NativeTestTarget(
        TargetDescriptor&& descriptor, NativeTestTargetMeta&& testMetaData)
        : TestTarget(AZStd::move(descriptor), AZStd::move(testMetaData.m_testTargetMeta))
        , m_canEnumerate(GetSuiteLabelSet().contains(SupportedTestFrameworks::GTest))
        , m_launchMeta(AZStd::move(testMetaData.m_launchMeta))
    {
        // Target must be able to enumerate as well as being opted in to test sharding in order to shard
        if (CanEnumerate())
        {
            if (GetSuiteLabelSet().contains(TiafShardingTestInterleavedLabel))
            {
                m_shardConfiguration = ShardingConfiguration::TestInterleaved;
            }
            else if (GetSuiteLabelSet().contains(TiafShardingFixtureInterleavedLabel))
            {
                m_shardConfiguration = ShardingConfiguration::FixtureInterleaved;
            }
        }
    }

    const AZStd::string& NativeTestTarget::GetCustomArgs() const
    {
        return m_launchMeta.m_customArgs;
    }

    LaunchMethod NativeTestTarget::GetLaunchMethod() const
    {
        return m_launchMeta.m_launchMethod;
    }

    bool NativeTestTarget::CanShard() const
    {
        return m_shardConfiguration != ShardingConfiguration::None;
    }

    ShardingConfiguration NativeTestTarget::GetShardingConfiguration() const
    {
        return m_shardConfiguration;
    }

    bool NativeTestTarget::CanEnumerate() const
    {
        return m_canEnumerate;
    }
} // namespace TestImpact
