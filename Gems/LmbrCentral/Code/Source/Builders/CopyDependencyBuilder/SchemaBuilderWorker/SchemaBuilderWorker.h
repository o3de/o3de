/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <Builders/CopyDependencyBuilder/CopyDependencyBuilderWorker.h>

namespace CopyDependencyBuilder
{
    class SchemaBuilderWorker
        : public CopyDependencyBuilderWorker
    {
    public:
        AZ_RTTI(SchemaBuilderWorker, "{BF5B2E93-0373-4078-ACA7-5A43C4A1F6CF}");

        SchemaBuilderWorker();

        void RegisterBuilderWorker() override;
        void UnregisterBuilderWorker() override;

        AZ::Outcome<AZStd::vector<AZStd::string>, AZStd::string> GetSourcesToReprocess(
            const AssetBuilderSDK::ProcessJobRequest& request) const override;

        bool ParseProductDependencies(
            const AssetBuilderSDK::ProcessJobRequest& request,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies) override;

    private:
        AZ::Data::AssetType GetAssetType(const AZStd::string& fileName) const override;
    };
}
