/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class QEvent;

#include <AzCore/Component/EntityBus.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyEntityIdCtrl.hxx>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/EntityIdDataInterface.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>

namespace GraphCanvas
{
    class GraphCanvasLabel;
    
    class EntityIdNodePropertyDisplay
        : public NodePropertyDisplay
        , public AZ::EntityBus::Handler
    {
        friend class EntityIdGraphicsEventFilter;

    public:
        AZ_CLASS_ALLOCATOR(EntityIdNodePropertyDisplay, AZ::SystemAllocator);
        EntityIdNodePropertyDisplay(EntityIdDataInterface* dataInterface);
        virtual ~EntityIdNodePropertyDisplay();
    
        // NodePropertyDisplay
        void RefreshStyle() override;
        void UpdateDisplay() override;
        
        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////

        // AZ::EntityBus
        void OnEntityNameChanged(const AZStd::string& name) override;
        ////

        // DataSlotNotifications
        void OnDragDropStateStateChanged(const DragDropState& dragState) override;
        ////

    protected:

        void ShowContextMenu(const QPoint&);
    
    private:

        void EditStart();
        void EditFinished();
        void SubmitValue();
        void SetupProxyWidget();
        void CleanupProxyWidget();
    
        EntityIdDataInterface*  m_dataInterface;
    
        GraphCanvasLabel*                           m_disabledLabel;
        AzToolsFramework::PropertyEntityIdCtrl*     m_propertyEntityIdCtrl;
        QGraphicsProxyWidget*                       m_proxyWidget;
        GraphCanvasLabel*                           m_displayLabel;
    };
}
