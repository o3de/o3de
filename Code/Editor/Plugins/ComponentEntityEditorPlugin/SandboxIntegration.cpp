/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SandboxIntegration.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/numeric.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Visibility/BoundsBus.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorEntityAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorActionUpdaterIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerUtils.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Undo/UndoSystem.h>
#include <AzToolsFramework/UI/Prefab/PrefabIntegrationInterface.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>
#include <AzToolsFramework/ViewportSelection/EditorHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <MathConversion.h>

#include <Atom/ImageProcessing/ImageProcessingDefines.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h>

#include "UI/QComponentEntityEditorMainWindow.h"

#include <LmbrCentral/Scripting/TagComponentBus.h>
#include <LmbrCentral/Scripting/EditorTagComponentBus.h>

// Sandbox imports.
#include <Editor/CryEditDoc.h>
#include <Editor/GameEngine.h>
#include <Editor/DisplaySettings.h>
#include <Editor/Settings.h>
#include <Editor/QtViewPaneManager.h>
#include <Editor/EditorViewportSettings.h>
#include <Editor/EditorViewportCamera.h>
#include <Editor/Util/PathUtil.h>
#include "CryEdit.h"
#include "Undo/Undo.h"

#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QHBoxLayout>
#include "MainWindow.h"

#include <AzCore/std/algorithm.h>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

//////////////////////////////////////////////////////////////////////////
// Gathers all selected entities, culling any that have an ancestor in the selection.
void GetSelectedEntitiesSetWithFlattenedHierarchy(AzToolsFramework::EntityIdSet& out)
{
    AzToolsFramework::EntityIdList entities;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        entities,
        &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    for (const AZ::EntityId& entityId : entities)
    {
        bool selectionIncludesTransformHeritage = false;
        AZ::EntityId parent = entityId;
        do
        {
            AZ::EntityId nextParentId;
            AZ::TransformBus::EventResult(
                /*result*/ nextParentId,
                /*address*/ parent,
                &AZ::TransformBus::Events::GetParentId);
            parent = nextParentId;
            if (!parent.IsValid())
            {
                break;
            }
            for (const AZ::EntityId& parentCheck : entities)
            {
                if (parentCheck == parent)
                {
                    selectionIncludesTransformHeritage = true;
                    break;
                }
            }
        } while (parent.IsValid() && !selectionIncludesTransformHeritage);

        if (!selectionIncludesTransformHeritage)
        {
            out.insert(entityId);
        }
    }
}

SandboxIntegrationManager::SandboxIntegrationManager()
    : m_startedUndoRecordingNestingLevel(0)
{
    // Required to receive events from the Cry Engine undo system.
    GetIEditor()->GetUndoManager()->AddListener(this);

    // Create the PrefabIntegrationManager, which initializes prefab-related operations.
    m_prefabIntegrationManager = aznew AzToolsFramework::Prefab::PrefabIntegrationManager();

    // Connect to the Action Manager Registration bus to correctly register actions, menus, toolbars etc.
    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
}

SandboxIntegrationManager::~SandboxIntegrationManager()
{
    AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();

    GetIEditor()->GetUndoManager()->RemoveListener(this);

    delete m_prefabIntegrationManager;
    m_prefabIntegrationManager = nullptr;
}

void SandboxIntegrationManager::Setup()
{
    AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
    AzToolsFramework::EditorRequests::Bus::Handler::BusConnect();
    AzToolsFramework::EditorWindowRequests::Bus::Handler::BusConnect();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();

    m_prefabIntegrationInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabIntegrationInterface>::Get();
    AZ_Assert(
        (m_prefabIntegrationInterface != nullptr),
        "SandboxIntegrationManager requires a PrefabIntegrationInterface instance to be present on Setup().");

    m_editorEntityAPI = AZ::Interface<AzToolsFramework::EditorEntityAPI>::Get();
    AZ_Assert(m_editorEntityAPI, "SandboxIntegrationManager requires an EditorEntityAPI instance to be present on Setup().");

    m_readOnlyEntityPublicInterface = AZ::Interface<AzToolsFramework::ReadOnlyEntityPublicInterface>::Get();
    AZ_Assert(m_readOnlyEntityPublicInterface, "SandboxIntegrationManager requires an ReadOnlyEntityPublicInterface instance to be present on Setup().");

    m_contextMenuBottomHandler.Setup();
}

