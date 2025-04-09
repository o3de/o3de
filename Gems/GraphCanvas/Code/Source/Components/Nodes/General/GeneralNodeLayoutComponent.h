/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QGraphicsLinearLayout>

#include <AzCore/Component/Component.h>

#include <Components/Nodes/NodeLayoutComponent.h>
#include <GraphCanvas/Components/Nodes/NodeConfiguration.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Styling/StyleHelper.h>

class QGraphicsGridLayout;

namespace GraphCanvas
{
    //! Lays out the parts of the generic Node
    class GeneralNodeLayoutComponent
        : public NodeLayoutComponent
        , public NodeNotificationBus::Handler
        , public StyleNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(GeneralNodeLayoutComponent, "{2AD34925-FF0E-4D0D-A371-6338FBAE0F43}", NodeLayoutComponent);
        static void Reflect(AZ::ReflectContext*);
        
        static AZ::Entity* CreateGeneralNodeEntity(const char* nodeType, const NodeConfiguration& nodeConfiguration = NodeConfiguration());

        explicit GeneralNodeLayoutComponent();
        ~GeneralNodeLayoutComponent();

        // AZ::Component
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(NodeLayoutSupportServiceCrc);
            dependent.push_back(AZ_CRC_CE("GraphCanvas_TitleService"));
            dependent.push_back(AZ_CRC_CE("GraphCanvas_SlotsContainerService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("GraphCanvas_NodeService"));
            required.push_back(AZ_CRC_CE("GraphCanvas_StyledGraphicItemService"));
        }
        
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////    

        // StyleNotificationBus
        void OnStyleChanged() override;
        ////

        // NodeNotificationBus
        void OnNodeActivated() override;
        ////
        
    protected:

        void UpdateLayoutParameters();
        void UpdateHorizontalLayout();

        QGraphicsLinearLayout* m_title;
        QGraphicsLinearLayout* m_slots;

        QGraphicsLinearLayout* m_inputSlots;
        QGraphicsLinearLayout* m_outputSlots;
    };
}
