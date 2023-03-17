/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QCoreApplication>
#include <QGraphicsLayoutItem>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QSizePolicy>


#include <Components/Slots/Extender/ExtenderSlotLayoutComponent.h>

#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/GraphicsItems/GraphCanvasSceneEventFilter.h>
#include <Components/Slots/Extender/ExtenderSlotConnectionPin.h>
#include <Widgets/GraphCanvasLabel.h>

namespace GraphCanvas
{
    class ExtenderLabelEventFilter
        : public SceneEventFilter
    {
    public:
        AZ_CLASS_ALLOCATOR(ExtenderLabelEventFilter, AZ::SystemAllocator);

        ExtenderLabelEventFilter(const SlotId& slotId)
            : SceneEventFilter(nullptr)
            , m_trackClick(false)
            , m_slotId(slotId)
        {
        }

        bool sceneEventFilter(QGraphicsItem* item, QEvent* event) override
        {
            switch (event->type())
            {
            case QEvent::GraphicsSceneMousePress:
            {
                m_trackClick = true;
                return true;
            }
            case QEvent::GraphicsSceneMouseRelease:
            {
                QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);

                if (m_trackClick && item->sceneBoundingRect().contains(mouseEvent->scenePos()))
                {
                    m_trackClick = false;
                    ExtenderSlotRequestBus::Event(m_slotId, &ExtenderSlotRequests::TriggerExtension);
                }
                return true;
            }
            default:
                break;
            }

            return false;
        }

    private:

        bool m_trackClick;

        SlotId m_slotId;
    };

    ///////////////////////
    // ExtenderSlotLayout
    ///////////////////////

    ExtenderSlotLayout::ExtenderSlotLayout(ExtenderSlotLayoutComponent& owner)
        : m_connectionType(ConnectionType::CT_Invalid)
        , m_owner(owner)
    {
        setInstantInvalidatePropagation(true);
        setOrientation(Qt::Horizontal);

        // Event Filter needs to be in the same scene. Ergo we will wait until the scene is set before trying
        // to install this event filter.
        m_slotLabelFilter = aznew ExtenderLabelEventFilter(owner.GetEntityId());

        m_slotConnectionPin = aznew ExtenderSlotConnectionPin(owner.GetEntityId());
        m_slotText = aznew GraphCanvasLabel();
        m_slotText->setAcceptHoverEvents(true);
        m_slotText->setAcceptedMouseButtons(Qt::MouseButton::LeftButton);
    }

    ExtenderSlotLayout::~ExtenderSlotLayout()
    {
        m_slotText->removeSceneEventFilter(m_slotLabelFilter);
        delete m_slotLabelFilter;
    }

    void ExtenderSlotLayout::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        SlotNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        StyleNotificationBus::Handler::BusConnect(m_owner.GetEntityId());
        m_slotConnectionPin->Activate();
    }

    void ExtenderSlotLayout::Deactivate()
    {
        m_slotConnectionPin->Deactivate();
        SceneMemberNotificationBus::Handler::BusDisconnect();
        SlotNotificationBus::Handler::BusDisconnect();
        StyleNotificationBus::Handler::BusDisconnect();
    }

    void ExtenderSlotLayout::OnSceneSet(const AZ::EntityId& graphId)
    {
        SlotRequestBus::EventResult(m_connectionType, m_owner.GetEntityId(), &SlotRequests::GetConnectionType);

        AZStd::string slotName;
        SlotRequestBus::EventResult(slotName, m_owner.GetEntityId(), &SlotRequests::GetName);

        m_slotText->SetLabel(slotName);

        AZStd::string toolTip;
        SlotRequestBus::EventResult(toolTip, m_owner.GetEntityId(), &SlotRequests::GetTooltip);

        OnTooltipChanged(toolTip);

        QGraphicsScene* graphicsScene = nullptr;
        SceneRequestBus::EventResult(graphicsScene, graphId, &SceneRequests::AsQGraphicsScene);

        if (graphicsScene)
        {
            graphicsScene->addItem(m_slotLabelFilter);
        }

        UpdateLayout();
        OnStyleChanged();
    }

    void ExtenderSlotLayout::OnSceneReady()
    {
        OnStyleChanged();
    }

    void ExtenderSlotLayout::OnRegisteredToNode(const AZ::EntityId& /*nodeId*/)
    {
        OnStyleChanged();
    }

    void ExtenderSlotLayout::OnNameChanged(const AZStd::string& name)
    {
        m_slotText->SetLabel(name);
    }

    void ExtenderSlotLayout::OnTooltipChanged(const AZStd::string& tooltip)
    {
        m_slotConnectionPin->setToolTip(tooltip.c_str());
        m_slotText->setToolTip(tooltip.c_str());
    }
    
    void ExtenderSlotLayout::OnStyleChanged()
    {
        m_style.SetStyle(m_owner.GetEntityId());

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

        m_slotConnectionPin->RefreshStyle();

        qreal padding = m_style.GetAttribute(Styling::Attribute::Padding, 2.);
        setContentsMargins(padding, padding, padding, padding);
        setSpacing(m_style.GetAttribute(Styling::Attribute::Spacing, 2.));

        UpdateGeometry();
    }

    void ExtenderSlotLayout::UpdateLayout()
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
            break;
        case ConnectionType::CT_Output:
            addItem(m_slotText);
            setAlignment(m_slotText, Qt::AlignRight);

            addItem(m_slotConnectionPin);
            setAlignment(m_slotConnectionPin, Qt::AlignRight);
            break;
        default:
            addItem(m_slotConnectionPin);
            addItem(m_slotText);
            break;
        }

        if (m_slotText)
        {
            m_slotText->installSceneEventFilter(m_slotLabelFilter);
        }
    }

    void ExtenderSlotLayout::UpdateGeometry()
    {
        m_slotConnectionPin->updateGeometry();
        m_slotText->update();

        invalidate();
        updateGeometry();
    }

    ////////////////////////////////
    // ExtenderSlotLayoutComponent
    ////////////////////////////////

    void ExtenderSlotLayoutComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<ExtenderSlotLayoutComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    ExtenderSlotLayoutComponent::ExtenderSlotLayoutComponent()
        : m_layout(nullptr)
    {
    }

    void ExtenderSlotLayoutComponent::Init()
    {
        SlotLayoutComponent::Init();

        m_layout = aznew ExtenderSlotLayout((*this));
        SetLayout(m_layout);
    }

    void ExtenderSlotLayoutComponent::Activate()
    {
        SlotLayoutComponent::Activate();
        m_layout->Activate();
    }

    void ExtenderSlotLayoutComponent::Deactivate()
    {
        SlotLayoutComponent::Deactivate();
        m_layout->Deactivate();
    }
}
