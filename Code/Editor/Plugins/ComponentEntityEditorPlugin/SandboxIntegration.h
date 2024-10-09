/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H
#define CRYINCLUDE_COMPONENTENTITYEDITORPLUGIN_SANDBOXINTEGRATION_H

#include "ContextMenuHandlers.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationManager.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
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

namespace AzToolsFramework
{
    class EditorEntityAPI;
    class ReadOnlyEntityPublicInterface;

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
    , private AzToolsFramework::EditorWindowRequests::Bus::Handler
    , private AzToolsFramework::EditorEntityContextNotificationBus::Handler
    , private IUndoManagerListener
    , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
{
public:

    SandboxIntegrationManager();
    ~SandboxIntegrationManager();

    void Setup();
    void Teardown();

private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::ToolsApplicationEvents::Bus::Handler overrides
    void OnBeginUndo(const char* label) override;
    void OnEndUndo(const char* label, bool changed) override;
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
    void CloneSelection(bool& handled) override;
    void DeleteSelectedEntities(bool includeDescendants) override;
    AZ::EntityId CreateNewEntity(AZ::EntityId parentId = AZ::EntityId()) override;
    AZ::EntityId CreateNewEntityAsChild(AZ::EntityId parentId) override;
    AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& /*pos*/, AZ::EntityId parentId = AZ::EntityId()) override;
    AzFramework::EntityContextId GetEntityContextId() override;
    QWidget* GetMainWindow() override;
    IEditor* GetEditor() override;
    void LaunchLuaEditor(const char* files) override;
    bool IsLevelDocumentOpen() override;
    AZStd::string GetLevelName() override;
    void OpenPinnedInspector(const AzToolsFramework::EntityIdSet& entities) override;
    void ClosePinnedInspector(AzToolsFramework::EntityPropertyEditor* editor) override;
    void GoToSelectedOrHighlightedEntitiesInViewports() override;
    void GoToSelectedEntitiesInViewports() override;
    bool CanGoToSelectedEntitiesInViewports() override;
    AZ::Vector3 GetWorldPositionAtViewportCenter() override;
    AZ::Vector3 GetWorldPositionAtViewportInteraction() const override;
    void ClearRedoStack() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorWindowRequests::Bus::Handler
    QWidget* GetAppMainWindow() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityContextNotificationBus::Handler
    void OnPrepareForContextReset() override;
    //////////////////////////////////////////////////////////////////////////

    // ActionManagerRegistrationNotificationBus overrides ...
    void OnActionRegistrationHook() override;
    void OnMenuBindingHook() override;

    // Context menu handlers.
    void ContextMenu_NewEntity();
    void ContextMenu_Duplicate();
    void ContextMenu_DeleteSelected();

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

    AZStd::string GetComponentEditorIcon(const AZ::Uuid& componentType, const AZ::Component* component) override;
    AZStd::string GetComponentTypeEditorIcon(const AZ::Uuid& componentType) override;
    AZStd::string GetComponentIconPath(const AZ::Uuid& componentType, AZ::Crc32 componentIconAttrib, const AZ::Component* component) override;

    //////////////////////////////////////////////////////////////////////////
    // IUndoManagerListener
    // Listens for Cry Undo System events.
    void UndoStackFlushed() override;

private:
    void GoToEntitiesInViewports(const AzToolsFramework::EntityIdList& entityIds);
    bool CanGoToEntityOrChildren(const AZ::EntityId& entityId) const;

private:
    EditorContextMenuHandler m_contextMenuBottomHandler;

    //! Position of the cursor when the context menu is opened inside the 3d viewport.
    //! note: The optional will be empty if the context menu was opened outside the 3d viewport.
    AZStd::optional<AzFramework::ScreenPoint> m_contextMenuViewPoint;

    short m_startedUndoRecordingNestingLevel; //!< Used in OnBegin/EndUndo to ensure we only accept undos we started recording

    const AZStd::string m_defaultComponentIconLocation = "Icons/Components/Component_Placeholder.svg";
    const AZStd::string m_defaultComponentViewportIconLocation = "Icons/Components/Viewport/Component_Placeholder.svg";
    const AZStd::string m_defaultEntityIconLocation = "Icons/Components/Viewport/Transform.svg";

    AzToolsFramework::EditorEntityAPI* m_editorEntityAPI = nullptr;
    AzToolsFramework::Prefab::PrefabIntegrationManager* m_prefabIntegrationManager = nullptr;
    AzToolsFramework::Prefab::PrefabIntegrationInterface* m_prefabIntegrationInterface = nullptr;
    AzToolsFramework::ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;
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
