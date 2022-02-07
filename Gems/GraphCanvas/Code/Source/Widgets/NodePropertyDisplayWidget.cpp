/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <qgraphicslinearlayout.h>
#include <qgraphicsscene.h>

#include <Widgets/NodePropertyDisplayWidget.h>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>

namespace GraphCanvas
{
    //////////////////////////////
    // NodePropertyDisplayWidget
    //////////////////////////////
    
    NodePropertyDisplayWidget::NodePropertyDisplayWidget(QGraphicsItem* parent)
        : QGraphicsWidget(parent)
        , m_nodePropertyDisplay(nullptr)
        , m_layoutItem(nullptr)
        , m_disabled(false)
        , m_editing(false)
        , m_forcedLayout(NodePropertyLayoutState::None)
    {
        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        setLayout(m_layout);

        m_layout->setContentsMargins(0, 0, 0, 0);
        setContentsMargins(0, 0, 0, 0);
    
        // Timer used to help manage switching between data slots.
        // To avoid situations where you tab to the next widget
        // and the editing components vanish because nothing is locked
        // and the mouse is off of the node.
        m_layoutTimer.setSingleShot(true);
        m_layoutTimer.setInterval(0);
        m_layoutTimer.stop();

        QObject::connect(&m_layoutTimer, &QTimer::timeout, [this]() { this->UpdateLayout(); });
    }
    
