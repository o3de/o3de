/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H
#define CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Slice/SliceBus.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/DisplayContextRequestBus.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Editor/EditorContextMenuBus.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/Layer/LayerUiHandler.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationManager.h>
#include <AzToolsFramework/UI/Slice/SliceOverridesNotificationWindowManager.hxx>
#include <AzToolsFramework/UI/Slice/SliceOverridesNotificationWindow.hxx>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

// Sandbox imports.
#include "../Editor/ViewManager.h"
#include "../Editor/Viewport.h"
#include "../Editor/Undo/IUndoManagerListener.h"
#include "../Editor/Undo/IUndoObject.h"

#include <QApplication>
#include <QPointer>

/**
* Integration of ToolsApplication behavior and Cry undo/redo and selection systems
* with respect to component entity operations.
*
* Undo/Redo
* - CToolsApplicationUndoLink represents a component application undo operation within
*   the Sandbox undo system. When an undo-able component operation is performed, we
*   intercept ToolsApplicationEventBus::OnBeginUndo()/OnEndUndo() events, and in turn
*   create and register a link instance.
* - When the user attempts to undo/redo a CToolsApplicationUndoLink event in Sandbox,
*   CToolsApplicationUndoLink::Undo()/Redo() is invoked, and the request is passed
*   to the component application via ToolsApplicationRequestBus::OnUndoPressed/OnRedoPressed,
*   where restoration of the previous entity snapshot is handled.
*
* AzToolsFramework::ToolsApplication Extensions
* - Provides engine UI customizations, such as using the engine's built in asset browser
*   when assigning asset references to component properties.
* - Handles component edit-time display requests (using the editor's drawing context).
* - Handles source control requests from AZ components or component-related UI.
*/

namespace AZ::Data
{
    class AssetInfo;
}

class CToolsApplicationUndoLink;

class QMenu;
class QWidget;
class CComponentEntityObject;
class CHyperGraph;

namespace AzToolsFramework
{
    class EditorEntityAPI;
    class EditorEntityUiInterface;

    namespace AssetBrowser
    {
        class AssetSelectionModel;
    }

    namespace Prefab
    {
        class PrefabIntegrationInterface;
    }
}

//////////////////////////////////////////////////////////////////////////

