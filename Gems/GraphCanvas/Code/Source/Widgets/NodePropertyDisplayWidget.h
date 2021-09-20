/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        void OnDisplayStateChanged(RootGraphicsItemDisplayState oldState, RootGraphicsItemDisplayState newState) override;
        ////
        
        // NodePropertiesRequestBus
        void LockEditState(NodePropertyDisplay* propertyDisplay) override;
        void UnlockEditState(NodePropertyDisplay* propertyDisplay) override;
        
        void ForceLayoutState(NodePropertyLayoutState layoutState) override;
        ////
        
        // NodePropertyRequestBus
        void SetDisabled(bool disabled) override;
        
        void SetNodePropertyDisplay(NodePropertyDisplay* nodePropertyDisplay) override;
        NodePropertyDisplay* GetNodePropertyDisplay() const override;
        ////

        void ClearDisplay();
        
    private:
    
        void ClearLayout();
        void UpdateLayout(bool forceUpdate = false);
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
