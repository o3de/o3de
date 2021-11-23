/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace ScriptCanvasEditor
{
    /*! EditorScriptCanvasComponent
    The user facing Editor Component for interfacing with ScriptCanvas
    It connects to the AssetCatalogEventBus in order to remove the ScriptCanvasAssetHolder asset reference
    when the asset is removed from the file system. The reason the ScriptCanvasAssetHolder holder does not
    remove the asset reference itself is because the ScriptCanvasEditor MainWindow has a ScriptCanvasAssetHolder
    which it uses to maintain the asset data in memory. Therefore removing an open ScriptCanvasAsset from the file system
    will remove the reference from the EditorScriptCanvasComponent, but not the reference from the MainWindow allowing the
    ScriptCanvas graph to still be modified while open
    Finally per graph instance variables values are stored on the EditorScriptCanvasComponent and injected into the runtime ScriptCanvas component in BuildGameEntity
    */
    class EditorScriptCanvasComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , private EditorContextMenuRequestBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
        , private AzToolsFramework::AssetSystemBus::Handler
        , private EditorScriptCanvasComponentLoggingBus::Handler
        , private EditorScriptCanvasComponentRequestBus::Handler
        , private AzToolsFramework::EditorEntityContextNotificationBus::Handler

    {
    public:
        AZ_COMPONENT(EditorScriptCanvasComponent, "{C28E2D29-0746-451D-A639-7F113ECF5D72}", AzToolsFramework::Components::EditorComponentBase);

        EditorScriptCanvasComponent();
        EditorScriptCanvasComponent(const SourceHandle& sourceHandle);
        ~EditorScriptCanvasComponent() override;

        // sets the soure but does not attempt to load anything;
        void InitializeSource(const SourceHandle& sourceHandle);

        //=====================================================================
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //=====================================================================

        //=====================================================================
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        void SetPrimaryAsset(const AZ::Data::AssetId&) override;
        //=====================================================================

        //=====================================================================
        // EditorScriptCanvasComponentLoggingBus
        AZ::NamedEntityId FindNamedEntityId() const override { return GetNamedEntityId(); }
        ScriptCanvas::GraphIdentifier GetGraphIdentifier() const override;
        //=====================================================================

        void OpenEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&);
        
        void SetName(AZStd::string_view name) { m_name = name; }
        const AZStd::string& GetName() const;
        AZ::EntityId GetEditorEntityId() const { return GetEntity() ? GetEntityId() : AZ::EntityId(); }
        AZ::NamedEntityId GetNamedEditorEntityId() const { return GetEntity() ? GetNamedEntityId() : AZ::NamedEntityId(); }

        //=====================================================================
        // EditorScriptCanvasComponentRequestBus
        void SetAssetId(const SourceHandle& assetId) override;
        bool HasAssetId() const override;
        //=====================================================================

        //=====================================================================
        // EditorContextMenuRequestBus
        AZ::Data::AssetId GetAssetId() const override;
        //=====================================================================
        
       


        //=====================================================================
        // EditorEntityContextNotificationBus
        void OnStartPlayInEditor() override;

        void OnStopPlayInEditor() override;

    protected:
        enum class SourceChangeDescription : AZ::u8
        {
            Error,
            Modified,
            Removed,
            SelectionChanged,
        };

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            (void)required;
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            (void)incompatible;
        }

        // complete the id, load call OnScriptCanvasAssetChanged
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        // update the display icon for failure, save the values in the graph
        void SourceFileRemoved(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;
        void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid fileAssetId) override;

        AZ::u32 OnFileSelectionChanged();

        void OnScriptCanvasAssetChanged(SourceChangeDescription changeDescription);

        void UpdateName();

        //=====================================================================
        void UpdatePropertyDisplay(const SourceHandle& sourceHandle);
        //=====================================================================

        void BuildGameEntityData();
        void ClearVariables();

    private:
        AZStd::string m_name;
        bool m_runtimeDataIsValid = false;
        ScriptCanvasBuilder::BuildVariableOverrides m_variableOverrides;
        SourceHandle m_sourceHandle;
        SourceHandle m_previousHandle;
        SourceHandle m_removedHandle;
    };
}
