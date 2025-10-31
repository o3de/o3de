/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/BooleanDataInterface.h>
#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>

#include <Widgets/GraphCanvasCheckBox.h>

namespace GraphCanvas
{
    class GraphCanvasLabel;

    class BooleanNodePropertyDisplay
        : public NodePropertyDisplay 
        , public GraphCanvasCheckBoxNotificationBus::Handler
    {    
    public:
        AZ_CLASS_ALLOCATOR(BooleanNodePropertyDisplay, AZ::SystemAllocator);
        BooleanNodePropertyDisplay(BooleanDataInterface* dataInterface);
        virtual ~BooleanNodePropertyDisplay();
    
        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;
        
        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////

        // GraphCanvsCheckBoxNotifications
        void OnValueChanged(bool value) override;
        void OnClicked() override;
        ////
    
    private:

        void InvertValue();
    
        BooleanDataInterface*   m_dataInterface;

        GraphCanvasCheckBox*    m_checkBox;    
        GraphCanvasLabel*       m_disabledLabel;
    };
}
