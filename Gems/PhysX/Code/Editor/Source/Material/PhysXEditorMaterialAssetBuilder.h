/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <PhysX/Material/PhysXMaterialConfiguration.h>

namespace Physics
{
    class MaterialAsset;
}

namespace PhysX
{
    class EditorMaterialAsset;

    //! Builder to convert PhysX Editor Material assets in the
    //! source folder into Physics Material assets in the cache folder.
    class EditorMaterialAssetBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(PhysX::EditorMaterialAssetBuilder, "{981E4C8F-CEBB-42B1-B5D5-82B1F815798B}");

        EditorMaterialAssetBuilder() = default;

        // Asset Builder Callback Functions...
        void CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response) const;
        void ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response) const;

        // AssetBuilderSDK::AssetBuilderCommandBus overrides...
        void ShutDown() override { }

    private:
        AZ::Data::Asset<EditorMaterialAsset> LoadEditorMaterialAsset(const AZStd::string& assetFullPath) const;

        bool SerializeOutPhysicsMaterialAsset(
            AZ::Data::Asset<Physics::MaterialAsset> physicsMaterialAsset,
            const AssetBuilderSDK::ProcessJobRequest& request,
            AssetBuilderSDK::ProcessJobResponse& response) const;
    };
} // PhysX