void SandboxIntegrationManager::Teardown()
{
    m_contextMenuBottomHandler.Teardown();
    AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    AzToolsFramework::EditorWindowRequests::Bus::Handler::BusDisconnect();
    AzToolsFramework::EditorRequests::Bus::Handler::BusDisconnect();
    AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
}

void SandboxIntegrationManager::OnBeginUndo([[maybe_unused]] const char* label)
{
    AzToolsFramework::UndoSystem::URSequencePoint* currentBatch = nullptr;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        currentBatch, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetCurrentUndoBatch);

    AZ_Assert(currentBatch, "No undo batch is active.");

    // Only generate a Sandbox placeholder for root-level undo batches.
    if (nullptr == currentBatch->GetParent())
    {
        // start Cry Undo
        if (!CUndo::IsRecording())
        {
            GetIEditor()->BeginUndo();
            // flag that we started recording the undo batch
            m_startedUndoRecordingNestingLevel = 1;
        }

    }
    else
    {
        if (m_startedUndoRecordingNestingLevel)
        {
            // if we previously started recording the undo, increment the nesting level so we can
            // detect when we need to accept the undo in OnEndUndo()
            m_startedUndoRecordingNestingLevel++;
        }
    }
}

void SandboxIntegrationManager::OnEndUndo(const char* label, bool changed)
{
    // Add the undo only after we know it's got a legit change, we can't remove undos from the cry undo system so we do it here instead of OnBeginUndo
    if (changed && CUndo::IsRecording())
    {
        CUndo::Record(new CToolsApplicationUndoLink());
    }
    if (m_startedUndoRecordingNestingLevel)
    {
        m_startedUndoRecordingNestingLevel--;
        if (m_startedUndoRecordingNestingLevel == 0)
        {
            if (changed)
            {
                // only accept the undo batch that we initially started undo recording on
                GetIEditor()->AcceptUndo(label);
            }
            else
            {
                GetIEditor()->CancelUndo();
            }
        }
    }
}

void SandboxIntegrationManager::OpenPinnedInspector(const AzToolsFramework::EntityIdSet& entities)
{
    QDockWidget* dockWidget = InstanceViewPane(LyViewPane::EntityInspectorPinned);
    if (dockWidget)
    {
        QComponentEntityEditorInspectorWindow* editor = static_cast<QComponentEntityEditorInspectorWindow*>(dockWidget->widget());
        if (editor && editor->GetPropertyEditor())
        {
            editor->GetPropertyEditor()->SetOverrideEntityIds(entities);

            AZStd::string widgetTitle;
            if (entities.size() == 1)
            {
                AZStd::string entityName;
                AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationBus::Events::GetEntityName, *entities.begin());
                widgetTitle = AZStd::string::format("%s Inspector", entityName.c_str());

                QObject::connect(editor->GetPropertyEditor(), &AzToolsFramework::EntityPropertyEditor::SelectedEntityNameChanged, [dockWidget](const AZ::EntityId& /*entityId*/, const AZStd::string& name)
                {
                    AZStd::string newTitle = AZStd::string::format("%s Inspector", name.c_str());
                    dockWidget->setWindowTitle(newTitle.c_str());
                });
            }
            else
            {
                widgetTitle = AZStd::string::format("%zu Entities - Inspector", entities.size());
            }

            dockWidget->setWindowTitle(widgetTitle.c_str());
        }
    }
}

void SandboxIntegrationManager::ClosePinnedInspector(AzToolsFramework::EntityPropertyEditor* editor)
{
    QWidget* currentWidget = editor->parentWidget();
    while (currentWidget)
    {
        QDockWidget* dockWidget = qobject_cast<QDockWidget*>(currentWidget);
        if (dockWidget)
        {
            QtViewPaneManager::instance()->ClosePaneInstance(LyViewPane::EntityInspectorPinned, dockWidget);
            return;
        }
        currentWidget = currentWidget->parentWidget();
    }
}

