/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "DialogStack.h"
#include "MysticQtManager.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QApplication>


namespace MysticQt
{
    // dialog stack splitter
    class DialogStackSplitter
        : public QSplitter
    {
    public:

        DialogStackSplitter()
        {
            setStyleSheet("QSplitter::handle{ height: 4px; background: transparent; }");
        }

        void MoveFirstSplitterToMin()
        {
            moveSplitter(0, 1);
        }

        void MoveFirstSplitterToMax()
        {
            moveSplitter(INT_MAX, 1);
        }

        void MoveFirstSplitter(int pos)
        {
            moveSplitter(pos, 1);
        }
    };


    // the constructor
    DialogStack::Dialog::Dialog()
    {
        mButton         = nullptr;
        mFrame          = nullptr;
        mWidget         = nullptr;
        mDialogWidget   = nullptr;
        mSplitter       = nullptr;
        mClosable       = true;
    }


    // destructor
    DialogStack::Dialog::~Dialog()
    {
        delete mDialogWidget;
    }


    // the constructor
    DialogStack::DialogStack(QWidget* parent)
        : QScrollArea(parent)
    {
        // set the memory category of the dialog array
        mDialogs.SetMemoryCategory(MEMCATEGORY_MYSTICQT);

        // set the object name
        setObjectName("DialogStack");

        // create the root splitter
        mRootSplitter = new DialogStackSplitter();
        mRootSplitter->setOrientation(Qt::Vertical);
        mRootSplitter->setChildrenCollapsible(false);

        // set the widget resizable to have the scrollarea resizing it
        setWidgetResizable(true);

        // set the scrollarea widget
        setWidget(mRootSplitter);
    }


    // destructor
    DialogStack::~DialogStack()
    {
    }


    // get rid of all dialogs and their allocated memory
    void DialogStack::Clear()
    {
        // destroy the dialogs
        mDialogs.Clear();

        // update the scroll bars
        UpdateScrollBars();
    }


