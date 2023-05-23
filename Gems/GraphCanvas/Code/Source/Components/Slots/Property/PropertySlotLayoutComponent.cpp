/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCoreApplication>
#include <qgraphicslayoutitem.h>
#include <qgraphicsscene.h>
#include <qsizepolicy.h>

#include <Components/Slots/Property/PropertySlotLayoutComponent.h>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/Slots/Property/PropertySlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/tools.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    ///////////////////////
    // PropertySlotLayout
    ///////////////////////

    PropertySlotLayout::PropertySlotLayout(PropertySlotLayoutComponent& owner)
        : m_connectionType(ConnectionType::CT_Invalid)
        , m_owner(owner)
        , m_nodePropertyDisplay(nullptr)
    {
        setInstantInvalidatePropagation(true);
        setOrientation(Qt::Horizontal);

        m_spacer = new QGraphicsWidget();
        m_spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        m_spacer->setAutoFillBackground(true);
        m_spacer->setMinimumSize(0, 0);
        m_spacer->setPreferredWidth(0);
        m_spacer->setMaximumHeight(0);

        m_nodePropertyDisplay = aznew NodePropertyDisplayWidget();
        m_slotText = aznew GraphCanvasLabel();        
    }
    
    PropertySlotLayout::~PropertySlotLayout()
    {
    }

    void PropertySlotLayout::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SlotNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        StyleNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
    }

    void PropertySlotLayout::Deactivate()
    {
        StyleNotificationBus::Handler::BusDisconnect();
        SlotNotificationBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void PropertySlotLayout::OnSceneSet(const AZ::EntityId&)
    {
        SlotRequestBus::EventResult(m_connectionType, m_owner.GetEntityId(), &SlotRequests::GetConnectionType);

        AZStd::string slotName;
        SlotRequestBus::EventResult(slotName, m_owner.GetEntityId(), &SlotRequests::GetName);

        m_slotText->SetLabel(slotName);

        AZStd::string toolTip;
        SlotRequestBus::EventResult(toolTip, m_owner.GetEntityId(), &SlotRequests::GetTooltip);

        OnTooltipChanged(toolTip);

        TryAndSetupSlot();
    }

    void PropertySlotLayout::OnSceneReady()
    {
        TryAndSetupSlot();
    }

    void PropertySlotLayout::OnRegisteredToNode(const AZ::EntityId& /*nodeId*/)
    {
        TryAndSetupSlot();
    }

    void PropertySlotLayout::OnTooltipChanged(const AZStd::string& tooltip)
    {
        m_slotText->setToolTip(Tools::qStringFromUtf8(tooltip));
        m_nodePropertyDisplay->setToolTip(Tools::qStringFromUtf8(tooltip));
    }
    
    void PropertySlotLayout::OnStyleChanged()
    {
        m_style.SetStyle(m_owner.GetEntityId());
        
        m_nodePropertyDisplay->RefreshStyle();

        switch (m_connectionType)
        {
        case ConnectionType::CT_Input:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".inputSlotName");
            break;
        case ConnectionType::CT_Output:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".outputSlotName");
            break;
        default:
            m_slotText->SetStyle(m_owner.GetEntityId(), ".slotName");
            break;
        };

        qreal padding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);
        setContentsMargins(padding, padding, padding, padding);
        setSpacing(m_style.GetAttribute(Styling::Attribute::Spacing, 2.));

        UpdateGeometry();
    }

    void PropertySlotLayout::TryAndSetupSlot()
    {
        if (m_nodePropertyDisplay->GetNodePropertyDisplay() == nullptr)
        {
            CreateDataDisplay();
        }
    }
    
    void PropertySlotLayout::CreateDataDisplay()
    {
        if (m_connectionType == ConnectionType::CT_Input)
        {
            AZ::EntityId sceneId;
            SceneMemberRequestBus::EventResult(sceneId, m_owner.GetEntityId(), &SceneMemberRequests::GetScene);

            AZ::EntityId nodeId;
            SlotRequestBus::EventResult(nodeId, m_owner.GetEntityId(), &SlotRequests::GetNode);

            AZ::Crc32 propertyId;
            PropertySlotRequestBus::EventResult(propertyId, m_owner.GetEntityId(), &PropertySlotRequests::GetPropertyId);

            NodePropertyDisplay* nodePropertyDisplay = nullptr;
            GraphModelRequestBus::EventResult(nodePropertyDisplay, sceneId, &GraphModelRequests::CreatePropertySlotPropertyDisplay, propertyId, nodeId, m_owner.GetEntityId());

            if (nodePropertyDisplay)
            {
                nodePropertyDisplay->SetNodeId(nodeId);
                nodePropertyDisplay->SetSlotId(m_owner.GetEntityId());

                m_nodePropertyDisplay->SetNodePropertyDisplay(nodePropertyDisplay);
                
                UpdateLayout();
                OnStyleChanged();
            }
        }
        else
        {
            UpdateLayout();
            OnStyleChanged();
        }
    }

    void PropertySlotLayout::UpdateLayout()
    {
        for (int i = count() - 1; i >= 0; --i)
        {
            removeAt(i);
        }

        switch (m_connectionType)
        {
        case ConnectionType::CT_Input:
            addItem(m_slotText);
            setAlignment(m_slotText, Qt::AlignLeft);

            addItem(m_nodePropertyDisplay);
            setAlignment(m_slotText, Qt::AlignLeft);

            addItem(m_spacer);
            setAlignment(m_spacer, Qt::AlignLeft);
            break;
        case ConnectionType::CT_Output:
            addItem(m_spacer);
            setAlignment(m_spacer, Qt::AlignRight);
            
            addItem(m_slotText);
            setAlignment(m_slotText, Qt::AlignRight);
            break;
        default:
            addItem(m_slotText);
            addItem(m_spacer);
            break;
        }

        UpdateGeometry();
    }
    
    void PropertySlotLayout::UpdateGeometry()
    {
        m_slotText->update();
        
        invalidate();
        updateGeometry();
    }
    
    ////////////////////////////////
    // PropertySlotLayoutComponent
    ////////////////////////////////

    void PropertySlotLayoutComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<PropertySlotLayoutComponent, AZ::Component>()
                ->Version(1)
            ;
        }
    }
    
    PropertySlotLayoutComponent::PropertySlotLayoutComponent()
        : m_layout(nullptr)
    {
    }
    
    void PropertySlotLayoutComponent::Init()
    {
        SlotLayoutComponent::Init();
        
        m_layout = aznew PropertySlotLayout((*this));
        SetLayout(m_layout);
    }
    
    void PropertySlotLayoutComponent::Activate()
    {
        SlotLayoutComponent::Activate();
        m_layout->Activate();
    }

    void PropertySlotLayoutComponent::Deactivate()
    {
        m_layout->Deactivate();
        SlotLayoutComponent::Deactivate();        
    }
}
