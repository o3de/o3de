/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCoreApplication>

#include <Components/Slots/Execution/ExecutionSlotComponent.h>
#include <Components/Slots/Execution/ExecutionSlotConnectionPin.h>
#include <Components/Slots/Execution/ExecutionSlotLayoutComponent.h>


namespace GraphCanvas
{
    ////////////////////////
    // ExecutionSlotLayout
    ////////////////////////

    ExecutionSlotLayout::ExecutionSlotLayout(ExecutionSlotLayoutComponent& owner)
        : m_connectionType(ConnectionType::CT_Invalid)
        , m_owner(owner)
        , m_textDecoration(nullptr)
    {
        setInstantInvalidatePropagation(true);
        setOrientation(Qt::Horizontal);

        m_slotConnectionPin = aznew ExecutionSlotConnectionPin(owner.GetEntityId());
        m_slotText = aznew GraphCanvasLabel();

        if (const AZ::Entity* ownerEntity = owner.GetEntity())
        {
            if (const SlotComponent* slotComponent = ownerEntity->FindComponent<ExecutionSlotComponent>())
            {
                m_isNameHidden = slotComponent->IsNameHidden();
            }
        }
    }

    ExecutionSlotLayout::~ExecutionSlotLayout()
    {
    }

    void ExecutionSlotLayout::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SlotNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        StyleNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        m_slotConnectionPin->Activate();
    }

    void ExecutionSlotLayout::Deactivate()
    {
        m_slotConnectionPin->Deactivate();
        SceneMemberNotificationBus::Handler::BusDisconnect();
        SlotNotificationBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
    }

    void ExecutionSlotLayout::OnSceneSet(const AZ::EntityId&)
    {
        SlotRequests* slotRequests = SlotRequestBus::FindFirstHandler(m_owner.GetEntityId());

        if (slotRequests)
        {
            m_connectionType = slotRequests->GetConnectionType();

            OnNameChanged(slotRequests->GetName());
            OnTooltipChanged(slotRequests->GetTooltip());

            const SlotConfiguration& configuration = slotRequests->GetSlotConfiguration();

            if (!configuration.m_textDecoration.empty())
            {
                SetTextDecoration(configuration.m_textDecoration, configuration.m_textDecorationToolTip);
            }
        }

        UpdateLayout();
        OnStyleChanged();
    }

    void ExecutionSlotLayout::OnSceneReady()
    {
        OnStyleChanged();
    }

    void ExecutionSlotLayout::OnRegisteredToNode(const AZ::EntityId& /*nodeId*/)
    {
        OnStyleChanged();
    }

    void ExecutionSlotLayout::OnNameChanged(const AZStd::string& name)
    {
        m_slotText->SetLabel(m_isNameHidden ? "" : name);
    }

    void ExecutionSlotLayout::OnTooltipChanged(const AZStd::string& tooltip)
    {
        m_slotConnectionPin->setToolTip(tooltip.c_str());
        m_slotText->setToolTip(tooltip.c_str());
    }

    void ExecutionSlotLayout::OnStyleChanged()
    {
        m_style.SetStyle(m_owner.GetEntityId());

        ApplyTextStyle(m_slotText);

        if (m_textDecoration)
        {
            ApplyTextStyle(m_textDecoration);
        }

        m_slotConnectionPin->RefreshStyle();

        qreal padding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);
        setContentsMargins(padding, padding, padding, padding);
        setSpacing(m_style.GetAttribute(Styling::Attribute::Spacing, 2.));

        UpdateGeometry();
    }

    void ExecutionSlotLayout::SetTextDecoration(const AZStd::string& textDecoration, const AZStd::string& toolTip)
    {
        if (m_textDecoration)
        {
            delete m_textDecoration;
            m_textDecoration = nullptr;
        }

        if (!textDecoration.empty())
        {
            m_textDecoration = new GraphCanvasLabel();
            m_textDecoration->SetLabel(textDecoration);
            m_textDecoration->setToolTip(toolTip.c_str());

            ApplyTextStyle(m_textDecoration);
        }
    }

    void ExecutionSlotLayout::ClearTextDecoration()
    {
        delete m_textDecoration;
        m_textDecoration = nullptr;
    }

    void ExecutionSlotLayout::ApplyTextStyle(GraphCanvasLabel* graphCanvasLabel)
    {
        switch (m_connectionType)
        {
        case ConnectionType::CT_Input:
            graphCanvasLabel->SetStyle(m_owner.GetEntityId(), ".inputSlotName");
            break;
        case ConnectionType::CT_Output:
            graphCanvasLabel->SetStyle(m_owner.GetEntityId(), ".outputSlotName");
            break;
        default:
            graphCanvasLabel->SetStyle(m_owner.GetEntityId(), ".slotName");
            break;
        };
    }

    void ExecutionSlotLayout::UpdateLayout()
    {
        for (int i = count() - 1; i >= 0; --i)
        {
            removeAt(i);
        }

        switch (m_connectionType)
        {
        case ConnectionType::CT_Input:
            addItem(m_slotConnectionPin);
            setAlignment(m_slotConnectionPin, Qt::AlignLeft);

            addItem(m_slotText);
            setAlignment(m_slotText, Qt::AlignLeft);

            if (m_textDecoration)
            {
                addItem(m_textDecoration);
                setAlignment(m_textDecoration, Qt::AlignLeft);
            }

            break;
        case ConnectionType::CT_Output:

            if (m_textDecoration)
            {
                addItem(m_textDecoration);
                setAlignment(m_textDecoration, Qt::AlignRight);
            }

            addItem(m_slotText);
            setAlignment(m_slotText, Qt::AlignRight);

            addItem(m_slotConnectionPin);
            setAlignment(m_slotConnectionPin, Qt::AlignRight);
            break;
        default:
            if (m_textDecoration)
            {
                addItem(m_textDecoration);
            }

            addItem(m_slotConnectionPin);
            addItem(m_slotText);
            break;
        }
    }

    void ExecutionSlotLayout::UpdateGeometry()
    {
        m_slotConnectionPin->updateGeometry();
        m_slotText->update();

        invalidate();
        updateGeometry();
    }

    /////////////////////////////////
    // ExecutionSlotLayoutComponent
    /////////////////////////////////

    void ExecutionSlotLayoutComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<ExecutionSlotLayoutComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    ExecutionSlotLayoutComponent::ExecutionSlotLayoutComponent()
        : m_layout(nullptr)
    {
    }

    void ExecutionSlotLayoutComponent::Init()
    {
        SlotLayoutComponent::Init();

        m_layout = aznew ExecutionSlotLayout((*this));
        SetLayout(m_layout);
    }

    void ExecutionSlotLayoutComponent::Activate()
    {
        SlotLayoutComponent::Activate();
        m_layout->Activate();
    }

    void ExecutionSlotLayoutComponent::Deactivate()
    {
        SlotLayoutComponent::Deactivate();
        m_layout->Deactivate();
    }
}
