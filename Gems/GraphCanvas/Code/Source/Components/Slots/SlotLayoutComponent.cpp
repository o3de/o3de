/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "precompiled.h"

#include <QGraphicsLayout>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Components/Slots/SlotLayoutComponent.h>

namespace GraphCanvas
{
    ////////////////////////
    // SlotLayoutComponent
    ////////////////////////

    void SlotLayoutComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<SlotLayoutComponent, AZ::Component>()
                ->Version(1)
            ;
        }
    }

    SlotLayoutComponent::SlotLayoutComponent()
        : m_layoutWidget(nullptr)
        , m_layout(nullptr)
        
    {
    }
    
    SlotLayoutComponent::~SlotLayoutComponent()
    {
    }
    
    void SlotLayoutComponent::Init()
    {
        m_layoutWidget = new QGraphicsWidget();
        m_layoutWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_layoutWidget->setFlag(QGraphicsItem::ItemIsFocusable, true);
        m_layoutWidget->setVisible(m_isVisible);
    }
    
    void SlotLayoutComponent::Activate()
    {
        VisualRequestBus::Handler::BusConnect(GetEntityId());
    }
    
    void SlotLayoutComponent::Deactivate()
    {
        VisualRequestBus::Handler::BusDisconnect();
    }

    QGraphicsItem* SlotLayoutComponent::AsGraphicsItem()
    {
        return m_layoutWidget;
    }
    
    QGraphicsLayoutItem* SlotLayoutComponent::AsGraphicsLayoutItem()
    { 
        return m_layoutWidget;
    }

    bool SlotLayoutComponent::Contains(const AZ::Vector2& position) const
    {        
        QPointF localRectPos = m_layoutWidget->mapFromScene(QPointF(position.GetX(), position.GetY()));
        return m_layoutWidget->contains(localRectPos);
    }
    
    void SlotLayoutComponent::SetVisible(bool visible)
    {
        if (m_isVisible != visible)
        {
            m_isVisible = visible;

            if (m_layoutWidget)
            {
                m_layoutWidget->setVisible(visible);
            }
        }
    }
    
    bool SlotLayoutComponent::IsVisible() const
    {
        return m_isVisible;
    }
    
    void SlotLayoutComponent::SetLayout(QGraphicsLayout* layout)
    {
        AZ_Error("SlotLayoutComponent", m_layout == nullptr, "Trying to register two layouts to the same layout component");
        
        if (m_layout == nullptr)
        {
            m_layoutWidget->setLayout(layout);
            m_layout = layout;
        }
    }
    
    QGraphicsLayout* SlotLayoutComponent::GetLayout()
    {
        return m_layout;
    }
}
