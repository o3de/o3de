/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/ReadOnlyDataInterface.h>

namespace GraphCanvas
{
    class GraphCanvasLabel;
    
    class ReadOnlyNodePropertyDisplay
        : public NodePropertyDisplay 
    {    
    public:
        AZ_CLASS_ALLOCATOR(ReadOnlyNodePropertyDisplay, AZ::SystemAllocator);
        
        ReadOnlyNodePropertyDisplay(ReadOnlyDataInterface* dataInterface);
        virtual ~ReadOnlyNodePropertyDisplay();
    
        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;
        
        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////

        // DataSlotNotifications
        void OnDragDropStateStateChanged(const DragDropState& dragState) override;
        ////
    
    private:
    
        ReadOnlyDataInterface*  m_dataInterface;
    
        GraphCanvasLabel*       m_disabledLabel;
        GraphCanvasLabel*       m_displayLabel;        
    };
}
