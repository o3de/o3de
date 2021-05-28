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
#include <QLineEdit>

#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <GraphCanvas/Components/NodePropertyDisplay/NodePropertyDisplay.h>
#include <GraphCanvas/Components/NodePropertyDisplay/NumericDataInterface.h>
#endif

class QGraphicsProxyWidget;

namespace GraphCanvas
{
    class GraphCanvasLabel;

    namespace Internal
    {
        // Need to know when the spin box gets focus in order to
        // manage the layout display when the mouse hovers off, but the
        // widget still has focus. Qt does not expose focus events in any
        // signal way, so this exposes that functionality for me.
        class FocusableDoubleSpinBox
            : public AzQtComponents::DoubleSpinBox
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(FocusableDoubleSpinBox, AZ::SystemAllocator, 0);
            FocusableDoubleSpinBox() = default;
            ~FocusableDoubleSpinBox() = default;

            void deselectAll()
            {
                lineEdit()->deselect();
                lineEdit()->setCursorPosition(0);
            }

        signals:
            void OnFocusIn();
            void OnFocusOut();

        protected:

            void focusInEvent(QFocusEvent* focusEvent) override
            {
                AzQtComponents::DoubleSpinBox::focusInEvent(focusEvent);
                emit OnFocusIn();
            }

            void focusOutEvent(QFocusEvent* focusEvent) override
            {
                AzQtComponents::DoubleSpinBox::focusOutEvent(focusEvent);
                emit OnFocusOut();
            }
        };
    }
    
    class NumericNodePropertyDisplay
        : public NodePropertyDisplay
    {    
    public:
        AZ_CLASS_ALLOCATOR(NumericNodePropertyDisplay, AZ::SystemAllocator, 0);
        NumericNodePropertyDisplay(NumericDataInterface* dataInterface);
        virtual ~NumericNodePropertyDisplay();
    
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

        void EditStart();
        void SubmitValue();
        void EditFinished();
        void SetupProxyWidget();
        void CleanupProxyWidget();
    
        NumericDataInterface*   m_dataInterface;
    
        GraphCanvasLabel*                   m_disabledLabel;
        GraphCanvasLabel*                   m_displayLabel;
        Internal::FocusableDoubleSpinBox*   m_spinBox;
        QGraphicsProxyWidget*               m_proxyWidget;
    };
}
