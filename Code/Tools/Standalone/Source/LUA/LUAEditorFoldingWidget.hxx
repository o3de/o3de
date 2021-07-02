/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_FOLDINGWIDGET_H
#define LUAEDITOR_FOLDINGWIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QtWidgets/QWidget>
#include "AzToolsFramework/UI/UICore/PlainTextEdit.hxx"
#endif

#pragma once

namespace LUAEditor
{
    class LUAEditorPlainTextEdit;

    class FoldingWidget : public QWidget
    {
        Q_OBJECT

        static const int c_borderSize{3};

    public:
        AZ_CLASS_ALLOCATOR(FoldingWidget, AZ::SystemAllocator, 0);

        FoldingWidget(QWidget *pParent = nullptr);
        virtual ~FoldingWidget();

        void SetTextEdit(AzToolsFramework::PlainTextEdit* textEdit) { m_textEdit = textEdit; }
        void OnContentChanged(int from, int charsRemoved, int charsAdded);

        void FoldAll();
        void UnfoldAll();

        void SetFont(QFont font);

    signals:
        void TextBlockFoldingChanged();

    private:
        void paintEvent(QPaintEvent*) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        AzToolsFramework::PlainTextEdit* m_textEdit;
        int m_singleSize{10}; //square size for folding widget, of a single line in editor
    };
}

#endif
