/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PreviewActionLog.h"

#include <QScrollBar>

#include <AzQtComponents/Components/Widgets/ScrollBar.h>

#include <LyShine/Bus/UiAnimationBus.h>
#include <LyShine/Bus/UiElementBus.h>

//////////////////////////////////////////////////////////////////////////
//  PAINT COLOURS
//
//  The action log's text colours and its fill, named so the legacy and the flattened GUI read one value. The text colours are the
//  colours Qt's colour names resolve to (white 255,255,255; gray 128,128,128; lightGray 211,211,211). A named namespace, not an
//  anonymous one: a unity build merges the anonymous namespaces of a target's files.
//////////////////////////////////////////////////////////////////////////
namespace PreviewActionLogFlat
{
    // PreviewActionLog.cpp:85 (QColor("lightGray"): a repeated message, the odd repeats). TOKENIZE: ActionLogRepeatOddText
    constexpr QColor RepeatOddText(211, 211, 211);

    // PreviewActionLog.cpp:89 (QColor("gray"): a repeated message, the even repeats). TOKENIZE: ActionLogRepeatEvenText
    constexpr QColor RepeatEvenText(128, 128, 128);

    // PreviewActionLog.cpp:96 (QColor("white"): a new message). TOKENIZE: ActionLogNewText
    constexpr QColor NewText(255, 255, 255);

    // PreviewActionLog.cpp:28 (background-color of the log, set as its sheet). TOKENIZE: ActionLogFill
    constexpr QColor Fill(0x22, 0x22, 0x22);
} // namespace PreviewActionLogFlat

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

    // The fill of the log, handed over as the consoles' colours are: the legacy GUI sets an inline sheet, `background: #222222` (the
    // sheet before the flattening was `background-color: #222222;`, the same fill); the flattened GUI sets the Base and Window colours
    // of the area and no sheet
    AzQtComponents::ScrollBar::setBackgroundColor(this, PreviewActionLogFlat::Fill);
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
            setTextColor(PreviewActionLogFlat::RepeatOddText);
        }
        else
        {
            setTextColor(PreviewActionLogFlat::RepeatEvenText);
        }
    }
    else
    {
        m_lastMessage = message;
        m_repeatCount = 0;
        setTextColor(PreviewActionLogFlat::NewText);
    }

    append(message); // Adds the message to the widget
    verticalScrollBar()->setValue(verticalScrollBar()->maximum()); // Scrolls to the bottom
}

QSize PreviewActionLog::sizeHint() const
{
    return QSize(300, 100);
}

