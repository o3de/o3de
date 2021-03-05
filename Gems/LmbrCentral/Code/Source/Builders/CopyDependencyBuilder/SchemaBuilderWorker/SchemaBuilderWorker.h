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