void SandboxIntegrationManager::CreateEditorRepresentation(AZ::Entity* entity)
{
    // Add required editor components to the entity.
    AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
        &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, *entity);
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::GoToSelectedOrHighlightedEntitiesInViewports()
{
    AzToolsFramework::EntityIdList entityIds;
    GetSelectedOrHighlightedEntities(entityIds);
    GoToEntitiesInViewports(entityIds);
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::GoToSelectedEntitiesInViewports()
{
    AzToolsFramework::EntityIdList entityIds;
    GetSelectedEntities(entityIds);
    GoToEntitiesInViewports(entityIds);
}

bool SandboxIntegrationManager::CanGoToSelectedEntitiesInViewports()
{
    AzToolsFramework::EntityIdList entityIds;
    GetSelectedEntities(entityIds);

    if (entityIds.size() == 0)
    {
        return false;
    }

    for (const AZ::EntityId& entityId : entityIds)
    {
        if (CanGoToEntityOrChildren(entityId))
        {
            return true;
        }
    }

    return false;
}

bool SandboxIntegrationManager::CanGoToEntityOrChildren(const AZ::EntityId& entityId) const
{
    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
    if (entity == nullptr)
    {
        return false;
    }

    // Skip if this entity doesn't have a transform;
    // UI entities and system components don't have transforms and thus aren't visible in the Editor viewport
    return entity->GetTransform() != nullptr;
}

AZ::Vector3 SandboxIntegrationManager::GetWorldPositionAtViewportCenter()
{
    if (GetIEditor() && GetIEditor()->GetViewManager())
    {
        CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport();
        if (view)
        {
            int width, height;
            view->GetDimensions(&width, &height);
            return LYVec3ToAZVec3(view->ViewToWorld(QPoint(width / 2, height / 2)));
        }
    }

    return AZ::Vector3::CreateZero();
}

AZ::Vector3 SandboxIntegrationManager::GetWorldPositionAtViewportInteraction() const
{
    const auto& iEditor = GetIEditor();
    if (const auto& viewManager = (iEditor != nullptr) ? iEditor->GetViewManager() : nullptr)
    {
        return viewManager->GetClickPositionInViewportSpace();
    }

    return AZ::Vector3::CreateZero();
}

void SandboxIntegrationManager::ClearRedoStack()
{
    // We have two separate undo systems that are assumed to be kept in sync,
    // So here we tell the legacy Undo system to clear the redo stack and at the same time
    // tell the new AZ undo system to clear redo stack ("slice" the stack)

    // Clear legacy redo stack
    GetIEditor()->ClearRedoStack();

    // Clear AZ redo stack
    AzToolsFramework::UndoSystem::UndoStack* undoStack = nullptr;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(undoStack, &AzToolsFramework::ToolsApplicationRequestBus::Events::GetUndoStack);
    if (undoStack)
    {
        undoStack->Slice();
    }
}

//////////////////////////////////////////////////////////////////////////

void SandboxIntegrationManager::CloneSelection(bool& handled)
{
    AZ_PROFILE_FUNCTION(AzToolsFramework);

    AzToolsFramework::EntityIdList entities;
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        entities,
        &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    AzToolsFramework::EntityIdSet duplicationSet = AzToolsFramework::GetCulledEntityHierarchy(entities);

    if (!duplicationSet.empty())
    {
        m_editorEntityAPI->DuplicateSelected();
        handled = true;
    }
    else
    {
        handled = false;
    }
}

void SandboxIntegrationManager::DeleteSelectedEntities([[maybe_unused]] const bool includeDescendants)
{
    AzToolsFramework::EntityIdList selectedEntityIds;
    AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
        selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::DeleteEntitiesAndAllDescendants, selectedEntityIds);
}

AZ::EntityId SandboxIntegrationManager::CreateNewEntity(AZ::EntityId parentId)
{
    AZ::Vector3 position = AZ::Vector3::CreateZero();

    // If we have an invalid parent, base new entity's position on the viewport.
    if (!parentId.IsValid())
    {
        position = GetWorldPositionAtViewportCenter();
    }
    return CreateNewEntityAtPosition(position, parentId);
}

