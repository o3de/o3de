/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <functional>

#include <QGraphicsLayoutItem>
#include <QGraphicsGridLayout>
#include <QGraphicsScene>
#include <qgraphicssceneevent.h>

#include <AzCore/RTTI/TypeInfo.h>

#include <Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/NodeLayerControllerComponent.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <Components/Nodes/General/GeneralNodeTitleComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>
#include <GraphCanvas/Utils/GraphUtils.h>

namespace GraphCanvas
{    
    //////////////////////
    // WrappedNodeLayout
    //////////////////////
    
    WrapperNodeLayoutComponent::WrappedNodeLayout::WrappedNodeLayout(WrapperNodeLayoutComponent& wrapperLayoutComponent)
        : QGraphicsWidget(nullptr)
        , m_wrapperLayoutComponent(wrapperLayoutComponent)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

        m_layout = new QGraphicsLinearLayout(Qt::Vertical);

        setLayout(m_layout);
    }

    WrapperNodeLayoutComponent::WrappedNodeLayout::~WrappedNodeLayout()
    {
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::RefreshStyle()
    {
        prepareGeometryChange();

        m_styleHelper.SetStyle(m_wrapperLayoutComponent.GetEntityId(), Styling::Elements::WrapperNode::NodeLayout);

        qreal margin = m_styleHelper.GetAttribute(Styling::Attribute::Margin, 0.0);
        setContentsMargins(margin, margin, margin, margin);

        setMinimumSize(m_styleHelper.GetMinimumSize());
        setMaximumSize(m_styleHelper.GetMaximumSize());
        
        // Layout spacing
        m_layout->setSpacing(m_styleHelper.GetAttribute(Styling::Attribute::Spacing, 0.0));
        
        m_layout->invalidate();
        m_layout->updateGeometry();

        updateGeometry();
        update();
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::RefreshLayout()
    {
        prepareGeometryChange();
        ClearLayout();
        LayoutItems();
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::LayoutItems()
    {
        if (!m_wrapperLayoutComponent.m_wrappedNodes.empty())
        {
            setVisible(true);
            for (const AZ::EntityId& nodeId : m_wrapperLayoutComponent.m_wrappedNodes)
            {
                QGraphicsLayoutItem* rootLayoutItem = nullptr;
                SceneMemberUIRequestBus::EventResult(rootLayoutItem, nodeId, &SceneMemberUIRequests::GetRootGraphicsLayoutItem);

                if (rootLayoutItem)
                {
                    m_layout->addItem(rootLayoutItem);
                }
            }            

            updateGeometry();
            update();
        }
        else
        {
            setVisible(false);
        }
    }
    
    void WrapperNodeLayoutComponent::WrappedNodeLayout::ClearLayout()
    {
        while (m_layout->count() > 0)
        {
            QGraphicsLayoutItem* layoutItem = m_layout->itemAt(m_layout->count() - 1);
            m_layout->removeAt(m_layout->count() - 1);
            layoutItem->setParentLayoutItem(nullptr);
        }
    }

    ////////////////////////////////////
    // WrappedNodeActionGraphicsWidget
    ////////////////////////////////////

    WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::WrappedNodeActionGraphicsWidget(WrapperNodeLayoutComponent& wrapperLayoutComponent)
        : QGraphicsWidget(nullptr)
        , m_wrapperLayoutComponent(wrapperLayoutComponent)
        , m_styleState(StyleState::Empty)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFlag(ItemIsFocusable);
        setAcceptHoverEvents(true);
        setAcceptDrops(true);

        QGraphicsLinearLayout* paddingLayout = new QGraphicsLinearLayout(Qt::Vertical);
        paddingLayout->setContentsMargins(6, 6, 6, 6);
        paddingLayout->setSpacing(0);

        m_displayLabel = new GraphCanvasLabel();
        m_displayLabel->setZValue(1);
        m_displayLabel->setFlag(ItemIsFocusable);
        m_displayLabel->setFocusPolicy(Qt::StrongFocus);
        m_displayLabel->setAcceptDrops(true);
        m_displayLabel->setAcceptHoverEvents(true);
        m_displayLabel->setAcceptedMouseButtons(Qt::MouseButton::LeftButton);

        paddingLayout->addItem(m_displayLabel);
        setLayout(paddingLayout);
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::OnAddedToScene()
    {
        m_displayLabel->installSceneEventFilter(this);
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::RefreshStyle()
    {
        switch (m_styleState)
        {
        case StyleState::Empty:
            m_displayLabel->SetStyle(m_wrapperLayoutComponent.GetEntityId(), Styling::Elements::WrapperNode::ActionLabelEmptyNodes);
            break;
        case StyleState::HasElements:
            m_displayLabel->SetStyle(m_wrapperLayoutComponent.GetEntityId(), Styling::Elements::WrapperNode::ActionLabel);
            break;
        }
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::SetActionString(const QString& displayString)
    {
        m_displayLabel->SetLabel(displayString.toUtf8().data());
    }

    void WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::SetStyleState(StyleState state)
    {
        if (m_styleState != state)
        {
            m_styleState = state;
            RefreshStyle();
        }
    }

    bool WrapperNodeLayoutComponent::WrappedNodeActionGraphicsWidget::sceneEventFilter(QGraphicsItem*, QEvent* event)
    {
        switch (event->type())
        {
            case QEvent::GraphicsSceneDragEnter:
            {
                QGraphicsSceneDragDropEvent* dragDropEvent = static_cast<QGraphicsSceneDragDropEvent*>(event);

                if (m_wrapperLayoutComponent.ShouldAcceptDrop(dragDropEvent->mimeData()))
                {
                    m_acceptDrop = true;

                    dragDropEvent->accept();
                    dragDropEvent->acceptProposedAction();

                    m_displayLabel->GetStyleHelper().AddSelector(Styling::States::ValidDrop);
                }
                else
                {
                    m_acceptDrop = false;

                    m_displayLabel->GetStyleHelper().AddSelector(Styling::States::InvalidDrop);
                }

                m_displayLabel->update();
                return true;
            }
            case QEvent::GraphicsSceneDragLeave:
            {
                event->accept();
                if (m_acceptDrop)
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::ValidDrop);

                    m_acceptDrop = false;
                    m_wrapperLayoutComponent.OnDragLeave();
                }
                else
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::InvalidDrop);
                }

                m_displayLabel->update();

                return true;
            }
            case QEvent::GraphicsSceneDrop:
            {
                if (m_acceptDrop)
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::ValidDrop);
                }
                else
                {
                    m_displayLabel->GetStyleHelper().RemoveSelector(Styling::States::InvalidDrop);
                }

                m_displayLabel->update();
                break;
            }
            case QEvent::GraphicsSceneMousePress:
            {
                return true;
            }
            case QEvent::GraphicsSceneMouseRelease:
            {
                QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
                m_wrapperLayoutComponent.OnActionWidgetClicked(mouseEvent->scenePos(), mouseEvent->screenPos());
                return true;
            }
            default:
                break;
        }

        return false;
    }
    
    ///////////////////////////////
    // WrapperNodeLayoutComponent
    ///////////////////////////////
    
    void WrapperNodeLayoutComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<WrappedNodeConfiguration>()
                ->Version(1)
                ->Field("LayoutOrder", &WrappedNodeConfiguration::m_layoutOrder)
                ->Field("ElementOrder", &WrappedNodeConfiguration::m_elementOrdering)
            ;
            serializeContext->Class<WrapperNodeLayoutComponent, NodeLayoutComponent>()
                ->Version(2)
                ->Field("ElementOrdering", &WrapperNodeLayoutComponent::m_elementCounter)
                ->Field("WrappedNodeConfigurations", &WrapperNodeLayoutComponent::m_wrappedNodeConfigurations)
            ;
        }
    }

    AZ::Entity* WrapperNodeLayoutComponent::CreateWrapperNodeEntity(const char* nodeType)
    {
        // Create this Node's entity.
        AZ::Entity* entity = NodeComponent::CreateCoreNodeEntity();

        entity->CreateComponent<GeneralNodeFrameComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::WrapperNode::Node, AZ::EntityId(), nodeType);
        entity->CreateComponent<WrapperNodeLayoutComponent>();
        entity->CreateComponent<GeneralNodeTitleComponent>();
        entity->CreateComponent<GeneralSlotLayoutComponent>();
        entity->CreateComponent<NodeLayerControllerComponent>();

        return entity;
    }

    WrapperNodeLayoutComponent::WrapperNodeLayoutComponent()
        : m_elementCounter(0)
        , m_wrappedNodes(WrappedNodeConfigurationComparator(&m_wrappedNodeConfigurations))
        , m_title(nullptr)
        , m_slots(nullptr)
        , m_wrappedNodeLayout(nullptr)
    {
    }

    WrapperNodeLayoutComponent::~WrapperNodeLayoutComponent()
    {
    }

    void WrapperNodeLayoutComponent::Init()
    {
        NodeLayoutComponent::Init();

        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        GetLayoutAs<QGraphicsLinearLayout>()->setInstantInvalidatePropagation(true);
        
        m_wrappedNodeLayout = aznew WrappedNodeLayout((*this));
        m_wrapperNodeActionWidget = aznew WrappedNodeActionGraphicsWidget((*this));
        
        for (auto& mapPair : m_wrappedNodeConfigurations)
        {
            m_wrappedNodes.insert(mapPair.first);
        }
    }

    void WrapperNodeLayoutComponent::Activate()
    {
        NodeLayoutComponent::Activate();

        SceneMemberNotificationBus::MultiHandler::BusConnect(GetEntityId());
        NodeNotificationBus::MultiHandler::BusConnect(GetEntityId());
        WrapperNodeRequestBus::Handler::BusConnect(GetEntityId());
    }

    void WrapperNodeLayoutComponent::Deactivate()
    {
        ClearLayout();

        NodeLayoutComponent::Deactivate();

        NodeNotificationBus::MultiHandler::BusDisconnect();
        SceneMemberNotificationBus::MultiHandler::BusDisconnect();

        WrapperNodeRequestBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
    }

    void WrapperNodeLayoutComponent::SetActionString(const QString& actionString)
    {
        m_wrapperNodeActionWidget->SetActionString(actionString);
    }

    AZStd::vector< AZ::EntityId > WrapperNodeLayoutComponent::GetWrappedNodeIds() const
    {
        return AZStd::vector< AZ::EntityId >(m_wrappedNodes.begin(), m_wrappedNodes.end());
    }
    
    void WrapperNodeLayoutComponent::WrapNode(const AZ::EntityId& nodeId, const WrappedNodeConfiguration& nodeConfiguration)
    {
        if (m_wrappedNodeConfigurations.find(nodeId) == m_wrappedNodeConfigurations.end())
        {
            NodeNotificationBus::MultiHandler::BusConnect(nodeId);
            SceneMemberNotificationBus::MultiHandler::BusConnect(nodeId);

            NodeRequestBus::Event(nodeId, &NodeRequests::SetWrappingNode, GetEntityId());
            WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnWrappedNode, nodeId);

            m_wrappedNodeConfigurations[nodeId] = nodeConfiguration;
            m_wrappedNodeConfigurations[nodeId].m_elementOrdering = m_elementCounter;
            
            ++m_elementCounter;
            
            m_wrappedNodes.insert(nodeId);
            m_wrappedNodeLayout->RefreshLayout();

            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);

            RootGraphicsItemEnabledState enabledState = RootGraphicsItemEnabledState::ES_Enabled;
            RootGraphicsItemRequestBus::EventResult(enabledState, GetEntityId(), &RootGraphicsItemRequests::GetEnabledState);

            RootGraphicsItemRequestBus::Event(nodeId, &RootGraphicsItemRequests::SetEnabledState, enabledState);

            RefreshActionStyle();
        }
    }
    
    void WrapperNodeLayoutComponent::UnwrapNode(const AZ::EntityId& nodeId)
    {
        auto configurationIter = m_wrappedNodeConfigurations.find(nodeId);

        if (configurationIter != m_wrappedNodeConfigurations.end())
        {
            SceneMemberNotificationBus::MultiHandler::BusDisconnect(nodeId);
            NodeNotificationBus::MultiHandler::BusDisconnect(nodeId);
            NodeRequestBus::Event(nodeId, &NodeRequests::SetWrappingNode, AZ::EntityId());
            WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnUnwrappedNode, nodeId);

            m_wrappedNodes.erase(nodeId);
            m_wrappedNodeConfigurations.erase(configurationIter);

            m_wrappedNodeLayout->RefreshLayout();

            NodeUIRequestBus::Event(GetEntityId(), &NodeUIRequests::AdjustSize);

            // If we unwrap something just set it to enabled.
            RootGraphicsItemRequestBus::Event(nodeId, &RootGraphicsItemRequests::SetEnabledState, RootGraphicsItemEnabledState::ES_Enabled);

            RefreshActionStyle();
        }
    }

    void WrapperNodeLayoutComponent::SetWrapperType(const AZ::Crc32& wrapperType)
    {
        m_wrapperType = wrapperType;
    }

    AZ::Crc32 WrapperNodeLayoutComponent::GetWrapperType() const
    {
        return m_wrapperType;
    }

    void WrapperNodeLayoutComponent::OnNodeActivated()
    {
        AZ::EntityId nodeId = (*NodeNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            CreateLayout();
        }
    }

    void WrapperNodeLayoutComponent::OnAddedToScene(const AZ::EntityId& /*sceneId*/)
    {
        AZ::EntityId nodeId = (*NodeNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            for (const AZ::EntityId& wrappedNodeId : m_wrappedNodes)
            {
                NodeNotificationBus::MultiHandler::BusConnect(wrappedNodeId);

                // Test to make sure the node is activated before we signal out anything to it.
                //
                // We listen for when the node activates, so these calls will be handled there.
                if (NodeRequestBus::FindFirstHandler(wrappedNodeId) != nullptr)
                {
                    NodeRequestBus::Event(wrappedNodeId, &NodeRequests::SetWrappingNode, GetEntityId());
                    WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnWrappedNode, wrappedNodeId);
                }
            }

            RefreshActionStyle();
            UpdateLayout();
            OnStyleChanged();

            // Event filtering for graphics items can only be done by items in the same scene.
            // and by objects that are in a scene. So I need to wait for them to be added to the
            // scene before installing my filters.
            m_wrapperNodeActionWidget->OnAddedToScene();

            StyleNotificationBus::Handler::BusConnect(GetEntityId());
        }
        else
        {
            NodeRequestBus::Event(nodeId, &NodeRequests::SetWrappingNode, GetEntityId());
            WrapperNodeNotificationBus::Event(GetEntityId(), &WrapperNodeNotifications::OnWrappedNode, nodeId);

            // Sort ick, but should work for now.
            // Mostly ick because it'll redo this layout waaaay to many times.
            UpdateLayout();
        }
    }

    void WrapperNodeLayoutComponent::OnSceneMemberAboutToSerialize(GraphSerialization& sceneSerialization)
    {
        AZ::EntityId nodeId = (*SceneMemberNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            AZStd::unordered_set<AZ::EntityId> memberIds;
            memberIds.insert(m_wrappedNodes.begin(), m_wrappedNodes.end());

            GraphUtils::ParseMembersForSerialization(sceneSerialization, memberIds);
        }
    }

    void WrapperNodeLayoutComponent::OnSceneMemberDeserialized(const AZ::EntityId& /*graphId*/, const GraphSerialization& sceneSerialization)
    {
        AZ::EntityId nodeId = (*SceneMemberNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            m_elementCounter = 0;
            m_wrappedNodes.clear();

            WrappedNodeConfigurationMap oldConfigurations = m_wrappedNodeConfigurations;
            m_wrappedNodeConfigurations.clear();

            for (const auto& configurationPair : oldConfigurations)
            {
                if (sceneSerialization.FindRemappedEntityId(configurationPair.first).IsValid())
                {
                    m_wrappedNodeConfigurations.insert(configurationPair);
                    m_wrappedNodes.insert(configurationPair.first);
                }
            }
        }
    }

    void WrapperNodeLayoutComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
    {
        AZ::EntityId nodeId = (*SceneMemberNotificationBus::GetCurrentBusId());

        if (nodeId == GetEntityId())
        {
            // We are about to remove everything.
            // So we don't really need to update ourselves to keep our state in order.
            SceneMemberNotificationBus::MultiHandler::BusDisconnect();

            AZStd::unordered_set< AZ::EntityId > deleteNodes(m_wrappedNodes.begin(), m_wrappedNodes.end());
            SceneRequestBus::Event(sceneId, &SceneRequests::Delete, deleteNodes);
        }
        else
        {
            UnwrapNode(nodeId);
        }
    }

    void WrapperNodeLayoutComponent::OnStyleChanged()
    {
        m_styleHelper.SetStyle(GetEntityId());

        qreal border = m_styleHelper.GetAttribute(Styling::Attribute::BorderWidth, 0.0);
        qreal spacing = m_styleHelper.GetAttribute(Styling::Attribute::Spacing, 0.0);
        qreal margin = m_styleHelper.GetAttribute(Styling::Attribute::Margin, 0.0);

        GetLayoutAs<QGraphicsLinearLayout>()->setContentsMargins(margin + border, margin + border, margin + border, margin + border);
        GetLayoutAs<QGraphicsLinearLayout>()->setSpacing(spacing);

        m_wrappedNodeLayout->RefreshStyle();
        m_wrapperNodeActionWidget->RefreshStyle();

        RefreshDisplay();
    }    

    void WrapperNodeLayoutComponent::RefreshActionStyle()
    {
        if (!m_wrappedNodes.empty())
        {
            m_wrapperNodeActionWidget->SetStyleState(WrappedNodeActionGraphicsWidget::StyleState::HasElements);
        }
        else
        {
            m_wrapperNodeActionWidget->SetStyleState(WrappedNodeActionGraphicsWidget::StyleState::Empty);
        }
    }

    bool WrapperNodeLayoutComponent::ShouldAcceptDrop(const QMimeData* mimeData) const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        bool shouldAcceptDrop = false;
        GraphModelRequestBus::EventResult(shouldAcceptDrop, sceneId, &GraphModelRequests::ShouldWrapperAcceptDrop, GetEntityId(), mimeData);

        if (shouldAcceptDrop)
        {
            GraphModelRequestBus::Event(sceneId, &GraphModelRequests::AddWrapperDropTarget, GetEntityId());
        }
        return shouldAcceptDrop;
    }

    void WrapperNodeLayoutComponent::OnDragLeave() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        GraphModelRequestBus::Event(sceneId, &GraphModelRequests::RemoveWrapperDropTarget, GetEntityId());
    }

    void WrapperNodeLayoutComponent::OnActionWidgetClicked(const QPointF& scenePoint, const QPoint& screenPoint) const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, sceneId, &SceneRequests::GetEditorId);
        AssetEditorRequestBus::Event(editorId, &AssetEditorRequests::OnWrapperNodeActionWidgetClicked, GetEntityId(), m_wrapperNodeActionWidget->boundingRect().toRect(), scenePoint, screenPoint);
    }
    
    void WrapperNodeLayoutComponent::ClearLayout()
    {
        if (m_layout)
        {
            for (int i=m_layout->count() - 1; i >= 0; --i)
            {
                m_layout->removeAt(i);
            }
        }
    }

    void WrapperNodeLayoutComponent::CreateLayout()
    {
        ClearLayout();

        if (m_title == nullptr)
        {
            NodeTitleRequestBus::EventResult(m_title, GetEntityId(), &NodeTitleRequests::GetGraphicsWidget);
        }

        if (m_slots == nullptr)
        {
            NodeSlotsRequestBus::EventResult(m_slots, GetEntityId(), &NodeSlotsRequests::GetGraphicsLayoutItem);
        }

        if (m_title)
        {
            GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_title);
        }

        if (m_slots)
        {
            GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_slots);
        }

        GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_wrappedNodeLayout);
        GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_wrapperNodeActionWidget);
    }
    
    void WrapperNodeLayoutComponent::UpdateLayout()
    {
        m_wrappedNodeLayout->RefreshLayout();
        
        RefreshDisplay();
    }
    
    void WrapperNodeLayoutComponent::RefreshDisplay()
    {
        m_layout->invalidate();
        m_layout->updateGeometry();
    }
}
