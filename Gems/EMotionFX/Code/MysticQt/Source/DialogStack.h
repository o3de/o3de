/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <MCore/Source/Array.h>
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
    /**
     *
     *
     */
    class MYSTICQT_API DialogStack
        : public QScrollArea
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(DialogStack, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT);

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
            MCORE_MEMORYOBJECTCATEGORY(DialogStack::Dialog, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT);
            Dialog();
            ~Dialog();
            QPushButton*    mButton;
            QWidget*        mFrame;
            QWidget*        mWidget;
            QWidget*        mDialogWidget;
            QSplitter*      mSplitter;
            bool            mClosable;
            bool            mMaximizeSize;
            bool            mStretchWhenMaximize;
            int             mMinimumHeightBeforeClose;
            int             mMaximumHeightBeforeClose;
            QLayout*        mLayout;
            QLayout*        mDialogLayout;
        };

    private:
        uint32 FindDialog(QPushButton* pushButton);
        void Open(QPushButton* button);
        void Close(QPushButton* button);
        void UpdateScrollBars();

    private:
        QSplitter*              mRootSplitter;
        MCore::Array<Dialog>    mDialogs;
        int32                   mPrevMouseX;
        int32                   mPrevMouseY;
    };
}   // namespace MysticQt

#endif