AZ::EntityId SandboxIntegrationManager::CreateNewEntityAsChild(AZ::EntityId parentId)
{
    AZ_Assert(parentId.IsValid(), "Entity created as a child of an invalid parent entity.");
    auto newEntityId = CreateNewEntity(parentId);

    // Some modules need to know that this special action has taken place; broadcast an event.
    ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::EntityCreatedAsChild, newEntityId, parentId);
    return newEntityId;
}

AZ::EntityId SandboxIntegrationManager::CreateNewEntityAtPosition(const AZ::Vector3& pos, AZ::EntityId parentId)
{
    using namespace AzToolsFramework;
    return m_prefabIntegrationInterface->CreateNewEntityAtPosition(pos, parentId);
}

AzFramework::EntityContextId SandboxIntegrationManager::GetEntityContextId()
{
    AzFramework::EntityContextId editorEntityContextId = AzFramework::EntityContextId::CreateNull();
    AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &AzToolsFramework::EditorEntityContextRequestBus::Events::GetEditorEntityContextId);

    return editorEntityContextId;
}

//////////////////////////////////////////////////////////////////////////
QWidget* SandboxIntegrationManager::GetAppMainWindow()
{
    return MainWindow::instance();
}

QWidget* SandboxIntegrationManager::GetMainWindow()
{
    return MainWindow::instance();
}

IEditor* SandboxIntegrationManager::GetEditor()
{
    return GetIEditor();
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::LaunchLuaEditor(const char* files)
{
    CCryEditApp::instance()->OpenLUAEditor(files);
}

bool SandboxIntegrationManager::IsLevelDocumentOpen()
{
    return (GetIEditor() && GetIEditor()->GetDocument() && GetIEditor()->GetDocument()->IsDocumentReady());
}

AZStd::string SandboxIntegrationManager::GetLevelName()
{
    return AZStd::string(GetIEditor()->GetGameEngine()->GetLevelName().toUtf8().constData());
}

void SandboxIntegrationManager::OnPrepareForContextReset()
{
    // Deselect everything.
    AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
        &AzToolsFramework::ToolsApplicationRequests::Bus::Events::SetSelectedEntities, AzToolsFramework::EntityIdList());
}

void SandboxIntegrationManager::OnActionRegistrationHook()
{
    auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
    auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();

    if (!actionManagerInterface || !hotKeyManagerInterface)
    {
        return;
    }

    // Create entity
    {
        const AZStd::string_view actionIdentifier = "o3de.action.sandbox.createEntity";
        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Create entity";
        actionProperties.m_description = "Creates an entity under the current selection";
        actionProperties.m_category = "Entity";
        actionProperties.m_menuVisibility = AzToolsFramework::ActionVisibility::HideWhenDisabled;

        actionManagerInterface->RegisterAction(
            EditorIdentifiers::MainWindowActionContextIdentifier,
            actionIdentifier,
            actionProperties,
            [this]()
            {
                AZ::Vector3 worldPosition = GetWorldPositionAtViewportInteraction();

                AzToolsFramework::EntityIdList selectedEntities;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                // when nothing is selected, entity is created at root level.
                if (selectedEntities.empty())
                {
                    CreateNewEntityAtPosition(worldPosition);
                }
                // when a single entity is selected, entity is created as its child.
                else if (selectedEntities.size() == 1)
                {
                    AZ::EntityId selectedEntityId = selectedEntities.front();
                    bool selectedEntityIsReadOnly = m_readOnlyEntityPublicInterface->IsReadOnly(selectedEntityId);
                    auto containerEntityInterface = AZ::Interface<AzToolsFramework::ContainerEntityInterface>::Get();

                    if (containerEntityInterface && containerEntityInterface->IsContainerOpen(selectedEntityId) && !selectedEntityIsReadOnly)
                    {
                        AZ::Transform entityTransform = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(entityTransform, selectedEntityId, &AZ::TransformBus::Events::GetWorldTM);
                        CreateNewEntityAtPosition(entityTransform.GetInverse().TransformPoint(worldPosition), selectedEntityId);
                    }
                }
            }
        );

        actionManagerInterface->InstallEnabledStateCallback(
            actionIdentifier,
            [readOnlyEntityPublicInterface = m_readOnlyEntityPublicInterface]() -> bool
            {
                AzToolsFramework::EntityIdList selectedEntities;
                AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                    selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

                if (selectedEntities.size() == 0)
                {
                    return true;
                }
                else if (selectedEntities.size() == 1)
                {
                    AZ::EntityId selectedEntityId = selectedEntities.front();
                    bool selectedEntityIsReadOnly = readOnlyEntityPublicInterface->IsReadOnly(selectedEntityId);
                    auto containerEntityInterface = AZ::Interface<AzToolsFramework::ContainerEntityInterface>::Get();

                    return (containerEntityInterface && containerEntityInterface->IsContainerOpen(selectedEntityId) && !selectedEntityIsReadOnly);
                }

                return false;
            }
        );

        // Trigger update whenever entity selection changes.
        actionManagerInterface->AddActionToUpdater(EditorIdentifiers::EntitySelectionChangedUpdaterIdentifier, actionIdentifier);

        hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "Ctrl+Alt+N");
    }
}

