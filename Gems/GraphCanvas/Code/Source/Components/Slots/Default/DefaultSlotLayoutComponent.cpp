/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <Components/Slots/Default/DefaultSlotLayoutComponent.h>

#include <Components/Slots/SlotConnectionPin.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    //////////////////////
    // DefaultSlotLayout
    //////////////////////

    DefaultSlotLayout::DefaultSlotLayout(DefaultSlotLayoutComponent& owner)
        : m_owner(owner)
    {
        m_slotConnectionPin = aznew SlotConnectionPin(owner.GetEntityId());
        m_slotText = aznew GraphCanvasLabel();

        OnStyleChanged();
    }

    DefaultSlotLayout::~DefaultSlotLayout()
    {
    }

    void DefaultSlotLayout::Activate()
    {
        StyleNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SceneMemberNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        m_slotConnectionPin->Activate();
    }

    void DefaultSlotLayout::Deactivate()
    {
        m_slotConnectionPin->Deactivate();
        StyleNotificationBus::Handler::BusDisconnect();
        SceneMemberNotificationBus::Handler::BusDisconnect();
    }

    void DefaultSlotLayout::OnSceneSet(const AZ::EntityId&)
    {
        AZStd::string slotName;
        SlotRequestBus::EventResult(slotName, m_owner.GetEntityId(), &SlotRequests::GetName);

        SlotRequestBus::EventResult(m_connectionType, m_owner.GetEntityId(), &SlotRequests::GetConnectionType);

        m_slotText->SetLabel(slotName);

        UpdateLayout();
        OnStyleChanged();
    }

    void DefaultSlotLayout::OnSceneReady()
    {
        UpdateLayout();
        OnStyleChanged();
    }

    void DefaultSlotLayout::OnStyleChanged()
    {
        m_style.SetStyle(m_owner.GetEntityId());

        m_slotText->SetStyle(m_owner.GetEntityId(), ".slotName");
        m_slotConnectionPin->RefreshStyle();

        qreal padding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);
        setContentsMargins(padding, padding, padding, padding);
        setSpacing(m_style.GetAttribute(Styling::Attribute::Spacing, 2.));

        UpdateGeometry();
    }

    void DefaultSlotLayout::UpdateLayout()
    {
        for (int i = count() - 1; i >= 0; --i)
        {
            removeAt(i);
        }

        switch (m_connectionType)
        {
        case ConnectionType::CT_Input:
            setOrientation(Qt::Horizontal);
            addItem(m_slotConnectionPin);
            addItem(m_slotText);
            setAlignment(m_slotConnectionPin, Qt::AlignLeft);
            setAlignment(m_slotText, Qt::AlignLeft);
            break;
        case ConnectionType::CT_Output:
            setOrientation(Qt::Horizontal);
            addItem(m_slotText);
            addItem(m_slotConnectionPin);
            setAlignment(m_slotText, Qt::AlignRight);
            setAlignment(m_slotConnectionPin, Qt::AlignRight);
            break;
        default:
            setOrientation(Qt::Horizontal);
            addItem(m_slotConnectionPin);
            addItem(m_slotText);
            break;
        }

        UpdateGeometry();
    }

    void DefaultSlotLayout::UpdateGeometry()
    {
        m_slotConnectionPin->updateGeometry();
        m_slotText->update();

        invalidate();
        updateGeometry();
    }

    ///////////////////////////////
    // DefaultSlotLayoutComponent
    ///////////////////////////////

    void DefaultSlotLayoutComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<DefaultSlotLayoutComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    DefaultSlotLayoutComponent::DefaultSlotLayoutComponent()
        : m_defaultSlotLayout(nullptr)
    {
    }

    void DefaultSlotLayoutComponent::Init()
    {
        SlotLayoutComponent::Init();

        m_defaultSlotLayout = aznew DefaultSlotLayout((*this));
        SetLayout(m_defaultSlotLayout);
    }

    void DefaultSlotLayoutComponent::Activate()
    {
        SlotLayoutComponent::Activate();
        m_defaultSlotLayout->Activate();
    }

    void DefaultSlotLayoutComponent::Deactivate()
    {
        m_defaultSlotLayout->Deactivate();
        SlotLayoutComponent::Deactivate();
    }
}
