/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <functional>

#include <AzCore/RTTI/TypeInfo.h>
#include <QGraphicsLayoutItem>
#include <QGraphicsGridLayout>

#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/NodeLayerControllerComponent.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <Components/Nodes/General/GeneralNodeTitleComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/tools.h>
#include <GraphCanvas/Styling/StyleHelper.h>

namespace GraphCanvas
{
    ///////////////////////////////
    // GeneralNodeLayoutComponent
    ///////////////////////////////

    void GeneralNodeLayoutComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GeneralNodeLayoutComponent, NodeLayoutComponent>()
                ->Version(1)
                ;
        }
    }

    AZ::Entity* GeneralNodeLayoutComponent::CreateGeneralNodeEntity(const char* nodeType, const NodeConfiguration& configuration)
    {
        
        // Create this Node's entity.
        AZ::Entity* entity = NodeComponent::CreateCoreNodeEntity(configuration);

        entity->CreateComponent<GeneralNodeFrameComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::Node, AZ::EntityId(), nodeType);        
        entity->CreateComponent<GeneralNodeLayoutComponent>();
        entity->CreateComponent<GeneralNodeTitleComponent>();
        entity->CreateComponent<GeneralSlotLayoutComponent>();
        entity->CreateComponent<NodeLayerControllerComponent>();

        return entity;
    }

    GeneralNodeLayoutComponent::GeneralNodeLayoutComponent()
        : m_title(nullptr)
        , m_slots(nullptr)
    {
    }

    GeneralNodeLayoutComponent::~GeneralNodeLayoutComponent()
    {
    }

    void GeneralNodeLayoutComponent::Init()
    {
        NodeLayoutComponent::Init();

        m_slots = new QGraphicsLinearLayout(Qt::Vertical);
        m_slots->setInstantInvalidatePropagation(true);

        m_title = new QGraphicsLinearLayout(Qt::Horizontal);
        m_title->setInstantInvalidatePropagation(true);
    }

    void GeneralNodeLayoutComponent::Activate()
    {
        NodeLayoutComponent::Activate();
        NodeNotificationBus::Handler::BusConnect(GetEntityId());
    }
    
    void GeneralNodeLayoutComponent::Deactivate()
    {
        NodeLayoutComponent::Deactivate();

        StyleNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
    }
    
    void GeneralNodeLayoutComponent::OnStyleChanged()
    {
        UpdateLayoutParameters();
    }

    void GeneralNodeLayoutComponent::OnNodeActivated()
    {
        StyleNotificationBus::Handler::BusConnect(GetEntityId());
        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        m_layout->setInstantInvalidatePropagation(true);

        NodeTitleRequestBus::EventResult(m_titleGraphicsItem, GetEntityId(), &NodeTitleRequests::GetGraphicsWidget);
        NodeSlotsRequestBus::EventResult(m_slotsGraphicsItem, GetEntityId(), &NodeSlotsRequestBus::Events::GetGraphicsLayoutItem);

        // In the future, we could change this to allow the user to specify what slot types they want their title between
        NodeSlotsRequestBus::EventResult(m_slotLayout, GetEntityId(), &NodeSlotsRequestBus::Events::GetLinearLayout, SlotGroups::DataGroup);
        NodeSlotsRequestBus::EventResult(m_horizontalSpacer, GetEntityId(), &NodeSlotsRequestBus::Events::GetSpacer, SlotGroups::DataGroup);

        if (m_titleGraphicsItem)
        {
            m_title->addItem(m_titleGraphicsItem);
        }
        if (m_slotsGraphicsItem)
        {
            m_slots->addItem(m_slotsGraphicsItem);
        }
        m_title->setContentsMargins(0, 0, 0, 0);

        UpdateLayoutParameters();
    }

    void GeneralNodeLayoutComponent::UpdateLayoutParameters()
    {
        Styling::StyleHelper style(GetEntityId());
        Qt::Orientation layoutOrientation = style.GetAttribute(Styling::Attribute::LayoutOrientation, Qt::Vertical);

        if (m_layoutOrientation != layoutOrientation)
        {
            m_layoutOrientation = layoutOrientation;

            GetLayoutAs<QGraphicsLinearLayout>()->setOrientation(layoutOrientation);

            if (m_slotLayout)
            {
                m_slotLayout->removeItem(m_title);
                if (m_horizontalSpacer)
                {
                    m_slotLayout->removeItem(m_horizontalSpacer);
                }
            }
            
            GetLayoutAs<QGraphicsLinearLayout>()->removeItem(m_title);
            GetLayoutAs<QGraphicsLinearLayout>()->removeItem(m_slots);

            if (m_layoutOrientation == Qt::Vertical)
            {
                GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_title);

                if (m_slotLayout)
                {
                    m_slotLayout->setContentsMargins(5.0, 5.0, 5.0, 5.0);
                    if (m_horizontalSpacer)
                    {
                        m_slotLayout->insertItem(1, m_horizontalSpacer);
                    }
                }
                
                m_slots->setContentsMargins(0, 0, 0, 0);
                GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_slots);
            }
            else
            {
                m_title->setSpacing(0);
                m_title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                m_title->setPreferredSize(0, 0);

                // Insert the title into slot layout so it appears between the input and output slots
                if (m_slotLayout)
                {
                    m_slotLayout->insertItem(1, m_title);
                    m_slotLayout->setContentsMargins(0, 0, 0, 0);
                }
                
                m_slots->setContentsMargins(3.0, 0, 3.0, 0);
                GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_slots);
            }
        }

        qreal border = m_layoutOrientation == Qt::Vertical ? style.GetAttribute(Styling::Attribute::BorderWidth, 0.0) : 0.0;
        qreal spacing = style.GetAttribute(Styling::Attribute::Spacing, 4.0);
        qreal margin = style.GetAttribute(Styling::Attribute::Margin, 4.0);

        GetLayoutAs<QGraphicsLinearLayout>()->setContentsMargins(margin + border, margin + border, margin + border, margin + border);
        GetLayoutAs<QGraphicsLinearLayout>()->setSpacing(spacing);

        GetLayout()->invalidate();
    }
}