void SandboxIntegrationManager::OnMenuBindingHook()
{
    auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();

    if (!menuManagerInterface)
    {
        return;
    }

    // Entity Outliner Context Menu
    auto outcome = menuManagerInterface->AddActionToMenu(EditorIdentifiers::EntityOutlinerContextMenuIdentifier, "o3de.action.sandbox.createEntity", 100);

    // Viewport Context Menu
    menuManagerInterface->AddActionToMenu(EditorIdentifiers::ViewportContextMenuIdentifier, "o3de.action.sandbox.createEntity", 100);
}

void SandboxIntegrationManager::ContextMenu_NewEntity()
{
    AZ::Vector3 worldPosition = AZ::Vector3::CreateZero();

    // If we don't have a viewport active to aid in placement, the object
    // will be created at the origin.
    if (CViewport* view = GetIEditor()->GetViewManager()->GetGameViewport();
        view && m_contextMenuViewPoint.has_value())
    {
        worldPosition = AzToolsFramework::FindClosestPickIntersection(
            view->GetViewportId(), m_contextMenuViewPoint.value(), AzToolsFramework::EditorPickRayLength,
            AzToolsFramework::GetDefaultEntityPlacementDistance());
    }

    CreateNewEntityAtPosition(worldPosition);
}

//////////////////////////////////////////////////////////////////////////
void SandboxIntegrationManager::GoToEntitiesInViewports(const AzToolsFramework::EntityIdList& entityIds)
{
    if (entityIds.empty())
    {
        return;
    }

    const AZ::Aabb aabb = AZStd::accumulate(
        AZStd::begin(entityIds), AZStd::end(entityIds), AZ::Aabb::CreateNull(), [](AZ::Aabb acc, const AZ::EntityId entityId) {
            const AZ::Aabb aabb = AzFramework::CalculateEntityWorldBoundsUnion(AzToolsFramework::GetEntityById(entityId));
            acc.AddAabb(aabb);
            return acc;
        });

    float radius;
    AZ::Vector3 center;
    aabb.GetAsSphere(center, radius);

    auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
    const int viewCount = GetIEditor()->GetViewManager()->GetViewCount(); // legacy call
    for (int viewIndex = 0; viewIndex < viewCount; ++viewIndex)
    {
        if (auto viewportContext = viewportContextManager->GetViewportContextById(viewIndex))
        {
            if (const AZStd::optional<AZ::Transform> nextCameraTransform = SandboxEditor::CalculateGoToEntityTransform(
                    viewportContext->GetCameraTransform(),
                    AzFramework::RetrieveFov(viewportContext->GetCameraProjectionMatrix()),
                    center,
                    radius);
                nextCameraTransform.has_value())
            {
                SandboxEditor::HandleDefaultViewportCameraTransitionFromSetting(*nextCameraTransform);
            }
        }
    }
}

