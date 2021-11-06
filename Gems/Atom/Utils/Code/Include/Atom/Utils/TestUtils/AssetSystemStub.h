/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace UnitTest
{
    /**
     * This class can stub out the asset system to enable the RPI's AssetUtils class to look up test assets.
     * This is included in the RPITestFixture class. To use it, first create a test asset using AssetManager::Create()
     * or one of the RPI's AssetCreators. This will register the asset with the asset database. Example:
     *
     *    Data::Asset<MaterialAsset> myTestMaterialAsset1;
     *    MaterialAssetCreator creator;
     *    creator.Begin(...);
     *    // set some stuff
     *    creator.End(myTestMaterialAsset1);
     *
     * Then call RegisterSourceInfo for whatever test assets you want to be able to access via AssetUtils.
     * This associates an AssetInfo object with some dummy source file name. No source file is actually created;
     * it just allows AssetUtils to find the desired AssetInfo when looking up a source file name that your unit 
     * test provides. Example: 
     *
     *    m_assetSystemStub.RegisterSourceInfo("MyTestMaterial1.material", Data::AssetInfo{ myTestMaterialAsset1.GetId() }, "");
     *    m_assetSystemStub.RegisterSourceInfo("MyTestMaterial2.material", Data::AssetInfo{ myTestMaterialAsset2.GetId() }, "");
     *
     * Now the RPI should be able to use AssetUtils like normal to access your test assets. Example:
     *
     *    RPI::AssetUtils::LoadAsset("MyTestMaterial1.material"); // Returns myTestMaterialAsset1
     */
    class AssetSystemStub : public AzToolsFramework::AssetSystemRequestBus::Handler
    {
    public:

        void Activate();
        void Deactivate();

        void RegisterSourceInfo(const char* sourcePath, const AZ::Data::AssetId& assetId);
        void RegisterSourceInfo(const char* sourcePath, const AZ::Data::AssetInfo& assetInfo, const AZStd::string& watchFolder);

    private:
        struct SourceInfo
        {
            AZ::Data::AssetInfo m_assetInfo;
            AZStd::string m_watchFolder;
        };

        AZStd::unordered_map<AZStd::string, SourceInfo> m_sourceInfoMap;

        bool GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override;
        bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& relativeProductPath) override;
        bool GenerateRelativeSourcePath(
            const AZStd::string& sourcePath, AZStd::string& relativePath, AZStd::string& watchFolder) override;
        bool GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullSourcePath) override;
        bool GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath) override;
        bool GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override;
        bool GetScanFolders(AZStd::vector<AZStd::string>& scanFolders) override;
        bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders) override;
        bool IsAssetPlatformEnabled(const char* platform) override;
        int GetPendingAssetsForPlatform(const char* platform) override;
        bool GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo) override;
    };
} // namespace UnitTest
