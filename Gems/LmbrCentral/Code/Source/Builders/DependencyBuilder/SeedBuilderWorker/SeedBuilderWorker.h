/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <Builders/DependencyBuilder/DependencyBuilderWorker.h>

namespace DependencyBuilder
{
    //! The Seed builder is a dependency builder that track dependencies information for seed files.
    class SeedBuilderWorker
        : public DependencyBuilderWorker
    {
    public:

    public:
        AZ_RTTI(SeedBuilderWorker, "{529F547B-F4C9-49B9-8BCC-E9F2C2273DC8}", DependencyBuilderWorker);

        SeedBuilderWorker();

        void RegisterBuilderWorker() override;
        void UnregisterBuilderWorker() override;

        AZ::Outcome<AZStd::vector<AssetBuilderSDK::SourceFileDependency>, AZStd::string> GetSourceDependencies(const AssetBuilderSDK::CreateJobsRequest& request)  const override;
    };
}
