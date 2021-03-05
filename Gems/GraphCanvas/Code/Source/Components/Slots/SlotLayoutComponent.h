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
            provided.push_back(AZ_CRC("GraphCanvas_SlotVisualService", 0x30aa128a));
            provided.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
        }
        
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("GraphCanvas_SlotVisualService", 0x30aa128a));
            incompatible.push_back(AZ_CRC("GraphCanvas_RootVisualService", 0x9ec46d3b));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("GraphCanvas_SlotService", 0x701eaf6b));
        }

        void Init();
        void Activate();
        void Deactivate();
        ////
        
        // VisualRequestBus
        QGraphicsItem* AsGraphicsItem() override;
        QGraphicsLayoutItem* AsGraphicsLayoutItem() override;

        bool Contains(const AZ::Vector2& position) const;
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