    // add an item to the stack
    void DialogStack::Add(QWidget* widget, const QString& headerTitle, bool closed, bool maximizeSize, bool closable, bool stretchWhenMaximize)
    {
        // create the dialog widget
        QWidget* dialogWidget = new QWidget();
        QVBoxLayout* dialogLayout = new QVBoxLayout();
        dialogLayout->setAlignment(Qt::AlignTop);
        dialogWidget->setLayout(dialogLayout);
        dialogLayout->setSpacing(0);
        dialogLayout->setMargin(0);

        // add the dialog widget
        // the splitter is hierarchical : {a, {b, c}}
        QSplitter* dialogSplitter;
        if (mDialogs.GetLength() == 0)
        {
            // add the dialog widget
            dialogSplitter = mRootSplitter;
            dialogSplitter->addWidget(dialogWidget);

            // stretch if needed
            if (maximizeSize && stretchWhenMaximize)
            {
                dialogSplitter->setStretchFactor(0, 1);
            }
        }
        else
        {
            // check if one space is free on the last splitter
            if (mDialogs.GetLast().mSplitter->count() == 1)
            {
                // add the dialog widget
                dialogSplitter = mDialogs.GetLast().mSplitter;
                dialogSplitter->addWidget(dialogWidget);

                // stretch if needed
                if (maximizeSize && stretchWhenMaximize)
                {
                    dialogSplitter->setStretchFactor(1, 1);
                }

                // less space used by the splitter when the last dialog is closed
                if (mDialogs.GetLast().mFrame->isHidden())
                {
                    mDialogs.GetLast().mSplitter->handle(1)->setFixedHeight(1);
                    mDialogs.GetLast().mSplitter->setStyleSheet("QSplitter::handle{ height: 1px; background: transparent; }");
                }

                // disable the splitter
                if (mDialogs.GetLast().mFrame->isHidden())
                {
                    mDialogs.GetLast().mSplitter->handle(1)->setDisabled(true);
                }
            }
            else // already two dialogs in the splitter
            {
                // create the new splitter
                dialogSplitter = new DialogStackSplitter();
                dialogSplitter->setOrientation(Qt::Vertical);
                dialogSplitter->setChildrenCollapsible(false);

                // add the current last dialog and the new dialog after
                dialogSplitter->addWidget(mDialogs.GetLast().mDialogWidget);
                dialogSplitter->addWidget(dialogWidget);

                // stretch if needed
                if (mDialogs.GetLast().mMaximizeSize && mDialogs.GetLast().mStretchWhenMaximize)
                {
                    dialogSplitter->setStretchFactor(0, 1);
                }

                // less space used by the splitter when the last dialog is closed
                if (mDialogs.GetLast().mFrame->isHidden())
                {
                    dialogSplitter->handle(1)->setFixedHeight(1);
                    dialogSplitter->setStyleSheet("QSplitter::handle{ height: 1px; background: transparent; }");
                }

                // disable the splitter
                if (mDialogs.GetLast().mFrame->isHidden())
                {
                    dialogSplitter->handle(1)->setDisabled(true);
                }

                // stretch if needed
                if (maximizeSize && stretchWhenMaximize)
                {
                    dialogSplitter->setStretchFactor(1, 1);
                }

                // replace the last dialog by the new splitter
                mDialogs.GetLast().mSplitter->addWidget(dialogSplitter);

                // disable the splitter
                const uint32 lastDialogIndex = mDialogs.GetLength() - 1;
                if (mDialogs[lastDialogIndex - 1].mFrame->isHidden())
                {
                    mDialogs.GetLast().mSplitter->handle(1)->setDisabled(true);
                }

                // stretch the splitter if needed
                // the correct behavior is found after experimentations
                if ((mDialogs.GetLast().mMaximizeSize && mDialogs.GetLast().mStretchWhenMaximize) || (mDialogs[lastDialogIndex - 1].mMaximizeSize && mDialogs[lastDialogIndex - 1].mStretchWhenMaximize == false))
                {
                    mDialogs.GetLast().mSplitter->setStretchFactor(1, 1);
                }

                // set the new splitter of the last dialog
                mDialogs.GetLast().mSplitter = dialogSplitter;
            }
        }

        // create the header button that we can click to open/close this dialog
        QPushButton* headerButton = new QPushButton(headerTitle);
        headerButton->setObjectName("HeaderButton");
        if (closed)
        {
            headerButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/ArrowRightGray.png"));
        }
        else
        {
            headerButton->setIcon(GetMysticQt()->FindIcon("Images/Icons/ArrowDownGray.png"));
        }

        // add the header button in the layout
        dialogLayout->addWidget(headerButton);
        connect(headerButton, &QPushButton::clicked, this, &MysticQt::DialogStack::OnHeaderButton);

        // create the frame where the dialog/widget will be inside
        QWidget* frame = new QWidget();
        frame->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        frame->setObjectName("StackFrame");
        dialogLayout->addWidget(frame);

        // create the layout thats inside of the frame
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(widget, Qt::AlignTop | Qt::AlignLeft);
        layout->setSpacing(0);
        layout->setMargin(3);

        // set the frame layout
        frame->setLayout(layout);

        // adjust size of the button
        headerButton->adjustSize();

        // adjust size of the widget
        widget->adjustSize();

        // set the constraints
        if (maximizeSize)
        {
            // set the layout size constraint
            layout->setSizeConstraint(QLayout::SetMaximumSize);
        }
        else
        {
            // set the layout size constraint
            layout->setSizeConstraint(QLayout::SetMinimumSize);

            // set the frame height
            frame->setFixedHeight(layout->minimumSize().height());

            // set the dialog height
            dialogWidget->setFixedHeight(dialogLayout->minimumSize().height());
        }

        // adjust size of the dialog widget
        dialogWidget->adjustSize();

        // register it, so that we know which frame is linked to which header button
        mDialogs.AddEmpty();
        mDialogs.GetLast().mButton              = headerButton;
        mDialogs.GetLast().mFrame               = frame;
        mDialogs.GetLast().mWidget              = widget;
        mDialogs.GetLast().mDialogWidget        = dialogWidget;
        mDialogs.GetLast().mSplitter            = dialogSplitter;
        mDialogs.GetLast().mClosable            = closable;
        mDialogs.GetLast().mMaximizeSize        = maximizeSize;
        mDialogs.GetLast().mStretchWhenMaximize = stretchWhenMaximize;
        mDialogs.GetLast().mLayout              = layout;
        mDialogs.GetLast().mDialogLayout        = dialogLayout;

        // check if the dialog is closed
        if (closed)
        {
            Close(headerButton);
        }

        // update the scroll bars
        UpdateScrollBars();
    }


    // add an item to the stack
    void DialogStack::Add(QLayout* layout, const QString& headerTitle, bool closed, bool maximizeSize, bool closable, bool stretchWhenMaximize)
    {
        // create the widget
        QWidget* widget = new QWidget();

        // set the layout on this widget
        widget->setLayout(layout);

        // add the dialog using the created widget
        Add(widget, headerTitle, closed, maximizeSize, closable, stretchWhenMaximize);
    }


