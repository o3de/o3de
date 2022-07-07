/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <qlineedit.h>

#include <AzCore/Component/TickBus.h>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>

#include <GraphCanvas/Components/NodePropertyDisplay/StringDataInterface.h>
#endif

class QGraphicsProxyWidget;

namespace GraphCanvas
{
    class GraphCanvasLabel;

    namespace Internal
    {
        // Need to know when the line edit gets focus in order to
        // manage the layout display when the mouse hovers off, but the
        // widget still has focus. Qt does not expose focus events in any
        // signal way, so this exposes that functionality for me.
        class FocusableLineEdit
            : public QLineEdit
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(FocusableLineEdit, AZ::SystemAllocator, 0);
            FocusableLineEdit() = default;
            ~FocusableLineEdit() = default;

        signals:
            void OnFocusIn();
            void OnFocusOut();

        private:

            void focusInEvent(QFocusEvent* focusEvent)
            {
                QLineEdit::focusInEvent(focusEvent);
                emit OnFocusIn();
            }

            void focusOutEvent(QFocusEvent* focusEvent)
            {
                QLineEdit::focusOutEvent(focusEvent);
                emit OnFocusOut();
            }
        };
    }
    
    class StringNodePropertyDisplay
        : public NodePropertyDisplay
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(StringNodePropertyDisplay, AZ::SystemAllocator, 0);
        StringNodePropertyDisplay(StringDataInterface* dataInterface);
        virtual ~StringNodePropertyDisplay();
    
        // DataSource
        void RefreshStyle() override;
        void UpdateDisplay() override;
        
        QGraphicsLayoutItem* GetDisabledGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetDisplayGraphicsLayoutItem() override;
        QGraphicsLayoutItem* GetEditableGraphicsLayoutItem() override;
        ////
    
        // DataSlotNotifications
        void OnDragDropStateStateChanged(const DragDropState& dragState) override;
        ////

        // AZ::SystemTickBus::Handler
        void OnSystemTick() override;
        ////

    private:

        void OnFocusOut();
        void EditStart();
        void SubmitValue();
        void EditFinished();
        void SetupProxyWidget();
        void CleanupProxyWidget();

        void ResizeToContents();
    
        StringDataInterface*   m_dataInterface;
    
        GraphCanvasLabel*               m_disabledLabel;
        GraphCanvasLabel*               m_displayLabel;
        Internal::FocusableLineEdit*    m_lineEdit;
        QGraphicsProxyWidget*           m_proxyWidget;

        bool m_valueDirty;
        bool m_isNudging;
    };
}
