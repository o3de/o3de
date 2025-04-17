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

        m_layout = new QGraphicsLinearLayout(Qt::Vertical);
        m_layout->setInstantInvalidatePropagation(true);
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
        Styling::StyleHelper style(GetEntityId());
        Qt::Orientation layoutOrientation = style.GetAttribute(Styling::Attribute::LayoutOrientation, Qt::Vertical);

        if (layoutOrientation == Qt::Horizontal)
        {
            UpdateHorizontalLayout();
        }
        UpdateLayoutParameters();
    }

    void GeneralNodeLayoutComponent::OnNodeActivated()
    {
        QGraphicsWidget* titleGraphicsItem = nullptr;
        NodeTitleRequestBus::EventResult(titleGraphicsItem, GetEntityId(), &NodeTitleRequests::GetGraphicsWidget);
        if (titleGraphicsItem)
        {
            m_title->addItem(titleGraphicsItem);
            m_title->setContentsMargins(0, 0, 0, 0);
        }
        GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_title);

        QGraphicsLayoutItem* slotsGraphicsItem = nullptr;
        NodeSlotsRequestBus::EventResult(slotsGraphicsItem, GetEntityId(), &NodeSlotsRequestBus::Events::GetGraphicsLayoutItem);
        if (slotsGraphicsItem)
        {
            m_slots->addItem(slotsGraphicsItem);
            m_slots->setContentsMargins(0, 0, 0, 0);
        }
        GetLayoutAs<QGraphicsLinearLayout>()->addItem(m_slots);

        StyleNotificationBus::Handler::BusConnect(GetEntityId());

        UpdateLayoutParameters();
    }

    void GeneralNodeLayoutComponent::UpdateHorizontalLayout()
    {
        QGraphicsLinearLayout* slotLayout;
        QGraphicsWidget* horizontalSpacer;

        NodeSlotsRequestBus::EventResult(slotLayout, GetEntityId(), &NodeSlotsRequestBus::Events::GetLinearLayout, SlotGroups::DataGroup);
        NodeSlotsRequestBus::EventResult(horizontalSpacer, GetEntityId(), &NodeSlotsRequestBus::Events::GetSpacer, SlotGroups::DataGroup);

        GetLayoutAs<QGraphicsLinearLayout>()->setOrientation(Qt::Horizontal);

        if (slotLayout && horizontalSpacer && horizontalSpacer->parentLayoutItem() == slotLayout)
        {
            slotLayout->removeItem(horizontalSpacer);
        }

        if (m_title->parentLayoutItem() == GetLayoutAs<QGraphicsLinearLayout>())
        {
            GetLayoutAs<QGraphicsLinearLayout>()->removeItem(m_title);

            // Insert the title into slot layout so it appears between the input and output slots
            if (slotLayout)
            {
                slotLayout->insertItem(1, m_title);
                slotLayout->setContentsMargins(0, 0, 0, 0);

                // Center the title and slots vertically
                for (int i = 0; i < slotLayout->count(); i++)
                {
                    slotLayout->setAlignment(slotLayout->itemAt(i), Qt::AlignVCenter);
                }
            }
        }
        
        m_title->setSpacing(0);
        m_title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        m_title->setPreferredSize(0, 0);
        m_title->setContentsMargins(0, 0, 0, 0);

        m_slots->setContentsMargins(3.0, 0, 3.0, 0);
    }

    void GeneralNodeLayoutComponent::UpdateLayoutParameters()
    {
        Styling::StyleHelper style(GetEntityId());
        Qt::Orientation layoutOrientation = style.GetAttribute(Styling::Attribute::LayoutOrientation, Qt::Vertical);

        qreal border = layoutOrientation == Qt::Vertical ? style.GetAttribute(Styling::Attribute::BorderWidth, 0.0) : 0.0;
        qreal spacing = style.GetAttribute(Styling::Attribute::Spacing, 4.0);
        qreal margin = style.GetAttribute(Styling::Attribute::Margin, 4.0);

        GetLayoutAs<QGraphicsLinearLayout>()->setContentsMargins(margin + border, margin + border, margin + border, margin + border);
        GetLayoutAs<QGraphicsLinearLayout>()->setSpacing(spacing);

        GetLayout()->invalidate();
    }
}
