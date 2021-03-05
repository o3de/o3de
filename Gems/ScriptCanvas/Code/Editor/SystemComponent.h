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

namespace AZ
{
    class JobManager;
    class JobContext;
}

namespace ScriptCanvasEditor
{
    class SystemComponent
        : public AZ::Component
        , private SystemRequestBus::Handler
        , private AzToolsFramework::EditorEvents::Bus::Handler
        , private AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
        , private ScriptCanvasExecutionBus::Handler
        , private AZ::UserSettingsNotificationBus::Handler
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
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // SystemRequestBus::Handler        
        void AddAsyncJob(AZStd::function<void()>&& jobFunc) override;
        void GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes);
        void CreateEditorComponentsOnEntity(AZ::Entity* entity, const AZ::Data::AssetType& assetType) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AztoolsFramework::EditorEvents::Bus::Handler overrides
        void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
        void NotifyRegisterViews() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ScriptCanvasExecutionBus::Handler
        Reporter RunGraph(AZStd::string_view path) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        //  AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus overrides
        AzToolsFramework::AssetBrowser::SourceFileDetails GetSourceFileDetails(const char* fullSourceFileName) override;
        void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::UserSettingsNotificationBus::Handler
        void OnUserSettingsActivated();
        ////////////////////////////////////////////////////////////////////////

    private:
        SystemComponent(const SystemComponent&) = delete;

        void FilterForScriptCanvasEnabledEntities(AzToolsFramework::EntityIdList& sourceList, AzToolsFramework::EntityIdList& targetList);
        void PopulateEditorCreatableTypes();

        AZStd::unique_ptr<AZ::JobManager> m_jobManager;
        AZStd::unique_ptr<AZ::JobContext> m_jobContext;

        AZStd::vector<AZStd::unique_ptr<AzToolsFramework::PropertyHandlerBase>> m_propertyHandlers;
        AZStd::unordered_set<ScriptCanvas::Data::Type> m_creatableTypes;

        AssetTracker m_assetTracker;
    };
}
