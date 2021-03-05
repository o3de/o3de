/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
