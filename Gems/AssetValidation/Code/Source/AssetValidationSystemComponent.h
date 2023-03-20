/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>

#include <AssetValidation/AssetValidationBus.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Asset/AssetSeedList.h>
#include <AzCore/Outcome/Outcome.h>
#include <AssetSystemTestCommands.h>

#include <CrySystemBus.h>
#include <AzFramework/Archive/ArchiveBus.h>

struct IConsoleCmdArgs; 

namespace AssetValidation
{
    class AssetValidationSystemComponent
        : public AZ::Component
        , protected AssetValidationRequestBus::Handler
        , public CrySystemEventBus::Handler
        , public AZ::IO::ArchiveNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(AssetValidationSystemComponent, "{BF122D5A-17B3-46B9-880B-39026989CD7E}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AssetValidationRequestBus interface implementation
        // Given an asset fileName does it exist in our dependency graph beneath any of our declared seed assetIds
        bool IsKnownAsset(const char* fileName) override;

        bool CheckKnownAsset(const char* fileName) override;

        bool AddSeedAssetId(AZ::Data::AssetId assetId, AZ::u32 sourceId) override;

        bool RemoveSeedAssetId(AZ::Data::AssetId assetId, AZ::u32 sourceId) override;

        bool RemoveSeedAssetIdList(AssetSourceList assetList) override;

        void SeedMode() override;

        bool AddSeedPath(const char* fileName) override;
       
        bool RemoveSeedPath(const char* fileName) override;

        void ListKnownAssets() override;

        void TogglePrintExcluded() override;

        virtual AZ::Outcome<AzFramework::AssetSeedList, AZStd::string> LoadSeedList(const char* fileName, AZStd::string& seedFilepath);
        bool RemoveSeedListHelper(const char* seedPath);
        ////////////////////////////////////////////////////////////////////////

        void BuildAssetList();
        void AddKnownAssets(AZ::Data::AssetId assetId);

        //! Return the number of occurrences of that assetID remaining, -1 for failure
        int RemoveSeedAssetIdBySource(const AZ::Data::AssetId& assetId, AZ::u32 sourceId);

        static void ConsoleCommandSeedMode(IConsoleCmdArgs* pCmdArgs);
        static void ConsoleCommandAddSeedPath(IConsoleCmdArgs* pCmdArgs);
        static void ConsoleCommandRemoveSeedPath(IConsoleCmdArgs* pCmdArgs);
        static void ConsoleCommandKnownAssets(IConsoleCmdArgs* pCmdArgs);
        static void ConsoleCommandTogglePrintExcluded(IConsoleCmdArgs* pCmdArgs);

        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ArchiveNotificationBus interface implementation
        void FileAccess(const char* filePath) override /*override*/;
        ////////////////////////////////////////////////////////////////////////

        bool AddSeedList(const char* seedPath) override;
        bool RemoveSeedList(const char* seedPath) override;
        ////////////////////////////////////////////////////////////////////////

        static void ConsoleCommandAddSeedList(IConsoleCmdArgs* pCmdArgs);
        static void ConsoleCommandRemoveSeedList(IConsoleCmdArgs* pCmdArgs);

        bool AddSeedsFor(const AzFramework::AssetSeedList& seedList, AZ::u32 sourceId);
        bool RemoveSeedsFor(const AzFramework::AssetSeedList& seedList, AZ::u32 sourceId);
        bool AddSeedListHelper(const char* seedPath);
    private:
        
        bool m_seedMode{ false };
        bool m_printExcluded{ false };

        AZStd::unordered_map<AZ::Data::AssetId, AZStd::set<AZ::u32>> m_seedAssetIds;
        AZStd::set<AZ::Data::AssetId> m_knownAssetIds;
        AZStd::set<AZStd::string> m_knownAssetPaths;
        AZStd::vector<AZStd::string> m_excludedFileTags;
        AZStd::set<AZStd::string> m_seedLists;
        AZStd::set<AZStd::string> m_assetSet;

        // Auto registration of Asset System Test Commands
        AssetValidation m_testCommands;
    };
}
