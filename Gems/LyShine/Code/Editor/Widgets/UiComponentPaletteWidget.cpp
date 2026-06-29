/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UiComponentPaletteWidget.h"

#include "UiEditorInternalBus.h"

#include <QModelIndex>

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>

UiComponentPaletteWidget::UiComponentPaletteWidget(QWidget* parent)
    : ComponentPaletteWidget(parent, true)
{
}

void UiComponentPaletteWidget::ActivateSelection(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    // The component class data is stored in UserRole+1 by the base class model
    auto componentClass = reinterpret_cast<const AZ::SerializeContext::ClassData*>(
        index.data(Qt::ItemDataRole::UserRole + 1).toULongLong());

    if (!componentClass)
    {
        return;
    }

    emit OnAddComponentBegin();

    // ---------------------------------------------------------------
    // Add component using LyShine's undo system
    // ---------------------------------------------------------------
    UiEditorInternalNotificationBus::Broadcast(
        &UiEditorInternalNotificationBus::Events::OnBeginUndoableEntitiesChange);

    AzToolsFramework::EntityIdList selectedEntities;
    UiEditorInternalRequestBus::BroadcastResult(
        selectedEntities, &UiEditorInternalRequestBus::Events::GetSelectedEntityIds);

    // If nothing is selected, fall back to the active canvas entity
    if (selectedEntities.empty())
    {
        AZ::EntityId canvasEntityId;
        UiEditorInternalRequestBus::BroadcastResult(
            canvasEntityId, &UiEditorInternalRequestBus::Events::GetActiveCanvasEntityId);
        if (canvasEntityId.IsValid())
        {
            selectedEntities.push_back(canvasEntityId);
        }
    }

    for (const AZ::EntityId& entityId : selectedEntities)
    {
        AZ::Entity* entity = AzToolsFramework::GetEntityById(entityId);
        if (!entity)
        {
            continue;
        }

        entity->Deactivate();

        AZ::Component* component = nullptr;
        AZ::ComponentDescriptorBus::EventResult(
            component, componentClass->m_typeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);

        if (component)
        {
            entity->AddComponent(component);
        }

        entity->Activate();
    }

    UiEditorInternalNotificationBus::Broadcast(
        &UiEditorInternalNotificationBus::Events::OnEndUndoableEntitiesChange, "add component");

    UiEditorInternalNotificationBus::Broadcast(
        &UiEditorInternalNotificationBus::Events::OnSelectedEntitiesPropertyChanged);

    emit OnAddComponentEnd();

    // Clear search and dismiss the palette through the base class codepath
    ClearSearch();
    hide();
}
