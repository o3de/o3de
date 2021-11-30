/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EntityIdQLineEdit.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <AzQtComponents/Components/Widgets/LineEdit.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsMessaging/EntityHighlightBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'qreal' to 'int', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QMouseEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    EntityIdQLineEdit::EntityIdQLineEdit(QWidget* parent)
        : QLineEdit(parent)
    {
        setReadOnly(true);
        setClearButtonEnabled(true);
        AzQtComponents::LineEdit::setEnableClearButtonWhenReadOnly(this, true);
    }

    EntityIdQLineEdit::~EntityIdQLineEdit()
    {
    }

    void EntityIdQLineEdit::SetEntityId(AZ::EntityId newId, const AZStd::string_view& nameOverride)
    {
        m_entityId = newId;
        if (!m_entityId.IsValid())
        {
            setText(QString());
            return;
        }

        const AZStd::string entityName = AzToolsFramework::GetEntityName(m_entityId, nameOverride);
        if (entityName.empty())
        {
            setText(tr("(Entity not found)"));
        }
        else
        {
            setText(entityName.c_str());
        }
    }

    void EntityIdQLineEdit::mousePressEvent(QMouseEvent* e)
    {
        if (e->modifiers() & Qt::ControlModifier)
        {
            emit RequestPickObject();
        }
        else
        {
            AzToolsFramework::EntityHighlightMessages::Bus::Broadcast(&AzToolsFramework::EntityHighlightMessages::EntityHighlightRequested, m_entityId);
        }

        QLineEdit::mousePressEvent(e);
    }

    void EntityIdQLineEdit::mouseDoubleClickEvent(QMouseEvent* e)
    {
        QLineEdit::mouseDoubleClickEvent(e);

        if (m_entityId.IsValid())
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityId);
            if (entity)
            {
                EntityIdList selectionList = { m_entityId };

                AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::SetSelectedEntities, selectionList);

                bool canGoTo = false;
                AzToolsFramework::EditorRequestBus::BroadcastResult(canGoTo, &AzToolsFramework::EditorRequests::CanGoToSelectedEntitiesInViewports);

                if (canGoTo)
                {
                    AzToolsFramework::EditorRequestBus::Broadcast(&AzToolsFramework::EditorRequests::GoToSelectedEntitiesInViewports);
                }
            }
        }

        AzToolsFramework::EntityHighlightMessages::Bus::Broadcast(&AzToolsFramework::EntityHighlightMessages::EntityStrongHighlightRequested, m_entityId);
        // The above EBus request calls EntityPropertyEditor::UpdateContents() down the code path, which in turn destroys
        // the current EntityIdQLineEdit object, therefore calling any method of EntityIdQLineEdit at this point is invalid.
    }
}
