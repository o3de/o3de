/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Static/TestImpactNativeTestTargetMeta.h>
#include <Target/Common/TestImpactTestTarget.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    //! The sharding configuration to use for a given test target opted in to test shard optimization.
    enum class ShardingConfiguration
    {
        None, //!< Do not use test shard optimization.
        TestInterleaved, //!< Interleave the tests across the available shards (better performance, less stable).
        FixtureInterleaved //!< Interleave the fixtures across the available shards (less performance, more stable).
    };

    //! Build target specialization for native test targets (build targets containing test code and no production code).
    class NativeTestTarget 
    : public TestTarget
    {
    public:
        NativeTestTarget(TargetDescriptor&& descriptor, NativeTestTargetMeta&& testMetaData);

        //! Returns the launcher custom arguments.
        const AZStd::string& GetCustomArgs() const;
        
        //! Returns the test target launch method.
        LaunchMethod GetLaunchMethod() const;

        //! Returns `true` if the target can shard, otherwise `false`.
        bool CanShard() const;

        //! Returns the sharding configuration for this test target.
        ShardingConfiguration GetShardingConfiguration() const;

        // TestTarget overrides ...
        bool CanEnumerate() const override;

    private:
        NativeTargetLaunchMeta m_launchMeta;
        ShardingConfiguration m_shardConfiguration = ShardingConfiguration::None;
        bool m_canEnumerate = false;
    };
} // namespace TestImpact
