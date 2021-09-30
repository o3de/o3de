
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Asset/AssetSeedManager.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <Core/GraphBus.h>
#include <Editor/Assets/ScriptCanvasAssetTracker.h>
#include <Editor/View/Windows/Tools/UpgradeTool/Model.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <ScriptCanvas/Bus/ScriptCanvasExecutionBus.h>

namespace ScriptCanvasEditor
{
    class SystemComponent
        : public AZ::Component
        , private SystemRequestBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , private ScriptCanvasExecutionBus::Handler
        , private AZ::UserSettingsNotificationBus::Handler
        , private AZ::Data::AssetBus::MultiHandler
        , private AzToolsFramework::AssetSeedManagerRequests::Bus::Handler
        , private AzToolsFramework::EditorContextMenuBus::Handler
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
        void GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes) override;
        void CreateEditorComponentsOnEntity(AZ::Entity* entity, const AZ::Data::AssetType& assetType) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorContextMenuBus::Handler...
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorEvents::Bus::Handler...
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
        // AssetSeedManagerRequests::Bus::Handler...
        AzToolsFramework::AssetSeedManagerRequests::AssetTypePairs GetAssetTypeMapping() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        
    private:
        SystemComponent(const SystemComponent&) = delete;

        void FilterForScriptCanvasEnabledEntities(AzToolsFramework::EntityIdList& sourceList, AzToolsFramework::EntityIdList& targetList);
        void PopulateEditorCreatableTypes();
        
        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;
        AZStd::unique_ptr<VersionExplorer::Model> m_versionExplorer;

        AZStd::unordered_set<ScriptCanvas::Data::Type> m_creatableTypes;

        AssetTracker m_assetTracker;

        AZStd::vector<AZ::Data::AssetId> m_assetsThatNeedManualUpgrade;

        bool m_isUpgrading = false;
        bool m_upgradeDisabled = false;
    };
}