void SandboxIntegrationManager::ContextMenu_Duplicate()
{
    bool handled = true;
    AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::CloneSelection, handled);
}

void SandboxIntegrationManager::ContextMenu_DeleteSelected()
{
    DeleteSelectedEntities(true);
}

void SandboxIntegrationManager::GetSelectedEntities(AzToolsFramework::EntityIdList& entities)
{
    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        entities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);
}

void SandboxIntegrationManager::GetSelectedOrHighlightedEntities(AzToolsFramework::EntityIdList& entities)
{
    AzToolsFramework::EntityIdList selectedEntities;
    AzToolsFramework::EntityIdList highlightedEntities;

    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        selectedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

    AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
        highlightedEntities, &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetHighlightedEntities);

    entities = AZStd::move(selectedEntities);

    for (AZ::EntityId highlightedId : highlightedEntities)
    {
        if (entities.end() == AZStd::find(entities.begin(), entities.end(), highlightedId))
        {
            entities.push_back(highlightedId);
        }
    }
}

AZStd::string SandboxIntegrationManager::GetComponentEditorIcon(const AZ::Uuid& componentType, const AZ::Component* component)
{
    AZStd::string iconPath = GetComponentIconPath(componentType, AZ::Edit::Attributes::Icon, component);
    return iconPath;
}

AZStd::string SandboxIntegrationManager::GetComponentTypeEditorIcon(const AZ::Uuid& componentType)
{
    return GetComponentEditorIcon(componentType, nullptr);
}

AZStd::string SandboxIntegrationManager::GetComponentIconPath(const AZ::Uuid& componentType,
    AZ::Crc32 componentIconAttrib, const AZ::Component* component)
{
    AZ_PROFILE_FUNCTION(AzToolsFramework);
    if (componentIconAttrib != AZ::Edit::Attributes::Icon
        && componentIconAttrib != AZ::Edit::Attributes::ViewportIcon
        && componentIconAttrib != AZ::Edit::Attributes::HideIcon)
    {
        AZ_Warning("SandboxIntegration", false, "Unrecognized component icon attribute!");
    }

    // return blank path if component shouldn't have icon at all
    AZStd::string iconPath;

    AZ::SerializeContext* serializeContext = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
    AZ_Assert(serializeContext, "No serialize context");

    auto classData = serializeContext->FindClassData(componentType);
    if (classData && classData->m_editData)
    {
        // check if component icon should be hidden
        bool hideIcon = false;

        auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
        if (editorElementData)
        {
            if (auto hideIconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::HideIcon))
            {
                auto hideIconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<bool>*>(hideIconAttribute);
                if (hideIconAttributeData)
                {
                    hideIcon = hideIconAttributeData->Get(nullptr);
                }
            }
        }

        if (!hideIcon)
        {
            // component should have icon. start with default
            iconPath = GetDefaultComponentEditorIcon();

            // check for specific icon
            if (editorElementData)
            {
                if (auto iconAttribute = editorElementData->FindAttribute(componentIconAttrib))
                {
                    if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
                    {
                        AZStd::string iconAttributeValue = iconAttributeData->Get(component);
                        if (!iconAttributeValue.empty())
                        {
                            iconPath = AZStd::move(iconAttributeValue);
                        }
                    }

                    auto iconOverrideAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::DynamicIconOverride);

                    // If it has an override and we're given an instance, then get any potential override from the instance here
                    if (component &&
                        (componentIconAttrib == AZ::Edit::Attributes::Icon || componentIconAttrib == AZ::Edit::Attributes::ViewportIcon) &&
                        iconOverrideAttribute)
                    {
                        AZStd::string iconValue;
                        AZ::AttributeReader iconReader(const_cast<AZ::Component*>(component), iconOverrideAttribute);
                        iconReader.Read<AZStd::string>(iconValue);

                        if (!iconValue.empty())
                        {
                            iconPath = AZStd::move(iconValue);
                        }
                    }
                }
            }
            // If Qt doesn't know where the relative path is we have to use the more expensive full path
            if (!QFile::exists(QString(iconPath.c_str())))
            {
                // use absolute path if possible
                AZStd::string iconFullPath;
                bool pathFound = false;
                using AssetSysReqBus = AzToolsFramework::AssetSystemRequestBus;
                AssetSysReqBus::BroadcastResult(
                    pathFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath,
                    iconPath, iconFullPath);
                if (pathFound)
                {
                    iconPath = AZStd::move(iconFullPath);
                }
                else
                {
                    // If we couldn't find the full source path, try appending the product extension if the icon
                    // source asset is one of the supported image assets. Most component icons are in .svg format,
                    // which isn't actually consumed by the Asset Processor so the GetFullSourcePathFromRelativeProductPath
                    // API can find the source asset without needing the product extension as well. So this edge case is
                    // to cover any component icons that are still using other formats (e.g. png), that haven't been converted
                    // to .svg yet, or for customers that prefer to use image formats besides .svg.
                    AZStd::string extension;
                    AzFramework::StringFunc::Path::GetExtension(iconPath.c_str(), extension);
                    bool supportedStreamingImage = false;
                    for (int i = 0; i < ImageProcessingAtom::s_TotalSupportedImageExtensions; ++i)
                    {
                        if (AZStd::wildcard_match(ImageProcessingAtom::s_SupportedImageExtensions[i], extension.c_str()))
                        {
                            supportedStreamingImage = true;
                            break;
                        }
                    }
                    if (supportedStreamingImage)
                    {
                        iconPath = AZStd::string::format("%s.%s", iconPath.c_str(), AZ::RPI::StreamingImageAsset::Extension);

                        AssetSysReqBus::BroadcastResult(
                            pathFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath,
                            iconPath, iconFullPath);
                    }

                    if (pathFound)
                    {
                        iconPath = AZStd::move(iconFullPath);
                    }
                    else
                    {
                        AZ_Warning(
                            "SandboxIntegration",
                            false,
                            "Unable to find icon path \"%s\" for component type: %s",
                            iconPath.c_str(),
                            classData->m_editData->m_name);
                    }
                }
            }
        }
    }

    return iconPath;
}