    NodePropertyDisplayWidget::~NodePropertyDisplayWidget()
    {
        ClearLayout();
        delete m_nodePropertyDisplay;

        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void NodePropertyDisplayWidget::RefreshStyle()
    {
        if (m_nodePropertyDisplay)
        {
            m_nodePropertyDisplay->RefreshStyle();
        }
    }

    void NodePropertyDisplayWidget::OnSystemTick()
    {
        if (m_nodePropertyDisplay)
        {
            NodeUIRequestBus::Event(m_nodePropertyDisplay->GetNodeId(), &NodeUIRequests::AdjustSize);
        }
    }

    void NodePropertyDisplayWidget::OnDisplayStateChanged(RootGraphicsItemDisplayState /*oldState*/, RootGraphicsItemDisplayState newState)
    {
        if (newState == RootGraphicsItemDisplayState::Inspection)
        {
            if (!m_editing)
            {
                m_editing = true;
                UpdateLayout();
            }
        }
        else if (m_editing)
        {
            m_editing = false;
            UpdateLayout();
        }
    }

    void NodePropertyDisplayWidget::LockEditState(NodePropertyDisplay* propertyDisplay)
    {
        SceneNotificationBus::Event(propertyDisplay->GetSceneId(), &SceneNotifications::OnNodeIsBeingEdited, true);

        m_forceEditSet.insert(propertyDisplay);
    }

    void NodePropertyDisplayWidget::UnlockEditState(NodePropertyDisplay* propertyDisplay)
    {
        AZStd::size_t count = m_forceEditSet.erase(propertyDisplay);

        // In case we are tabbing between elements, we don't want to update the layout immediately.
        if (count > 0 && m_forceEditSet.empty())
        {
            m_layoutTimer.start();

            SceneNotificationBus::Event(propertyDisplay->GetSceneId(), &SceneNotifications::OnNodeIsBeingEdited, false);
        }
    }
    
    void NodePropertyDisplayWidget::SetNodePropertyDisplay(NodePropertyDisplay* propertyDisplayController)
    {
        if (m_nodePropertyDisplay != nullptr)
        {
            ClearDisplay();
        }
        
        RootGraphicsItemNotificationBus::Handler::BusDisconnect();
        NodePropertiesRequestBus::Handler::BusDisconnect();
        NodePropertyRequestBus::Handler::BusDisconnect();
        
        m_nodePropertyDisplay = propertyDisplayController;

        if (propertyDisplayController)
        {
            m_nodePropertyDisplay->UpdateDisplay();
            m_nodePropertyDisplay->RefreshStyle();
        
            RootGraphicsItemNotificationBus::Handler::BusConnect(m_nodePropertyDisplay->GetNodeId());
            NodePropertiesRequestBus::Handler::BusConnect(m_nodePropertyDisplay->GetNodeId());
            NodePropertyRequestBus::Handler::BusConnect(m_nodePropertyDisplay->GetSlotId());
        }

        RootGraphicsItemDisplayState displayState = RootGraphicsItemDisplayState::Neutral;
        RootGraphicsItemRequestBus::EventResult(displayState, m_nodePropertyDisplay->GetNodeId(), &RootGraphicsItemRequests::GetDisplayState);

        OnDisplayStateChanged(RootGraphicsItemDisplayState::Neutral, displayState);

        const bool updateLayout = true;
        UpdateLayout(updateLayout);
    }
    
    NodePropertyDisplay* NodePropertyDisplayWidget::GetNodePropertyDisplay() const
    {
        return m_nodePropertyDisplay;
    }

    void NodePropertyDisplayWidget::ClearDisplay()
    {
        ClearLayout();
        delete m_nodePropertyDisplay;

        m_nodePropertyDisplay = nullptr;
        m_layoutItem = nullptr;
    }
    
    void NodePropertyDisplayWidget::ForceLayoutState(NodePropertyLayoutState layoutState)
    {
        if (m_forcedLayout != layoutState)
        {
            m_forcedLayout = layoutState;
            UpdateLayout();
        }
    }
    
    void NodePropertyDisplayWidget::SetDisabled(bool disabled)
    {
        if (m_disabled != disabled)
        {
            m_disabled = disabled;
            UpdateLayout();
        }
    }
    
   void NodePropertyDisplayWidget::ClearLayout()
    {
        for (int i = m_layout->count() - 1; i >= 0; --i)
        {
            QGraphicsLayoutItem* layoutItem = m_layout->itemAt(i);
            m_layout->removeAt(i);
            layoutItem->setParentLayoutItem(nullptr);
        }
    }
    
    void NodePropertyDisplayWidget::UpdateLayout(bool forceUpdate)
    {
        bool isForcedEdit = !m_forceEditSet.empty();

        if (!forceUpdate && isForcedEdit)
        {
            return;
        }

        ClearLayout();
        
        if (m_nodePropertyDisplay == nullptr)
        {
            return;
        }

        // Removing the m_layoutItem from the scene needs to be delayed to the next event loop,
        // because this method gets called during QGraphicsScene::mouseMoveEvent, which in turn
        // generates additional events based on a cached list of items in the scene, so if we
        // remove an item from the scene while that is in progress, it can crash since it is
        // still internally using a cached list of the scene items.
        QTimer::singleShot(0, this, [this, isForcedEdit]() {
            // Element here needs to be removed from the scene since removing it from the layout
            // doesn't actually remove it from the scene. The end result of this is you get an elements
            // which is still being rendered, and acts like it's apart of the layout despite not being
            // in the layout.
            if (m_layoutItem)
            {
                QGraphicsScene* graphicsScene = nullptr;
                AZ::EntityId sceneId = m_nodePropertyDisplay->GetSceneId();

                SceneRequestBus::EventResult(graphicsScene, sceneId, &SceneRequests::AsQGraphicsScene);

                if (graphicsScene && m_layoutItem->graphicsItem()->scene() != nullptr)
                {
                    graphicsScene->removeItem(m_layoutItem->graphicsItem());
                }

                m_layoutItem = nullptr;
            }

            NodePropertyLayoutState layoutState = m_forcedLayout;

            if (layoutState == NodePropertyLayoutState::None)
            {
                if (m_disabled)
                {
                    layoutState = NodePropertyLayoutState::Disabled;
                }
                else if (m_editing || isForcedEdit)
                {
                    layoutState = NodePropertyLayoutState::Editing;
                }
                else
                {
                    layoutState = NodePropertyLayoutState::Display;
                }
            }

            if (m_nodePropertyDisplay)
            {
                switch (layoutState)
                {
                case NodePropertyLayoutState::Disabled:
                    m_layoutItem = m_nodePropertyDisplay->GetDisabledGraphicsLayoutItem();
                    break;
                case NodePropertyLayoutState::Editing:
                    m_layoutItem = m_nodePropertyDisplay->GetEditableGraphicsLayoutItem();
                    break;
                case NodePropertyLayoutState::Display:
                    m_layoutItem = m_nodePropertyDisplay->GetDisplayGraphicsLayoutItem();
                    break;
                default:
                    AZ_Warning("DataSlotLayoutComponent", false, "Unhandled layout case.");
                    m_layoutItem = m_nodePropertyDisplay->GetDisabledGraphicsLayoutItem();
                    break;
                }

                m_layout->addItem(m_layoutItem);
                m_layout->setAlignment(m_layoutItem, Qt::AlignCenter);
            }

            UpdateGeometry();
        });
    }
    
    void NodePropertyDisplayWidget::UpdateGeometry()
    {
        if (m_layoutItem)
        {
            m_layoutItem->updateGeometry();
            m_layout->invalidate();
        }

        AZ::SystemTickBus::Handler::BusConnect();
    }
}