class SandboxIntegrationManager
    : private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
    , private AzToolsFramework::EditorRequests::Bus::Handler
    , private AzToolsFramework::EditorContextMenuBus::Handler
    , private AzToolsFramework::EditorWindowRequests::Bus::Handler
    , private AzFramework::AssetCatalogEventBus::Handler
    , private AzFramework::DisplayContextRequestBus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler
    , private IUndoManagerListener
    , private AzToolsFramework::Layers::EditorLayerComponentNotificationBus::Handler
{
public:

    SandboxIntegrationManager();
    ~SandboxIntegrationManager();

    void Setup();
    void Teardown();

private:

    //////////////////////////////////////////////////////////////////////////
    // AssetCatalogEventBus::Handler
    void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
    void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::ToolsApplicationEvents::Bus::Handler overrides
    void OnBeginUndo(const char* label) override;
    void OnEndUndo(const char* label, bool changed) override;
    void EntityParentChanged(
        AZ::EntityId entityId,
        AZ::EntityId newParentId,
        AZ::EntityId oldParentId) override;
    void OnSaveLevel() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorRequests::Bus::Handler overrides
    void RegisterViewPane(const char* name, const char* category, const AzToolsFramework::ViewPaneOptions& viewOptions, const WidgetCreationFunc& widgetCreationFunc) override;
    void RegisterCustomViewPane(const char* name, const char* category, const AzToolsFramework::ViewPaneOptions& viewOptions) override;
    void UnregisterViewPane(const char* name) override;
    QWidget* GetViewPaneWidget(const char* viewPaneName) override;
    void OpenViewPane(const char* paneName) override;
    QDockWidget* InstanceViewPane(const char* paneName) override;
    void CloseViewPane(const char* paneName) override;
    void BrowseForAssets(AzToolsFramework::AssetBrowser::AssetSelectionModel& selection) override;
    void CreateEditorRepresentation(AZ::Entity* entity) override;
    bool DestroyEditorRepresentation(AZ::EntityId entityId, bool deleteAZEntity) override;
    void CloneSelection(bool& handled) override;
    void DeleteSelectedEntities(bool includeDescendants) override;
    AZ::EntityId CreateNewEntity(AZ::EntityId parentId = AZ::EntityId()) override;
    AZ::EntityId CreateNewEntityAsChild(AZ::EntityId parentId) override;
    AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& /*pos*/, AZ::EntityId parentId = AZ::EntityId()) override;
    AzFramework::EntityContextId GetEntityContextId() override;
    QWidget* GetMainWindow() override;
    IEditor* GetEditor() override;
    bool GetUndoSliceOverrideSaveValue() override;
    bool GetShowCircularDependencyError() override;
    void SetShowCircularDependencyError(const bool& showCircularDependencyError) override;
    void LaunchLuaEditor(const char* files) override;
    bool IsLevelDocumentOpen() override;
    AZStd::string GetLevelName() override;
    void OpenPinnedInspector(const AzToolsFramework::EntityIdSet& entities) override;
    void ClosePinnedInspector(AzToolsFramework::EntityPropertyEditor* editor) override;
    void GoToSelectedOrHighlightedEntitiesInViewports() override;
    void GoToSelectedEntitiesInViewports() override;
    bool CanGoToSelectedEntitiesInViewports() override;
    AZ::Vector3 GetWorldPositionAtViewportCenter() override;
    void InstantiateSliceFromAssetId(const AZ::Data::AssetId& assetId) override;
    void ClearRedoStack() override;
    int GetIconTextureIdFromEntityIconPath(const AZStd::string& entityIconPath) override;
    bool DisplayHelpersVisible() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorWindowRequests::Bus::Handler
    QWidget* GetAppMainWindow() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorContextMenu::Bus::Handler overrides
    void PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags) override;
    int GetMenuPosition() const override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityContextNotificationBus::Handler
    void OnContextReset() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    /// AzToolsFramework::SliceEditorEntityOwnershipServiceNotificationBus::Handler
    void OnSliceInstantiated(
        const AZ::Data::AssetId& sliceAssetId,
        AZ::SliceComponent::SliceInstanceAddress& sliceAddress,
        const AzFramework::SliceInstantiationTicket& ticket) override;
    //////////////////////////////////////////////////////////////////////////

    // AzFramework::DisplayContextRequestBus (and @deprecated EntityDebugDisplayRequestBus)
    // AzFramework::DisplayContextRequestBus
    void SetDC(DisplayContext* dc) override;
    DisplayContext* GetDC() override;

    // Context menu handlers.
    void ContextMenu_NewEntity();
    AZ::EntityId ContextMenu_NewLayer();
    void ContextMenu_SaveLayers(const AZStd::unordered_set<AZ::EntityId>& layers);
    void ContextMenu_MakeSlice(AzToolsFramework::EntityIdList entities);
    void ContextMenu_InheritSlice(AzToolsFramework::EntityIdList entities);
    void ContextMenu_InstantiateSlice();
    void ContextMenu_SelectSlice();
    void ContextMenu_PushEntitiesToSlice(AzToolsFramework::EntityIdList entities,
        AZ::SliceComponent::EntityAncestorList ancestors,
        AZ::Data::AssetId targetAncestorId,
        bool affectEntireHierarchy);
    void ContextMenu_Duplicate();
    void ContextMenu_DeleteSelected();
    void ContextMenu_ResetToSliceDefaults(AzToolsFramework::EntityIdList entities);

    void MakeSliceFromEntities(const AzToolsFramework::EntityIdList& entities, bool inheritSlices, bool setAsDynamic);

    void GetSelectedEntities(AzToolsFramework::EntityIdList& entities);
    void GetSelectedOrHighlightedEntities(AzToolsFramework::EntityIdList& entities);

    AZStd::string GetDefaultComponentViewportIcon() override
    {
        return m_defaultComponentViewportIconLocation;
    }

    AZStd::string GetDefaultComponentEditorIcon() override
    {
        return m_defaultComponentIconLocation;
    }

    AZStd::string GetDefaultEntityIcon() override
    {
        return m_defaultEntityIconLocation;
    }

    AZStd::string GetComponentEditorIcon(const AZ::Uuid& componentType, AZ::Component* component) override;
    AZStd::string GetComponentTypeEditorIcon(const AZ::Uuid& componentType) override;
    AZStd::string GetComponentIconPath(const AZ::Uuid& componentType, AZ::Crc32 componentIconAttrib, AZ::Component* component) override;

    //////////////////////////////////////////////////////////////////////////
    // IUndoManagerListener
    // Listens for Cry Undo System events.
    void UndoStackFlushed() override;

    // EditorLayerRequestBus...
    void OnLayerComponentActivated(AZ::EntityId entityId) override;
    void OnLayerComponentDeactivated(AZ::EntityId entityId) override;

