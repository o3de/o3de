/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
