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
#include <Builders/CopyDependencyBuilder/CopyDependencyBuilderWorker.h>

namespace CopyDependencyBuilder
{
    class CfgBuilderWorker
        : public CopyDependencyBuilderWorker
    {
    public:
        AZ_RTTI(CfgBuilderWorker, "{3386036B-A65B-4CC8-A35F-93C7C53A0333}", CopyDependencyBuilderWorker);

        CfgBuilderWorker();

        void RegisterBuilderWorker() override;
        void UnregisterBuilderWorker() override;
        bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) override;

        // This is used for automated tests, it shouldn't be called directly.
        static bool ParseProductDependenciesFromCfgContents(const AZStd::string& fullPath, const AZStd::string& contents, AssetBuilderSDK::ProductPathDependencySet& pathDependencies);
    private:
    };
}
