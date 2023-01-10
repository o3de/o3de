/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include <QScrollBar>

PreviewActionLog::PreviewActionLog([[maybe_unused]] EditorWindow* editorWindow)
    : QTextEdit()
    , m_lastMessage()
    , m_repeatCount(0)
{
    // make the QTextEdit read-only so the user can't type into it
    setReadOnly(true);

    // turn off line wrap, this allows the user to dock on the left and make narrow and just see the
    // start of the message (the action name)
    setLineWrapMode(QTextEdit::NoWrap);
}

PreviewActionLog::~PreviewActionLog()
{
    Deactivate();
}

void PreviewActionLog::Activate(AZ::EntityId canvasEntityId)
{
    // start listening for canvas actions from the given canvas
    UiCanvasNotificationBus::Handler::BusConnect(canvasEntityId);
    m_canvasEntityId = canvasEntityId;

    // reset variables and clear the log
    m_lastMessage = "";
    m_repeatCount = 0;
    clear();
}

void PreviewActionLog::Deactivate()
{
    if (m_canvasEntityId.IsValid())
    {
        // Stop listening for actions
        UiCanvasNotificationBus::Handler::BusDisconnect(m_canvasEntityId);
        m_canvasEntityId.SetInvalid();
    }
}

void PreviewActionLog::OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName)
{
    // Get element ID
    LyShine::ElementId elementId = 0;
    UiElementBus::EventResult(elementId, entityId, &UiElementBus::Events::GetElementId);

    // Get the entity name
    QString entityName;
    AZ::Entity* entity = nullptr;
    AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
    AZ_Assert(entity, "Invalid Entity ID found");
    if (entity)
    {
        entityName = entity->GetName().c_str();
    }

    // build the message that we will append to the log
    QString message = QString("'%1' from element %2 (Element ID = %3)").arg(actionName.c_str()).arg(entityName).arg(elementId);

    // Because a whole string of "changed" message from dragging or scrolling makes it hard to see
    // if anything is being written to the log we modifiy the color for repeated messages. There is one color
    // to the first one, after that the colors alternate between two colors.
    if (message == m_lastMessage)
    {
        m_repeatCount++;
        if (m_repeatCount & 1)
        {
            setTextColor(QColor("lightGray"));
        }
        else
        {
            setTextColor(QColor("gray"));
        }
    }
    else
    {
        m_lastMessage = message;
        m_repeatCount = 0;
        setTextColor(QColor("white"));
    }

    append(message); // Adds the message to the widget
    verticalScrollBar()->setValue(verticalScrollBar()->maximum()); // Scrolls to the bottom
}

QSize PreviewActionLog::sizeHint() const
{
    return QSize(300, 100);
}

#include <moc_PreviewActionLog.cpp>