private:
    // Right click context menu when a layer is included in the selection.
    void SetupLayerContextMenu(QMenu* menu);
    void SetupSliceContextMenu(QMenu* menu);
    void SetupSliceContextMenu_Modify(QMenu* menu, const AzToolsFramework::EntityIdList& selectedEntities, const AZ::u32 numEntitiesInSlices);
    void SaveSlice(const bool& QuickPushToFirstLevel);
    void GetEntitiesInSlices(const AzToolsFramework::EntityIdList& selectedEntities, AZ::u32& entitiesInSlices, AZStd::vector<AZ::SliceComponent::SliceInstanceAddress>& sliceInstances);

    void GoToEntitiesInViewports(const AzToolsFramework::EntityIdList& entityIds);

    bool CanGoToEntityOrChildren(const AZ::EntityId& entityId) const;

    // This struct exists to help handle the error case where slice assets are 
    // accidentally deleted from disk but their instances are still in the editing level.
    struct SliceAssetDeletionErrorInfo
    {
        SliceAssetDeletionErrorInfo() = default;

        SliceAssetDeletionErrorInfo(AZ::Data::AssetId assetId, AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityRestoreInfo>>&& entityRestoreInfos) 
            : m_assetId(assetId)
            , m_entityRestoreInfos(AZStd::move(entityRestoreInfos))
        { }

        AZ::Data::AssetId m_assetId;
        AZStd::vector<AZStd::pair<AZ::EntityId, AZ::SliceComponent::EntityRestoreInfo>> m_entityRestoreInfos;
    };

private:
    AZ::Vector2 m_contextMenuViewPoint;

    short m_startedUndoRecordingNestingLevel;   // used in OnBegin/EndUndo to ensure we only accept undo's we started recording

    AzToolsFramework::SliceOverridesNotificationWindowManager* m_notificationWindowManager;

    DisplayContext* m_dc;

    AZStd::vector<SliceAssetDeletionErrorInfo> m_sliceAssetDeletionErrorRestoreInfos;

    // Tracks new entities that have not yet been saved.
    AZStd::unordered_set<AZ::EntityId> m_unsavedEntities;

    const AZStd::string m_defaultComponentIconLocation = "Icons/Components/Component_Placeholder.svg";
    const AZStd::string m_defaultComponentViewportIconLocation = "Icons/Components/Viewport/Component_Placeholder.svg";
    const AZStd::string m_defaultEntityIconLocation = "Icons/Components/Viewport/Transform.svg";

    AzToolsFramework::Prefab::PrefabIntegrationManager* m_prefabIntegrationManager = nullptr;

    AzToolsFramework::EditorEntityUiInterface* m_editorEntityUiInterface = nullptr;
    AzToolsFramework::Prefab::PrefabIntegrationInterface* m_prefabIntegrationInterface = nullptr;
    AzToolsFramework::EditorEntityAPI* m_editorEntityAPI = nullptr;

    // Overrides UI styling and behavior for Layer Entities
    AzToolsFramework::LayerUiHandler m_layerUiOverrideHandler;
};

//////////////////////////////////////////////////////////////////////////
class CToolsApplicationUndoLink
    : public IUndoObject
{
public:

    CToolsApplicationUndoLink()
    {
    }

    int GetSize() override
    {
        return 0;
    }

    void Undo(bool bUndo = true) override
    {
        // Always run the undo even if the flag was set to false, that just means that undo wasn't expressly desired, but can be used in cases of canceling the current super undo.

        // Restore previous focus after applying the undo.
        QPointer<QWidget> w = QApplication::focusWidget();

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::UndoPressed);

        // Slice the redo stack if this wasn't due to explicit undo command
        if (!bUndo)
        {
            AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::FlushRedo);
        }

        if (!w.isNull())
        {
            w->setFocus(Qt::OtherFocusReason);
        }
    }

    void Redo() override
    {
        // Restore previous focus after applying the undo.
        QPointer<QWidget> w = QApplication::focusWidget();

        AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::RedoPressed);

        if (!w.isNull())
        {
            w->setFocus(Qt::OtherFocusReason);
        }
    }
};

#endif // CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H
