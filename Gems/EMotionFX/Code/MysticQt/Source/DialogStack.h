/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "MysticQtConfig.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QScrollArea>
#include <AzCore/std/containers/vector.h>
#endif

// forward declarations
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QFrame)
QT_FORWARD_DECLARE_CLASS(QLayout)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QSplitter)


namespace MysticQt
{
    class DialogStackSplitter;

    class MYSTICQT_API DialogStack
        : public QScrollArea
    {
        Q_OBJECT // AUTOMOC

    public:
        DialogStack(QWidget* parent = nullptr);
        ~DialogStack();

        void Add(QWidget* widget, const QString& headerTitle, bool closed = false, bool maximizeSize = false, bool closable = true, bool stretchWhenMaximize = true);
        void Add(QLayout* layout, const QString& headerTitle, bool closed = false, bool maximizeSize = false, bool closable = true, bool stretchWhenMaximize = true);
        bool Remove(QWidget* widget);
        void ReplaceWidget(QWidget* oldWidget, QWidget* newWidget);
        void Clear();

    protected:
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    protected slots:
        void OnHeaderButton();

    private:
        struct Dialog
        {
            QPushButton*    m_button = nullptr;
            QWidget*        m_frame = nullptr;
            QWidget*        m_widget = nullptr;
            QWidget*        m_dialogWidget = nullptr;
            DialogStackSplitter* m_splitter = nullptr;
            bool            m_closable = true;
            bool            m_maximizeSize = false;
            bool            m_stretchWhenMaximize = false;
            int             m_minimumHeightBeforeClose = 0;
            int             m_maximumHeightBeforeClose = 0;
            QLayout*        m_layout = nullptr;
            QLayout*        m_dialogLayout = nullptr;
        };

    private:
        size_t FindDialog(QPushButton* pushButton);
        void Open(QPushButton* button);
        void Close(QPushButton* button);
        void UpdateScrollBars();

    private:
        DialogStackSplitter*    m_rootSplitter;
        AZStd::vector<Dialog>   m_dialogs;
        int32                   m_prevMouseX;
        int32                   m_prevMouseY;
    };
}   // namespace MysticQt
