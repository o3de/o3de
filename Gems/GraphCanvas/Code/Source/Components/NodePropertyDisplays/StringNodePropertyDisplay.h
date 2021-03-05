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

#if !defined(Q_MOC_RUN)
#include <qlineedit.h>

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

    private:

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