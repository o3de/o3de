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

#include <QGraphicsWidget>
#include <QGraphicsLayoutItem>
#include <QTimer>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>

#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>

class QGraphicsLayoutItem;
class QGraphicsLinearLayout;

namespace GraphCanvas
{
    class NodePropertyDisplayWidget
        : public QGraphicsWidget
        , public RootGraphicsItemNotificationBus::Handler
        , public NodePropertiesRequestBus::Handler
        , public NodePropertyRequestBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(NodePropertyDisplayWidget, AZ::SystemAllocator, 0);

        NodePropertyDisplayWidget(QGraphicsItem* parent = nullptr);
        ~NodePropertyDisplayWidget() override;

        void RefreshStyle();

        // AZ::SystemTickBus
        void OnSystemTick() override;
        ////

        // RootGraphicsItemNotificationBus
        void OnDisplayStateChanged(RootGraphicsItemDisplayState oldState, RootGraphicsItemDisplayState newState);
        ////
        
        // NodePropertiesRequestBus
        void LockEditState(NodePropertyDisplay* propertyDisplay) override;
        void UnlockEditState(NodePropertyDisplay* propertyDisplay) override;
        
        void ForceLayoutState(NodePropertyLayoutState layoutState) override;
        ////
        
        // NodePropertyRequestBus
        void SetDisabled(bool disabled);
        
        void SetNodePropertyDisplay(NodePropertyDisplay* nodePropertyDisplay) override;
        NodePropertyDisplay* GetNodePropertyDisplay() const override;
        ////

        void ClearDisplay();
        
    private:
    
        void ClearLayout();
        void UpdateLayout();
        void UpdateGeometry();
    
        NodePropertyConfiguration   m_propertyConfiguration;
        NodePropertyDisplay*        m_nodePropertyDisplay;
        
        QGraphicsLayoutItem*   m_layoutItem;
        QGraphicsLinearLayout* m_layout;
        
        bool m_disabled;
        bool m_editing;
        NodePropertyLayoutState m_forcedLayout;
        
        QTimer                                      m_layoutTimer;
        AZStd::unordered_set<NodePropertyDisplay*>  m_forceEditSet;
    };
}
