/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Network/SocketConnection.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>

namespace AzToolsFramework
{
    namespace AssetSystem
    {
        /**
        * A tools level component for interacting with the asset processor
        *
        * Currently used to translate between full and relative asset paths, 
        * and to query information about asset processor jobs
        */
        class AssetSystemComponent
            : public AZ::Component
            , private AzToolsFramework::AssetSystemRequestBus::Handler
            , private AzToolsFramework::AssetSystemJobRequestBus::Handler
            , private AzToolsFramework::AssetSystemBus::Handler
            , private AzToolsFramework::ToolsAssetSystemBus::Handler
            , private AZ::SystemTickBus::Handler
        {
        public:
            AZ_COMPONENT(AssetSystemComponent, "{B1352D59-945B-446A-A7E1-B2D3EB717C6D}")

            AssetSystemComponent() = default;
            virtual ~AssetSystemComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component overrides
            void Init() override {}
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        private:

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::AssetSystemRequestBus::Handler overrides
            bool GetAbsoluteAssetDatabaseLocation(AZStd::string& result) override;
            bool GetRelativeProductPathFromFullSourceOrProductPath(const AZStd::string& fullPath, AZStd::string& outputPath) override;
            bool GenerateRelativeSourcePath(
                const AZStd::string& sourcePath, AZStd::string& outputPath, AZStd::string& watchFolder) override;
            bool GetFullSourcePathFromRelativeProductPath(const AZStd::string& relPath, AZStd::string& fullPath) override;
            bool GetAssetInfoById(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType, const AZStd::string& platformName, AZ::Data::AssetInfo& assetInfo, AZStd::string& rootFilePath) override;
            bool GetSourceInfoBySourcePath(const char* sourcePath, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override;
            bool GetSourceInfoBySourceUUID(const AZ::Uuid& sourceUuid, AZ::Data::AssetInfo& assetInfo, AZStd::string& watchFolder) override;
            bool GetScanFolders(AZStd::vector<AZStd::string>& scanFolders) override;
            bool GetAssetSafeFolders(AZStd::vector<AZStd::string>& assetSafeFolders) override;
            bool IsAssetPlatformEnabled(const char* platform) override;
            int GetPendingAssetsForPlatform(const char* platform) override;
            bool GetAssetsProducedBySourceUUID(const AZ::Uuid& sourceUuid, AZStd::vector<AZ::Data::AssetInfo>& productsAssetInfo) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::AssetSystemJobRequest::Bus::Handler overrides
            virtual AZ::Outcome<AssetSystem::JobInfoContainer> GetAssetJobsInfo(const AZStd::string& path, const bool escalateJobs) override;
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfoByAssetID(const AZ::Data::AssetId& assetId, const bool escalateJobs, bool requireFencing) override;
            virtual AZ::Outcome<JobInfoContainer> GetAssetJobsInfoByJobKey(const AZStd::string& jobKey, const bool escalateJobs) override;
            virtual AZ::Outcome<JobStatus> GetAssetJobsStatusByJobKey(const AZStd::string& jobKey, const bool escalateJobs) override;
            virtual AZ::Outcome<AZStd::string> GetJobLog(AZ::u64 jobrunkey) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::::AssetSystemBus::Handler overrides
            void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
            void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
            void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AzToolsFramework::ToolsAssetSystemBus::Handler overrides
            void RegisterSourceAssetType(const AZ::Data::AssetType& assetType, const char* assetFileFilter) override;
            void UnregisterSourceAssetType(const AZ::Data::AssetType& assetType) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // SystemTickBus::Handler overrides
            void OnSystemTick() override;
            //////////////////////////////////////////////////////////////////////////

            AzFramework::SocketConnection::TMessageCallbackHandle m_cbHandle = 0;
            AzFramework::SocketConnection::TMessageCallbackHandle m_showAssetBrowserCBHandle = 0;
            AzFramework::SocketConnection::TMessageCallbackHandle m_wantShowAssetBrowserCBHandle = 0;

            AZStd::unordered_map<AZStd::string, AZStd::string> m_assetSourceRelativePathToFullPathCache;
        };


    } // namespace AssetSystem
} // namespace AzToolsFramework

