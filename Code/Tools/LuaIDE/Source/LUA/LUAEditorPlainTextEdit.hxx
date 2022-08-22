/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QtWidgets/QCompleter>
#include <QTextEdit>
#include <AzCore/std/functional.h>
#include <AzCore/Memory/SystemAllocator.h>
#endif

#pragma once

namespace LUAEditor
{
    class LUAEditorPlainTextEdit : public QTextEdit
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(LUAEditorPlainTextEdit, AZ::SystemAllocator, 0);

        LUAEditorPlainTextEdit(QWidget *pParent = nullptr);
        virtual ~LUAEditorPlainTextEdit();

        void SetTabSize(int tabSize) { m_tabSize = tabSize; }
        void SetUseSpaces(bool useSpaces) { m_useSpaces = useSpaces; }
        void UpdateFont(QFont font, int tabSize);
        void SetGetLuaName(AZStd::function<QString(const QTextCursor&)> lambda) { m_getLUAName = lambda; }
        
        void ForEachVisibleBlock(AZStd::function<void(QTextBlock& block, const QRectF&)> operation) const;

    protected:
        void focusInEvent(QFocusEvent* pEvent) override;
        void focusOutEvent(QFocusEvent* pEvent) override;
        void paintEvent(QPaintEvent*) override;
        void keyPressEvent(QKeyEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void dropEvent(QDropEvent *e) override;

    private:
        class Completer* m_completer;
        class CompletionModel* m_completionModel;
        AZStd::function<QString(const QTextCursor&)> m_getLUAName;
        int m_tabSize = 4;
        bool m_useSpaces = false;

        bool HandleIndentKeyPress(QKeyEvent* event);
        bool HandleHomeKeyPress(QKeyEvent* event);
        bool HandleNewline(QKeyEvent* event);
        void mouseDoubleClickEvent(QMouseEvent* event) override;

    signals:
        void FocusChanged(bool focused);
        void ZoomIn();
        void ZoomOut();
        void Scrolled();
        //accept the event to avoid default double click behavior
        void BlockDoubleClicked(QMouseEvent* event, const QTextBlock& block);

    public slots:
        void OnScopeNamesUpdated(const QStringList& scopeNames);

    private slots:
        void CompletionSelected(const QString& text);
    };
}