    bool DialogStack::Remove(QWidget* widget)
    {
        const uint32 numDialogs = mDialogs.GetLength();
        for (uint32 i = 0; i < numDialogs; ++i)
        {
            QLayout*    layout  = mDialogs[i].mFrame->layout();
            int         index   = layout->indexOf(widget);

            // if the widget is located in the current layout, remove it
            // all next dialogs has to be moved to the previous splitter and delete if the last splitter is empty
            if (index != -1)
            {
                // remove the dialog
                // TODO : shift all dialogs needed as explained on the previous comment
                mDialogs[i].mDialogWidget->hide();
                mDialogs[i].mDialogWidget->deleteLater();
                mDialogs.Remove(i);

                // update the scroll bars
                UpdateScrollBars();

                // done
                return true;
            }
        }

        // not found
        return false;
    }


    // on header button press
    void DialogStack::OnHeaderButton()
    {
        QPushButton* button = (QPushButton*)sender();
        const uint32 dialogIndex = FindDialog(button);
        if (mDialogs[dialogIndex].mFrame->isHidden())
        {
            Open(button);
        }
        else
        {
            Close(button);
        }
    }


    // find the dialog that goes with the given button
    uint32 DialogStack::FindDialog(QPushButton* pushButton)
    {
        const uint32 numDialogs = mDialogs.GetLength();
        for (uint32 i = 0; i < numDialogs; ++i)
        {
            if (mDialogs[i].mButton == pushButton)
            {
                return i;
            }
        }
        return MCORE_INVALIDINDEX32;
    }


