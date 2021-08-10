/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MYSTICQT_DIALOGSTACK_H
#define __MYSTICQT_DIALOGSTACK_H

//
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

    /**
     *
     *
     */
    class MYSTICQT_API DialogStack
        : public QScrollArea
    {
        Q_OBJECT

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
            QPushButton*    mButton = nullptr;
            QWidget*        mFrame = nullptr;
            QWidget*        mWidget = nullptr;
            QWidget*        mDialogWidget = nullptr;
            DialogStackSplitter* mSplitter = nullptr;
            bool            mClosable = true;
            bool            mMaximizeSize = false;
            bool            mStretchWhenMaximize = false;
            int             mMinimumHeightBeforeClose = 0;
            int             mMaximumHeightBeforeClose = 0;
            QLayout*        mLayout = nullptr;
            QLayout*        mDialogLayout = nullptr;
        };

    private:
        size_t FindDialog(QPushButton* pushButton);
        void Open(QPushButton* button);
        void Close(QPushButton* button);
        void UpdateScrollBars();

    private:
        DialogStackSplitter*    mRootSplitter;
        AZStd::vector<Dialog>   mDialogs;
        int32                   mPrevMouseX;
        int32                   mPrevMouseY;
    };
}   // namespace MysticQt

#endif
