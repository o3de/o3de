/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <qgraphicswidget.h>

#include <AzCore/Component/Component.h>

#include <GraphCanvas/Components/VisualBus.h>

class QGraphicsLayout;

namespace GraphCanvas
{
    class SlotLayoutComponent
        : public AZ::Component
        , public VisualRequestBus::Handler
    {
    public:
    
        AZ_COMPONENT(SlotLayoutComponent , "{77518A10-3443-4668-ADCB-D6EFC3BF9907}");
        static void Reflect(AZ::ReflectContext* context);
    
        SlotLayoutComponent();
        virtual ~SlotLayoutComponent();
    
        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("GraphCanvas_SlotVisualService"));
            provided.push_back(AZ_CRC_CE("GraphCanvas_RootVisualService"));
        }
        
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_SlotVisualService"));
            incompatible.push_back(AZ_CRC_CE("GraphCanvas_RootVisualService"));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_SlotService"));
        }

        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////
        
        // VisualRequestBus
        QGraphicsItem* AsGraphicsItem() override;
        QGraphicsLayoutItem* AsGraphicsLayoutItem() override;

        bool Contains(const AZ::Vector2& position) const override;
        void SetVisible(bool visible) override;
        bool IsVisible() const override;
        ////
    
    protected:
    
        void SetLayout(QGraphicsLayout* layout);
        QGraphicsLayout* GetLayout();
        
    private:
        QGraphicsWidget* m_layoutWidget;
        QGraphicsLayout* m_layout;

        bool m_isVisible = true;
    };
}
