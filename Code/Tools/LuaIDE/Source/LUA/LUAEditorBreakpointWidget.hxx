/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QtWidgets/QWidget>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/unordered_set.h>
#endif

#pragma once

namespace LUAEditor
{
    class LUAEditorPlainTextEdit;

    //also draws line numbers in addition to breakpoint indicators
    class LUAEditorBreakpointWidget : public QWidget
    {
        Q_OBJECT

        static const int c_borderSize = 3;

    public:
        AZ_CLASS_ALLOCATOR(LUAEditorBreakpointWidget, AZ::SystemAllocator);

        LUAEditorBreakpointWidget(QWidget *pParent = nullptr);
        virtual ~LUAEditorBreakpointWidget();

        //This must be called before textEdit parent widget is destroyed
        void PreDestruction();

        void SetTextEdit(LUAEditorPlainTextEdit* textEdit) { m_textEdit = textEdit; }
        
        //set to -1 to disable
        void SetCurrentlyExecutingLine(int lineNumber) { m_currentExecLine = lineNumber; }

        bool HasBreakpoint(int lineNumber) const;
        void AddBreakpoint(int lineNumber);
        void RemoveBreakpoint(int lineNumber);
        void ClearBreakpoints();

        void SetFont(QFont font);
    signals:

        void toggleBreakpoint(int lineNumber);
        void breakpointLineMove(int fromLineNumber, int toLineNumber);
        void breakpointDelete(int lineNumber);

    private:
        void paintEvent(QPaintEvent*) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void UpdateSize();

        LUAEditorPlainTextEdit* m_textEdit;
        AZStd::unordered_set<int> m_breakpoints;

        AZStd::vector<int> m_DeletedBreakpoints;
        int m_currentExecLine;
        QFont m_font{"OpenSans", 10};
        int m_numDigits{1};

    public slots:
        void OnBlockCountChange();
        void OnCharsRemoved(int position, int charsRemoved);
    };
}