    // open the dialog
    void DialogStack::Open(QPushButton* button)
    {
        // find the dialog index
        const uint32 dialogIndex = FindDialog(button);
        if (dialogIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // show the widget inside the dialog
        mDialogs[dialogIndex].mFrame->show();

        // set the previous minimum and maximum height before closed
        mDialogs[dialogIndex].mDialogWidget->setMinimumHeight(mDialogs[dialogIndex].mMinimumHeightBeforeClose);
        mDialogs[dialogIndex].mDialogWidget->setMaximumHeight(mDialogs[dialogIndex].mMaximumHeightBeforeClose);

        // change the stylesheet and the icon
        button->setStyleSheet("");
        button->setIcon(GetMysticQt()->FindIcon("Images/Icons/ArrowDownGray.png"));

        // more space used by the splitter when the dialog is open
        if (dialogIndex < (mDialogs.GetLength() - 1))
        {
            mDialogs[dialogIndex].mSplitter->handle(1)->setFixedHeight(4);
            mDialogs[dialogIndex].mSplitter->setStyleSheet("QSplitter::handle{ height: 4px; background: transparent; }");
        }

        // enable the splitter
        if (dialogIndex < (mDialogs.GetLength() - 1))
        {
            mDialogs[dialogIndex].mSplitter->handle(1)->setEnabled(true);
        }

        // maximize the size if it's needed
        if (mDialogs.GetLength() > 1)
        {
            if (mDialogs[dialogIndex].mMaximizeSize)
            {
                // special case if it's the first dialog
                if (dialogIndex == 0)
                {
                    // if it's the first dialog and stretching is enabled, it expand to the max, all others expand to the min
                    if (mDialogs[dialogIndex].mStretchWhenMaximize == false && mDialogs[dialogIndex + 1].mMaximizeSize && mDialogs[dialogIndex + 1].mFrame->isHidden() == false)
                    {
                        static_cast<DialogStackSplitter*>(mDialogs[dialogIndex].mSplitter)->MoveFirstSplitterToMin();
                    }
                    else
                    {
                        static_cast<DialogStackSplitter*>(mDialogs[dialogIndex].mSplitter)->MoveFirstSplitterToMax();
                    }
                }
                else // not the first dialog
                {
                    // set the previous dialog to the min to have this dialog expanded to the top
                    if (mDialogs[dialogIndex - 1].mFrame->isHidden() || mDialogs[dialogIndex - 1].mMaximizeSize == false || (mDialogs[dialogIndex - 1].mMaximizeSize && mDialogs[dialogIndex - 1].mStretchWhenMaximize == false))
                    {
                        static_cast<DialogStackSplitter*>(mDialogs[dialogIndex - 1].mSplitter)->MoveFirstSplitterToMin();
                    }

                    // special case if it's not the last dialog
                    if (dialogIndex < (mDialogs.GetLength() - 1))
                    {
                        // if the next dialog is closed, it's needed to expand to the max too
                        if (mDialogs[dialogIndex + 1].mFrame->isHidden())
                        {
                            static_cast<DialogStackSplitter*>(mDialogs[dialogIndex].mSplitter)->MoveFirstSplitterToMax();
                        }
                    }
                }
            }
        }

        // update the scrollbars
        UpdateScrollBars();
    }


    // close the dialog
    void DialogStack::Close(QPushButton* button)
    {
        // find the dialog index
        const uint32 dialogIndex = FindDialog(button);
        if (dialogIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // only closable dialog can be closed
        if (mDialogs[dialogIndex].mClosable == false)
        {
            return;
        }

        // keep the min and max height before close
        mDialogs[dialogIndex].mMinimumHeightBeforeClose = mDialogs[dialogIndex].mDialogWidget->minimumHeight();
        mDialogs[dialogIndex].mMaximumHeightBeforeClose = mDialogs[dialogIndex].mDialogWidget->maximumHeight();

        // hide the widget inside the dialog
        mDialogs[dialogIndex].mFrame->hide();

        // set the widget to fixed size to not have it possible to resize
        mDialogs[dialogIndex].mDialogWidget->setMinimumHeight(mDialogs[dialogIndex].mButton->height());
        mDialogs[dialogIndex].mDialogWidget->setMaximumHeight(mDialogs[dialogIndex].mButton->height());

        // change the stylesheet and the icon
        button->setStyleSheet("border-bottom-left-radius: 4px; border-bottom-right-radius: 4px; border: 1px solid rgb(40,40,40);"); // TODO: link to the real style sheets
        button->setIcon(GetMysticQt()->FindIcon("Images/Icons/ArrowRightGray.png"));

        // less space used by the splitter when the dialog is closed
        if (dialogIndex < (mDialogs.GetLength() - 1))
        {
            mDialogs[dialogIndex].mSplitter->handle(1)->setFixedHeight(1);
            mDialogs[dialogIndex].mSplitter->setStyleSheet("QSplitter::handle{ height: 1px; background: transparent; }");
        }

        // disable the splitter
        if (dialogIndex < (mDialogs.GetLength() - 1))
        {
            mDialogs[dialogIndex].mSplitter->handle(1)->setDisabled(true);
        }

        // set the first splitter to the min if needed
        if (dialogIndex < (mDialogs.GetLength() - 1))
        {
            static_cast<DialogStackSplitter*>(mDialogs[dialogIndex].mSplitter)->MoveFirstSplitterToMin();
        }

        // maximize the first needed to avoid empty space
        bool findPreviousMaximizedDialogNeeded = true;
        const uint32 numDialogs = mDialogs.GetLength();
        for (uint32 i = dialogIndex + 1; i < numDialogs; ++i)
        {
            if (mDialogs[i].mMaximizeSize && mDialogs[i].mFrame->isHidden() == false)
            {
                if (i < (numDialogs - 1) && mDialogs[i + 1].mFrame->isHidden())
                {
                    static_cast<DialogStackSplitter*>(mDialogs[i].mSplitter)->MoveFirstSplitterToMax();
                }
                else
                {
                    static_cast<DialogStackSplitter*>(mDialogs[i - 1].mSplitter)->MoveFirstSplitterToMin();
                }
                findPreviousMaximizedDialogNeeded = false;
                break;
            }
        }
        if (findPreviousMaximizedDialogNeeded)
        {
            for (int32 i = dialogIndex - 1; i >= 0; --i)
            {
                if (mDialogs[i].mMaximizeSize && mDialogs[i].mFrame->isHidden() == false)
                {
                    static_cast<DialogStackSplitter*>(mDialogs[i].mSplitter)->MoveFirstSplitterToMax();
                    break;
                }
            }
        }

        // update the scrollbars
        UpdateScrollBars();
    }


    // when we press the mouse
    void DialogStack::mousePressEvent(QMouseEvent* event)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            // keep the mouse pos
            mPrevMouseX = event->globalX();
            mPrevMouseY = event->globalY();

            // set the cursor if the scrollbar is visible
            if ((horizontalScrollBar()->maximum() > 0) || (verticalScrollBar()->maximum() > 0))
            {
                QApplication::setOverrideCursor(Qt::ClosedHandCursor);
            }
        }
    }


    // when we double click
    // without this event handled the hand cursor is not set when double click
    void DialogStack::mouseDoubleClickEvent(QMouseEvent* event)
    {
        mousePressEvent(event);
    }


    // when the mouse button is released
    void DialogStack::mouseReleaseEvent(QMouseEvent* event)
    {
        // action only for the left button
        if (event->button() != Qt::LeftButton)
        {
            return;
        }

        // reset the cursor if the scrollbar is visible
        if ((horizontalScrollBar()->maximum() > 0) || (verticalScrollBar()->maximum() > 0))
        {
            QApplication::restoreOverrideCursor();
        }
    }


