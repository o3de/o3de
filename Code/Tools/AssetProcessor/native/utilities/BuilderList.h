/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <utilities/Builder.h>

namespace AssetProcessor
{
    //! Helper class that keeps track of the Builders and manages reserving a builder specifically for CreateJobs
    //! This class is not inherently thread-safe and must be locked before any access
    class BuilderList
    {
    public:
        BuilderList() = default;

        void AddBuilder(AZStd::shared_ptr<Builder> builder, BuilderPurpose purpose);
        AZStd::shared_ptr<Builder> Find(AZ::Uuid uuid);
        BuilderRef GetFirst(BuilderPurpose purpose);
        AZStd::string RemoveByConnectionId(AZ::u32 connId);
        void RemoveByUuid(AZ::Uuid uuid);
        void PumpIdleBuilders();

        AZ_DISABLE_COPY_MOVE(BuilderList);

    protected:
        AZStd::unordered_map<AZ::Uuid, AZStd::shared_ptr<Builder>> m_builders;
        AZStd::shared_ptr<Builder> m_createJobsBuilder; // Special builder reserved for create jobs to ensure CreateJobs never waits for process startup
    };
} // namespace AssetProcessor
