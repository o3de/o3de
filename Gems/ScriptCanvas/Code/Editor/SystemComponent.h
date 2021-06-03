/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Core/GraphBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasExecutionBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <Editor/Assets/ScriptCanvasAssetTracker.h>

#include <AzToolsFramework/Asset/AssetSeedManager.h>

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>

namespace ScriptCanvasEditor
{
    class SystemComponent
        : public AZ::Component
        , private SystemRequestBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , private ScriptCanvasExecutionBus::Handler
        , private AZ::UserSettingsNotificationBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
        , private AZ::Interface<IUpgradeRequests>::Registrar
        , private AzToolsFramework::AssetSeedManagerRequests::Bus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{1DE7A120-4371-4009-82B5-8140CB1D7B31}");

        SystemComponent();
        ~SystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // SystemRequestBus::Handler...
        void AddAsyncJob(AZStd::function<void()>&& jobFunc) override;
        void GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes);
        void CreateEditorComponentsOnEntity(AZ::Entity* entity, const AZ::Data::AssetType& assetType) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorEvents::Bus::Handler...
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ScriptCanvasExecutionBus::Handler...
        Reporter RunAssetGraph(AZ::Data::Asset<AZ::Data::AssetData>, ScriptCanvas::ExecutionMode mode) override;
        Reporter RunGraph(AZStd::string_view path, ScriptCanvas::ExecutionMode mode) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        //  AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus...
        AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
        void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::UserSettingsNotificationBus::Handler...
        void OnUserSettingsActivated() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus::Handler...
        void OnCatalogLoaded(const char* /*catalogFile*/) override;
        void OnCatalogAssetAdded(const AZ::Data::AssetId& /*assetId*/) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& /*assetId*/, const AZ::Data::AssetInfo& /*assetInfo*/) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AssetSeedManagerRequests::Bus::Handler...
        AzToolsFramework::AssetSeedManagerRequests::AssetTypePairs GetAssetTypeMapping() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // IUpgradeRequests...
        IUpgradeRequests::AssetList& GetAssetsToUpgrade() override
        {
            return m_assetsToConvert;
        }

        void GraphNeedsManualUpgrade(const AZ::Data::AssetId& assetId) override
        {
            if (AZStd::find(m_assetsThatNeedManualUpgrade.begin(), m_assetsThatNeedManualUpgrade.end(), assetId) == m_assetsThatNeedManualUpgrade.end())
            {
                m_assetsThatNeedManualUpgrade.push_back(assetId);
            }
        }

        AZStd::vector<AZ::Data::AssetId>& GetGraphsThatNeedManualUpgrade() override
        {
            return m_assetsThatNeedManualUpgrade;
        }

        bool IsUpgrading() override
        {
            return m_isUpgrading;
        }

        void SetIsUpgrading(bool isUpgrading) override
        {
            m_isUpgrading = isUpgrading;
        }

        ////////////////////////////////////////////////////////////////////////

    private:
        SystemComponent(const SystemComponent&) = delete;

        void FilterForScriptCanvasEnabledEntities(AzToolsFramework::EntityIdList& sourceList, AzToolsFramework::EntityIdList& targetList);
        void PopulateEditorCreatableTypes();
        void AddAssetToUpgrade(const AZ::Data::AssetInfo& assetInfo);

        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;

        AZStd::unordered_set<ScriptCanvas::Data::Type> m_creatableTypes;

        AssetTracker m_assetTracker;

        IUpgradeRequests::AssetList m_assetsToConvert;
        AZStd::vector<AZ::Data::AssetId> m_assetsThatNeedManualUpgrade;

        bool m_isUpgrading = false;
        bool m_upgradeDisabled = false;

    };
}