    // when a mouse button was clicked and we're moving the mouse
    void DialogStack::mouseMoveEvent(QMouseEvent* event)
    {
        // action only for the left button
        if ((event->buttons() & Qt::LeftButton) == false)
        {
            return;
        }

        // calculate the delta mouse movement
        const int32 deltaX = event->globalX() - mPrevMouseX;
        const int32 deltaY = event->globalY() - mPrevMouseY;

        // now apply this delta movement to the scroller
        int32 newX = horizontalScrollBar()->value() - deltaX;
        int32 newY = verticalScrollBar()->value() - deltaY;
        horizontalScrollBar()->setSliderPosition(newX);
        verticalScrollBar()->setSliderPosition(newY);

        // store the current value as previous value
        mPrevMouseX = event->globalX();
        mPrevMouseY = event->globalY();
    }


    // update the scroll bars ranges
    void DialogStack::UpdateScrollBars()
    {
        // compute the new range
        const QSize areaSize    = viewport()->size();
        const QSize widgetSize  = widget()->size();
        const int32 rangeX      = widgetSize.width()  - areaSize.width();
        const int32 rangeY      = widgetSize.height() - areaSize.height();

        // set the new ranges
        horizontalScrollBar()->setRange(0, rangeX);
        verticalScrollBar()->setRange(0, rangeY);
    }


    // when resizing
    void DialogStack::resizeEvent(QResizeEvent* event)
    {
        // update the scrollbar ranges
        UpdateScrollBars();

        // update the scroll area widget
        QScrollArea::resizeEvent(event);

        // maximize the first dialog needed
        const uint32 numDialogs = mDialogs.GetLength();
        const int32 lastDialogIndex = static_cast<int32>(numDialogs) - 1;
        for (int32 i = lastDialogIndex; i >= 0; --i)
        {
            if (mDialogs[i].mMaximizeSize && mDialogs[i].mFrame->isHidden() == false)
            {
                if (i < lastDialogIndex)
                {
                    static_cast<DialogStackSplitter*>(mDialogs[i].mSplitter)->MoveFirstSplitterToMax();
                }
                break;
            }
        }
    }


    // replace an internal widget
    void DialogStack::ReplaceWidget(QWidget* oldWidget, QWidget* newWidget)
    {
        for (uint32 i = 0; i < mDialogs.GetLength(); ++i)
        {
            // go next if the widget is not the same
            if (mDialogs[i].mWidget != oldWidget)
            {
                continue;
            }

            // replace the widget
            mDialogs[i].mFrame->layout()->replaceWidget(oldWidget, newWidget);
            mDialogs[i].mWidget = newWidget;

            // adjust size of the new widget
            newWidget->adjustSize();

            // set the constraints
            if (mDialogs[i].mMaximizeSize == false)
            {
                // get margins
                const QMargins frameMargins = mDialogs[i].mLayout->contentsMargins();
                const QMargins dialogMargins = mDialogs[i].mDialogLayout->contentsMargins();
                const int frameMarginTopBottom = frameMargins.top() + frameMargins.bottom();
                const int dialogMarginTopBottom = dialogMargins.top() + dialogMargins.bottom();
                const int allMarginsTopBottom = frameMarginTopBottom + dialogMarginTopBottom;

                // set the frame height
                mDialogs[i].mFrame->setFixedHeight(newWidget->height() + frameMarginTopBottom);

                // compute the dialog height
                const int dialogHeight = newWidget->height() + allMarginsTopBottom + mDialogs[i].mButton->height();

                // set the maximum height in case the dialog is not closed, if it's closed update the stored height
                if (mDialogs[i].mFrame->isHidden() == false)
                {
                    // set the dialog height
                    mDialogs[i].mDialogWidget->setFixedHeight(dialogHeight);

                    // set the first splitter to the min if needed
                    if (i < (mDialogs.GetLength() - 1))
                    {
                        static_cast<DialogStackSplitter*>(mDialogs[i].mSplitter)->MoveFirstSplitterToMin();
                    }
                }
                else // dialog closed
                {
                    // update the minimum and maximum stored height
                    mDialogs[i].mMinimumHeightBeforeClose = dialogHeight;
                    mDialogs[i].mMaximumHeightBeforeClose = dialogHeight;
                }
            }

            // stop here
            return;
        }
    }
}   // namespace MysticQt

#include <MysticQt/Source/moc_DialogStack.cpp>