void SandboxIntegrationManager::UndoStackFlushed()
{
    AzToolsFramework::ToolsApplicationRequestBus::Broadcast(&AzToolsFramework::ToolsApplicationRequestBus::Events::FlushUndo);
}

void SandboxIntegrationManager::RegisterViewPane(const char* name, const char* category, const AzToolsFramework::ViewPaneOptions& viewOptions, const WidgetCreationFunc& widgetCreationFunc)
{
    QtViewPaneManager::instance()->RegisterPane(name, category, widgetCreationFunc, viewOptions);
}

void SandboxIntegrationManager::RegisterCustomViewPane(const char* name, const char* category, const AzToolsFramework::ViewPaneOptions& viewOptions)
{
    QtViewPaneManager::instance()->RegisterPane(name, category, nullptr, viewOptions);
}

void SandboxIntegrationManager::UnregisterViewPane(const char* name)
{
    QtViewPaneManager::instance()->UnregisterPane(name);
}

QWidget* SandboxIntegrationManager::GetViewPaneWidget(const char* viewPaneName)
{
    return FindViewPane<QWidget>(viewPaneName);
}

void SandboxIntegrationManager::OpenViewPane(const char* paneName)
{
    const QtViewPane* pane = QtViewPaneManager::instance()->OpenPane(paneName);
    if (pane)
    {
        pane->m_dockWidget->raise();
        pane->m_dockWidget->activateWindow();
    }
}

QDockWidget* SandboxIntegrationManager::InstanceViewPane(const char* paneName)
{
    return QtViewPaneManager::instance()->InstancePane(paneName);
}

void SandboxIntegrationManager::CloseViewPane(const char* paneName)
{
    QtViewPaneManager::instance()->ClosePane(paneName);
}

void SandboxIntegrationManager::BrowseForAssets(AssetSelectionModel& selection)
{
    AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, GetMainWindow());
}
