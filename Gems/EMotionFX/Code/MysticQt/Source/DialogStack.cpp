/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "DialogStack.h"
#include "AzCore/std/iterator.h"
#include "AzCore/std/limits.h"
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
    DialogStack::DialogStack(QWidget* parent)
        : QScrollArea(parent)
    {
        // set the object name
        setObjectName("DialogStack");

        // create the root splitter
        m_rootSplitter = new DialogStackSplitter();
        m_rootSplitter->setOrientation(Qt::Vertical);
        m_rootSplitter->setChildrenCollapsible(false);

        // set the widget resizable to have the scrollarea resizing it
        setWidgetResizable(true);

        // set the scrollarea widget
        setWidget(m_rootSplitter);
    }

    DialogStack::~DialogStack()
    {
    }


    // get rid of all dialogs and their allocated memory
    void DialogStack::Clear()
    {
        for (Dialog& dialog : m_dialogs)
        {
            if (dialog.m_dialogWidget)
            {
                dialog.m_dialogWidget->deleteLater();
            }
        }

        // destroy the dialogs
        m_dialogs.clear();

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
        DialogStackSplitter* dialogSplitter;
        if (m_dialogs.empty())
        {
            // add the dialog widget
            dialogSplitter = m_rootSplitter;
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
            if (m_dialogs.back().m_splitter->count() == 1)
            {
                // add the dialog widget
                dialogSplitter = m_dialogs.back().m_splitter;
                dialogSplitter->addWidget(dialogWidget);

                // stretch if needed
                if (maximizeSize && stretchWhenMaximize)
                {
                    dialogSplitter->setStretchFactor(1, 1);
                }

                // less space used by the splitter when the last dialog is closed
                if (m_dialogs.back().m_frame->isHidden())
                {
                    m_dialogs.back().m_splitter->handle(1)->setFixedHeight(1);
                    m_dialogs.back().m_splitter->setStyleSheet("QSplitter::handle{ height: 1px; background: transparent; }");
                }

                // disable the splitter
                if (m_dialogs.back().m_frame->isHidden())
                {
                    m_dialogs.back().m_splitter->handle(1)->setDisabled(true);
                }
            }
            else // already two dialogs in the splitter
            {
                // create the new splitter
                dialogSplitter = new DialogStackSplitter();
                dialogSplitter->setOrientation(Qt::Vertical);
                dialogSplitter->setChildrenCollapsible(false);

                // add the current last dialog and the new dialog after
                dialogSplitter->addWidget(m_dialogs.back().m_dialogWidget);
                dialogSplitter->addWidget(dialogWidget);

                // stretch if needed
                if (m_dialogs.back().m_maximizeSize && m_dialogs.back().m_stretchWhenMaximize)
                {
                    dialogSplitter->setStretchFactor(0, 1);
                }

                // less space used by the splitter when the last dialog is closed
                if (m_dialogs.back().m_frame->isHidden())
                {
                    dialogSplitter->handle(1)->setFixedHeight(1);
                    dialogSplitter->setStyleSheet("QSplitter::handle{ height: 1px; background: transparent; }");
                }

                // disable the splitter
                if (m_dialogs.back().m_frame->isHidden())
                {
                    dialogSplitter->handle(1)->setDisabled(true);
                }

                // stretch if needed
                if (maximizeSize && stretchWhenMaximize)
                {
                    dialogSplitter->setStretchFactor(1, 1);
                }

                // replace the last dialog by the new splitter
                m_dialogs.back().m_splitter->addWidget(dialogSplitter);

                // disable the splitter
                if (m_dialogs.size() > 1)
                {
                    const auto previousDialogIt = m_dialogs.end() - 2;
                    if (previousDialogIt->m_frame->isHidden())
                    {
                        m_dialogs.back().m_splitter->handle(1)->setDisabled(true);
                    }

                    // stretch the splitter if needed
                    // the correct behavior is found after experimentations
                    if ((m_dialogs.back().m_maximizeSize && m_dialogs.back().m_stretchWhenMaximize) || (previousDialogIt->m_maximizeSize && previousDialogIt->m_stretchWhenMaximize == false))
                    {
                        m_dialogs.back().m_splitter->setStretchFactor(1, 1);
                    }
                }

                // set the new splitter of the last dialog
                m_dialogs.back().m_splitter = dialogSplitter;
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
        m_dialogs.emplace_back(Dialog{
            /*.m_button              =*/ headerButton,
            /*.m_frame               =*/ frame,
            /*.m_widget              =*/ widget,
            /*.m_dialogWidget        =*/ dialogWidget,
            /*.m_splitter            =*/ dialogSplitter,
            /*.m_closable            =*/ closable,
            /*.m_maximizeSize        =*/ maximizeSize,
            /*.m_stretchWhenMaximize =*/ stretchWhenMaximize,
            /*.m_minimumHeightBeforeClose =*/ 0,
            /*.m_maximumHeightBeforeClose =*/ 0,
            /*.m_layout              =*/ layout,
            /*.m_dialogLayout        =*/ dialogLayout,
        });

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
        const auto foundDialog = AZStd::find_if(begin(m_dialogs), end(m_dialogs), [widget](const Dialog& dialog)
        {
            return dialog.m_frame->layout()->indexOf(widget) != -1;
        });

        if (foundDialog == end(m_dialogs))
        {
            return false;
        }

        // if the widget is located in the current layout, remove it
        // all next dialogs has to be moved to the previous splitter and delete if the last splitter is empty
        // TODO : shift all dialogs needed as explained on the previous comment
        foundDialog->m_dialogWidget->hide();
        foundDialog->m_dialogWidget->deleteLater();
        m_dialogs.erase(foundDialog);

        // update the scroll bars
        UpdateScrollBars();

        return true;
    }


    // on header button press
    void DialogStack::OnHeaderButton()
    {
        QPushButton* button = (QPushButton*)sender();
        const size_t dialogIndex = FindDialog(button);
        if (m_dialogs[dialogIndex].m_frame->isHidden())
        {
            Open(button);
        }
        else
        {
            Close(button);
        }
    }


    // find the dialog that goes with the given button
    size_t DialogStack::FindDialog(QPushButton* pushButton)
    {
        const auto foundDialog = AZStd::find_if(begin(m_dialogs), end(m_dialogs), [pushButton](const Dialog& dialog)
        {
            return dialog.m_button == pushButton;
        });
        return foundDialog != end(m_dialogs) ? AZStd::distance(begin(m_dialogs), foundDialog) : MCore::InvalidIndex;
    }


    // open the dialog
    void DialogStack::Open(QPushButton* button)
    {
        const auto dialog = AZStd::find_if(begin(m_dialogs), end(m_dialogs), [button](const Dialog& dialog)
        {
            return dialog.m_button == button;
        });
        if (dialog == end(m_dialogs))
        {
            return;
        }

        // show the widget inside the dialog
        dialog->m_frame->show();

        // set the previous minimum and maximum height before closed
        dialog->m_dialogWidget->setMinimumHeight(dialog->m_minimumHeightBeforeClose);
        dialog->m_dialogWidget->setMaximumHeight(dialog->m_maximumHeightBeforeClose);

        // change the stylesheet and the icon
        button->setStyleSheet("");
        button->setIcon(GetMysticQt()->FindIcon("Images/Icons/ArrowDownGray.png"));

        // more space used by the splitter when the dialog is open
        if (dialog != m_dialogs.end() - 1)
        {
            dialog->m_splitter->handle(1)->setFixedHeight(4);
            dialog->m_splitter->setStyleSheet("QSplitter::handle{ height: 4px; background: transparent; }");
        }

        // enable the splitter
        if (dialog != m_dialogs.end() - 1)
        {
            dialog->m_splitter->handle(1)->setEnabled(true);
        }

        // maximize the size if it's needed
        if (m_dialogs.size() > 1)
        {
            if (dialog->m_maximizeSize)
            {
                // special case if it's the first dialog
                if (dialog == m_dialogs.begin())
                {
                    // if it's the first dialog and stretching is enabled, it expand to the max, all others expand to the min
                    if (dialog->m_stretchWhenMaximize == false && (dialog + 1)->m_maximizeSize && (dialog + 1)->m_frame->isHidden() == false)
                    {
                        static_cast<DialogStackSplitter*>(dialog->m_splitter)->MoveFirstSplitterToMin();
                    }
                    else
                    {
                        static_cast<DialogStackSplitter*>(dialog->m_splitter)->MoveFirstSplitterToMax();
                    }
                }
                else // not the first dialog
                {
                    // set the previous dialog to the min to have this dialog expanded to the top
                    if ((dialog - 1)->m_frame->isHidden() || (dialog - 1)->m_maximizeSize == false || ((dialog - 1)->m_maximizeSize && (dialog - 1)->m_stretchWhenMaximize == false))
                    {
                        static_cast<DialogStackSplitter*>((dialog - 1)->m_splitter)->MoveFirstSplitterToMin();
                    }

                    // special case if it's not the last dialog
                    if (dialog != m_dialogs.end() - 1)
                    {
                        // if the next dialog is closed, it's needed to expand to the max too
                        if ((dialog + 1)->m_frame->isHidden())
                        {
                            static_cast<DialogStackSplitter*>(dialog->m_splitter)->MoveFirstSplitterToMax();
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
        const auto dialog = AZStd::find_if(begin(m_dialogs), end(m_dialogs), [button](const Dialog& dialog)
        {
            return dialog.m_button == button;
        });
        if (dialog == end(m_dialogs))
        {
            return;
        }

        // only closable dialog can be closed
        if (dialog->m_closable == false)
        {
            return;
        }

        // keep the min and max height before close
        dialog->m_minimumHeightBeforeClose = dialog->m_dialogWidget->minimumHeight();
        dialog->m_maximumHeightBeforeClose = dialog->m_dialogWidget->maximumHeight();

        // hide the widget inside the dialog
        dialog->m_frame->hide();

        // set the widget to fixed size to not have it possible to resize
        dialog->m_dialogWidget->setMinimumHeight(dialog->m_button->height());
        dialog->m_dialogWidget->setMaximumHeight(dialog->m_button->height());

        // change the stylesheet and the icon
        button->setStyleSheet("border-bottom-left-radius: 4px; border-bottom-right-radius: 4px; border: 1px solid rgb(40,40,40);"); // TODO: link to the real style sheets
        button->setIcon(GetMysticQt()->FindIcon("Images/Icons/ArrowRightGray.png"));

        // less space used by the splitter when the dialog is closed
        if (dialog < m_dialogs.end() - 1)
        {
            dialog->m_splitter->handle(1)->setFixedHeight(1);
            dialog->m_splitter->setStyleSheet("QSplitter::handle{ height: 1px; background: transparent; }");
            dialog->m_splitter->handle(1)->setDisabled(true);
            static_cast<DialogStackSplitter*>(dialog->m_splitter)->MoveFirstSplitterToMin();
        }

        // maximize the first needed to avoid empty space
        bool findPreviousMaximizedDialogNeeded = true;
        for (auto curDialog = dialog + 1; curDialog != m_dialogs.end(); ++curDialog)
        {
            if (curDialog->m_maximizeSize && curDialog->m_frame->isHidden() == false)
            {
                if (curDialog != (m_dialogs.end() - 1) && (curDialog + 1)->m_frame->isHidden())
                {
                    static_cast<DialogStackSplitter*>(curDialog->m_splitter)->MoveFirstSplitterToMax();
                }
                else
                {
                    static_cast<DialogStackSplitter*>((curDialog - 1)->m_splitter)->MoveFirstSplitterToMin();
                }
                findPreviousMaximizedDialogNeeded = false;
                break;
            }
        }
        if (findPreviousMaximizedDialogNeeded)
        {
            for (auto curDialog = AZStd::make_reverse_iterator(dialog); curDialog != m_dialogs.rend(); ++curDialog)
            {
                if (curDialog->m_maximizeSize && curDialog->m_frame->isHidden() == false)
                {
                    static_cast<DialogStackSplitter*>(curDialog->m_splitter)->MoveFirstSplitterToMax();
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
            m_prevMouseX = event->globalX();
            m_prevMouseY = event->globalY();

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
        const int32 deltaX = event->globalX() - m_prevMouseX;
        const int32 deltaY = event->globalY() - m_prevMouseY;

        // now apply this delta movement to the scroller
        int32 newX = horizontalScrollBar()->value() - deltaX;
        int32 newY = verticalScrollBar()->value() - deltaY;
        horizontalScrollBar()->setSliderPosition(newX);
        verticalScrollBar()->setSliderPosition(newY);

        // store the current value as previous value
        m_prevMouseX = event->globalX();
        m_prevMouseY = event->globalY();
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
        if (m_dialogs.empty() || m_dialogs.size() == 1)
        {
            return;
        }
        for (auto dialog = m_dialogs.rbegin() + 1; dialog != m_dialogs.rend(); ++dialog)
        {
            if (dialog->m_maximizeSize && dialog->m_frame->isHidden() == false)
            {
                static_cast<DialogStackSplitter*>(dialog->m_splitter)->MoveFirstSplitterToMax();
                break;
            }
        }
    }


    // replace an internal widget
    void DialogStack::ReplaceWidget(QWidget* oldWidget, QWidget* newWidget)
    {
        for (auto dialog = m_dialogs.begin(); dialog != m_dialogs.end(); ++dialog)
        {
            // go next if the widget is not the same
            if (dialog->m_widget != oldWidget)
            {
                continue;
            }

            // replace the widget
            dialog->m_frame->layout()->replaceWidget(oldWidget, newWidget);
            dialog->m_widget = newWidget;

            // adjust size of the new widget
            newWidget->adjustSize();

            // set the constraints
            if (dialog->m_maximizeSize == false)
            {
                // get margins
                const QMargins frameMargins = dialog->m_layout->contentsMargins();
                const QMargins dialogMargins = dialog->m_dialogLayout->contentsMargins();
                const int frameMarginTopBottom = frameMargins.top() + frameMargins.bottom();
                const int dialogMarginTopBottom = dialogMargins.top() + dialogMargins.bottom();
                const int allMarginsTopBottom = frameMarginTopBottom + dialogMarginTopBottom;

                // set the frame height
                dialog->m_frame->setFixedHeight(newWidget->height() + frameMarginTopBottom);

                // compute the dialog height
                const int dialogHeight = newWidget->height() + allMarginsTopBottom + dialog->m_button->height();

                // set the maximum height in case the dialog is not closed, if it's closed update the stored height
                if (dialog->m_frame->isHidden() == false)
                {
                    // set the dialog height
                    dialog->m_dialogWidget->setFixedHeight(dialogHeight);

                    // set the first splitter to the min if needed
                    if (dialog != m_dialogs.end() - 1)
                    {
                        static_cast<DialogStackSplitter*>(dialog->m_splitter)->MoveFirstSplitterToMin();
                    }
                }
                else // dialog closed
                {
                    // update the minimum and maximum stored height
                    dialog->m_minimumHeightBeforeClose = dialogHeight;
                    dialog->m_maximumHeightBeforeClose = dialogHeight;
                }
            }

            // stop here
            return;
        }
    }
} // namespace MysticQt
