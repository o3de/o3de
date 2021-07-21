/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>

#include <AzQtComponents/Buses/ShortcutDispatch.h>

AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>
#include <GraphCanvas/Components/Nodes/NodeUIBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Widgets/NodePropertyBus.h>
AZ_POP_DISABLE_WARNING

// qpainter.h(465): warning C4251: 'QPainter::d_ptr': class 'QScopedPointer<QPainterPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QPainter'
// qpainter.h(450) : warning C4800 : 'QFlags<QPainter::RenderHint>::Int' : forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800 4244, "-Wunknown-warning-option")
#include <QGraphicsScene>
#include <QGraphicsView>
AZ_POP_DISABLE_WARNING

class QGraphicsLayoutItem;

namespace GraphCanvas
{
    class DataInterface;
    class NodePropertyDisplay;
    class GraphCanvasLabel;

    // Base class for displaying a NodeProperty.
    //
    // Main idea is that in QGraphics, we want to use QWidgets
    // for a lot of our in-node editing, but this is slow with a large
    // number of instances.
    //
    // This provides an interface for allowing widgets to be swapped out depending
    // on state(thus letting us have a QWidget editable field, with a QGraphicsWidget display).
    class NodePropertyDisplay
        : public AzQtComponents::ShortcutDispatchTraits::Bus::Handler
        , public DataSlotNotificationBus::Handler
        , public DataSlotDragDropInterface
    {
    public:
        static AZStd::string CreateDisabledLabelStyle(const AZStd::string& type)
        {
            return AZStd::string::format("%sPropertyDisabledLabel", type.data());
        }

        static AZStd::string CreateDisplayLabelStyle(const AZStd::string& type)
        {
            return AZStd::string::format("%sPropertyDisplayLabel", type.data());
        }

        NodePropertyDisplay(DataInterface* dataInterface);
        virtual ~NodePropertyDisplay();
        
        AZ_DEPRECATED(void SetId(const AZ::EntityId& id), "Function deprecated. Use SetSlotId instead")
        {
            SetSlotId(id);
        }

        void SetSlotId(const SlotId& slotId);

        AZ_DEPRECATED(const AZ::EntityId& GetId() const, "Function deprecated. Use GetSlotId instead.") { return GetSlotId(); }
        const SlotId& GetSlotId() const { return m_slotId; }
        
        void SetNodeId(const AZ::EntityId& nodeId);        
        const AZ::EntityId& GetNodeId() const;

        AZ::EntityId GetSceneId() const;

        void TryAndSelectNode() const;

        bool EnableDropHandling() const;

        // DataSlotDragDropInterface
        AZ::Outcome<DragDropState> OnDragEnterEvent(QGraphicsSceneDragDropEvent* dragDropEvent) override;
        void OnDragLeaveEvent(QGraphicsSceneDragDropEvent* dragDropEvent) override;
        void OnDropEvent(QGraphicsSceneDragDropEvent* dragDropEvent) override;
        void OnDropCancelled() override;
        ////

        // DataSlotNotifications
        void OnDragDropStateStateChanged(const DragDropState& dragDropState) override { AZ_UNUSED(dragDropState); }
        ////

        void RegisterShortcutDispatcher(QWidget* widget);
        void UnregisterShortcutDispatcher(QWidget* widget);

        virtual void RefreshStyle() = 0;
        virtual void UpdateDisplay() = 0;

        // Display Widgets handles display the 'disabled' widget.
        virtual QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() = 0;

        // Display Widgets handles displaying the data in the non-editable base case.
        virtual QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() = 0;
        
        // Display Widgets handles displaying the data in an editable way
        virtual QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() = 0;

    protected:

        virtual void OnIdSet() {}
        virtual void OnSlotIdSet() { OnIdSet(); }

        // AzQtComponents::ShortcutDispatchBus::Handler
        QWidget* GetShortcutDispatchScopeRoot(QWidget*) override;
        ////

        void UpdateStyleForDragDrop(const DragDropState& dragDropState, Styling::StyleHelper& styleHelper);

    private:

        DataInterface* m_dataInterface;
        
        NodeId m_nodeId;
        SlotId m_slotId;
    };
}
