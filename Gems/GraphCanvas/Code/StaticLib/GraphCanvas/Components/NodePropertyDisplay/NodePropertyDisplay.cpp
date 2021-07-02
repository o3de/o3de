/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsProxyWidget>
#include <QMenu>
#include <QMimeData>
AZ_POP_DISABLE_WARNING

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>

#include <GraphCanvas/Components/NodePropertyDisplay/DataInterface.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <Source/Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ////////////////////////
    // NodePropertyDisplay
    ////////////////////////

    NodePropertyDisplay::NodePropertyDisplay(DataInterface* dataInterface)
        : m_dataInterface(dataInterface)
    {
        m_dataInterface->RegisterDisplay(this);
    }

    NodePropertyDisplay::~NodePropertyDisplay()
    {
        NodePropertiesRequestBus::Event(GetNodeId(), &NodePropertiesRequests::UnlockEditState, this);
    }

    void NodePropertyDisplay::SetSlotId(const SlotId& slotId)
    {
        DataSlotNotificationBus::Handler::BusConnect(slotId);

        m_slotId = slotId;
        OnSlotIdSet();
    }

    void NodePropertyDisplay::SetNodeId(const AZ::EntityId& nodeId)
    {
        m_nodeId = nodeId;
    }

    const AZ::EntityId& NodePropertyDisplay::GetNodeId() const
    {
        return m_nodeId;
    }

    AZ::EntityId NodePropertyDisplay::GetSceneId() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, m_nodeId, &SceneMemberRequests::GetScene);
        return sceneId;
    }

    void NodePropertyDisplay::TryAndSelectNode() const
    {
        bool isSelected = false;
        SceneMemberUIRequestBus::EventResult(isSelected, m_nodeId, &SceneMemberUIRequests::IsSelected);

        if (!isSelected)
        {
            SceneRequestBus::Event(GetSceneId(), &SceneRequests::ClearSelection);
            SceneMemberUIRequestBus::Event(GetNodeId(), &SceneMemberUIRequests::SetSelected, true);
        }
    }

    bool NodePropertyDisplay::EnableDropHandling() const
    {
        return m_dataInterface->EnableDropHandling();
    }

    AZ::Outcome<DragDropState> NodePropertyDisplay::OnDragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        AZ::Outcome<DragDropState> outcomeResult = AZ::Failure();

        bool isConnected = false;
        SlotRequestBus::EventResult(isConnected, GetSlotId(), &SlotRequests::HasConnections);

        if (!isConnected)
        {
            const QMimeData* dropMimeData = dragDropEvent->mimeData();

            outcomeResult = m_dataInterface->ShouldAcceptMimeData(dropMimeData);
        }

        return outcomeResult;
    }

    void NodePropertyDisplay::OnDragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent)
    {
        AZ_UNUSED(dragDropEvent);
    }

    void NodePropertyDisplay::OnDropEvent(QGraphicsSceneDragDropEvent* dropEvent)
    {
        const QMimeData* dropMimeData = dropEvent->mimeData();

        if (m_dataInterface->HandleMimeData(dropMimeData))
        {
            UpdateDisplay();
        }
    }

    void NodePropertyDisplay::OnDropCancelled()
    {
    }

    void NodePropertyDisplay::RegisterShortcutDispatcher(QWidget* widget)
    {
        AzQtComponents::ShortcutDispatchBus::Handler::BusConnect(widget);
    }

    void NodePropertyDisplay::UnregisterShortcutDispatcher(QWidget* widget)
    {
        AzQtComponents::ShortcutDispatchBus::Handler::BusDisconnect(widget);

        widget->clearFocus();
        widget->releaseKeyboard();
        widget->releaseMouse();
    }

    QWidget* NodePropertyDisplay::GetShortcutDispatchScopeRoot(QWidget*)
    {
        QGraphicsScene* graphicsScene = nullptr;
        SceneRequestBus::EventResult(graphicsScene, GetSceneId(), &SceneRequests::AsQGraphicsScene);

        if (graphicsScene)
        {
            // Get the list of views. Which one it uses shouldn't matter.
            // Since they should all be parented to the same root window.
            QList<QGraphicsView*> graphicViews = graphicsScene->views();

            if (!graphicViews.empty())
            {
                return graphicViews.front();
            }
        }

        return nullptr;
    }

    void NodePropertyDisplay::UpdateStyleForDragDrop(const DragDropState& dragDropState, Styling::StyleHelper& styleHelper)
    {
        switch (dragDropState)
        {
        case DragDropState::Valid:
        {
            styleHelper.AddSelector(Styling::States::ValidDrop);
            break;
        }
        case DragDropState::Invalid:
        {
            styleHelper.AddSelector(Styling::States::InvalidDrop);
            break;
        }
        case DragDropState::Idle:
            styleHelper.RemoveSelector(Styling::States::ValidDrop);
            styleHelper.RemoveSelector(Styling::States::InvalidDrop);
            break;
        default:
            break;
        }
    }
}
