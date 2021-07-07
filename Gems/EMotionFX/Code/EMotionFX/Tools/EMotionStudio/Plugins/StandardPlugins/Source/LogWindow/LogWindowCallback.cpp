/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LogWindowCallback.h"
#include <QApplication>
#include <QHeaderView>
#include <QClipboard>
#include <QKeyEvent>
#include <QTime>
#include <QMenu>
#include <QScrollBar>


namespace EMStudio
{
    // constructor
    LogWindowCallback::LogWindowCallback(QWidget* parent)
        : QTableWidget(parent)
        , m_scrollToBottom(false)
    {
        qRegisterMetaType<MCore::LogCallback::ELogLevel>();

        // init the max second column width
        mMaxSecondColumnWidth = 0;

        // init the table
        setColumnCount(2);
        QStringList headerLabels;
        headerLabels.append("Time");
        headerLabels.append("Message");
        setHorizontalHeaderLabels(headerLabels);
        setColumnWidth(0, 70);
        setColumnWidth(1, 0);
        setAutoScroll(false);
        setShowGrid(false);
        verticalHeader()->setVisible(false);
        horizontalHeader()->setVisible(false);
        horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        setSelectionBehavior(QAbstractItemView::SelectRows);

        // set the filter
    #ifdef AZ_DEBUG_BUILD
        mFilter = LOGLEVEL_FATAL | LOGLEVEL_ERROR | LOGLEVEL_WARNING | LOGLEVEL_INFO | LOGLEVEL_DETAILEDINFO | LOGLEVEL_DEBUG;
    #else
        mFilter = LOGLEVEL_FATAL | LOGLEVEL_ERROR | LOGLEVEL_WARNING | LOGLEVEL_INFO;
    #endif

        connect(this, &LogWindowCallback::DoLog, this, &LogWindowCallback::LogImpl, Qt::QueuedConnection);
    }


    // destructor
    LogWindowCallback::~LogWindowCallback()
    {
    }


    // get the log window type ID
    uint32 LogWindowCallback::GetType() const
    {
        return LogWindowCallback::TYPE_ID;
    }


    // log the message
    void LogWindowCallback::Log(const char* text, ELogLevel logLevel)
    {
        emit DoLog(text, logLevel);
    }

    void LogWindowCallback::LogImpl(const QString text, ELogLevel logLevel)
    {
        // add the row in the table
        const QTime currentTime = QTime::currentTime();
        const QString currentDateTimeString = currentTime.toString("[hh:mm:ss]");
        QTableWidgetItem* timeItem = new QTableWidgetItem(currentDateTimeString);
        timeItem->setForeground(QColor(105, 105, 105));
        QTableWidgetItem* messageItem = new QTableWidgetItem(text);
        messageItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        messageItem->setData(Qt::UserRole, (int)logLevel);
        switch (logLevel)
        {
        // MCore::LogCallback::LOGLEVEL_INFO not needed because it uses the default color on this case
        case MCore::LogCallback::LOGLEVEL_FATAL:
            messageItem->setForeground(QColor("red"));
            break;
        case MCore::LogCallback::LOGLEVEL_ERROR:
            messageItem->setForeground(QColor("red"));
            break;
        case MCore::LogCallback::LOGLEVEL_WARNING:
            messageItem->setForeground(QColor("orange"));
            break;
        case MCore::LogCallback::LOGLEVEL_DETAILEDINFO:
            messageItem->setForeground(QColor("darkgray"));
            break;
        case MCore::LogCallback::LOGLEVEL_DEBUG:
            messageItem->setForeground(QColor("yellow"));
            break;
        default:
            ;
        }
        const int newRowIndex = rowCount();
        insertRow(newRowIndex);
        setRowHeight(newRowIndex, 21);
        setItem(newRowIndex, 0, timeItem);
        setItem(newRowIndex, 1, messageItem);

        // check the filter, if the filter is not enabled, it's not needed to test the find value
        if ((mFilter & (int)logLevel) != 0)
        {
            // check the find value, set the row not visible if the text is not found
            if (messageItem->text().contains(mFind, Qt::CaseInsensitive))
            {
                // set the row not hidden
                setRowHidden(newRowIndex, false);

                // custom resize of the column to be efficient
                const int itemWidth = itemDelegate()->sizeHint(viewOptions(), indexFromItem(messageItem)).width();
                mMaxSecondColumnWidth = qMax(mMaxSecondColumnWidth, itemWidth);
                SetColumnWidthToTakeWholeSpace();
            }
            else
            {
                setRowHidden(newRowIndex, true);
            }
        }
        else
        {
            setRowHidden(newRowIndex, true);
        }

        // scroll to bottom to see the last message
        m_scrollToBottom = true;
    }


    // set find
    void LogWindowCallback::SetFind(const QString& find)
    {
        // store the new find
        mFind = find;

        // init the max second column width
        mMaxSecondColumnWidth = 0;

        // test each row with the new find
        const int numRows = rowCount();
        for (int i = 0; i < numRows; ++i)
        {
            // get the item values
            QTableWidgetItem* messageItem = item(i, 1);
            const int logLevel = messageItem->data(Qt::UserRole).toInt();

            // check the filter, if the filter is not enabled, it's not needed to test the find value
            if ((mFilter & logLevel) != 0)
            {
                // check the find value, set the row not visible if the text is not found
                if (messageItem->text().contains(mFind, Qt::CaseInsensitive))
                {
                    // set the row not hidden
                    setRowHidden(i, false);

                    // update the new column width to keep the maximum
                    const int itemWidth = itemDelegate()->sizeHint(viewOptions(), indexFromItem(messageItem)).width();
                    mMaxSecondColumnWidth = qMax(mMaxSecondColumnWidth, itemWidth);
                }
                else
                {
                    setRowHidden(i, true);
                }
            }
            else
            {
                setRowHidden(i, true);
            }
        }

        // set the column to take the whole space
        SetColumnWidthToTakeWholeSpace();
    }


    // set filter
    void LogWindowCallback::SetFilter(uint32 filter)
    {
        // store the new filter
        mFilter = filter;

        // init the max second column width
        mMaxSecondColumnWidth = 0;

        // test each row with the new find
        const int numRows = rowCount();
        for (int i = 0; i < numRows; ++i)
        {
            // get the item values
            QTableWidgetItem* messageItem = item(i, 1);
            const int logLevel = messageItem->data(Qt::UserRole).toInt();

            // check the filter, if the filter is not enabled, it's not needed to test the find value
            if ((mFilter & logLevel) != 0)
            {
                // check the find value, set the row not visible if the text is not found
                if (messageItem->text().contains(mFind, Qt::CaseInsensitive))
                {
                    // set the row not hidden
                    setRowHidden(i, false);

                    // update the new column width to keep the maximum
                    const int itemWidth = itemDelegate()->sizeHint(viewOptions(), indexFromItem(messageItem)).width();
                    mMaxSecondColumnWidth = qMax(mMaxSecondColumnWidth, itemWidth);
                }
                else
                {
                    setRowHidden(i, true);
                }
            }
            else
            {
                setRowHidden(i, true);
            }
        }

        // set the column to take the whole space
        SetColumnWidthToTakeWholeSpace();
    }


    // key press event
    // it's needed because the copy action only copy one row not all selected
    void LogWindowCallback::keyPressEvent(QKeyEvent* event)
    {
        if (event->matches(QKeySequence::Copy))
        {
            Copy();
            event->accept();
        }
        else
        {
            QTableWidget::keyPressEvent(event);
        }
    }


    // resizeEvent
    // used to resize the column to take the whole space
    void LogWindowCallback::resizeEvent(QResizeEvent* event)
    {
        QTableWidget::resizeEvent(event);
        SetColumnWidthToTakeWholeSpace();
    }


    // set the column width to take the whole space
    void LogWindowCallback::SetColumnWidthToTakeWholeSpace()
    {
        const int firstColumnWidth = columnWidth(0);
        const int widthWihoutFirstColumnWidth = qMax(0, viewport()->width() - firstColumnWidth);
        if (mMaxSecondColumnWidth < widthWihoutFirstColumnWidth)
        {
            setColumnWidth(1, widthWihoutFirstColumnWidth);
        }
        else
        {
            setColumnWidth(1, mMaxSecondColumnWidth);
        }
    }


    // copy
    void LogWindowCallback::Copy()
    {
        // get the selected item
        const QList<QTableWidgetItem*> items = selectedItems();

        // get the number of selected items
        const uint32 numSelectedItems = items.count();

        // check if nothing needed to be copied
        if (numSelectedItems == 0)
        {
            return;
        }

        // filter the items
        MCore::Array<uint32> rowIndices;
        rowIndices.Reserve(numSelectedItems);
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            const uint32 rowIndex = items[i]->row();
            if (rowIndices.Find(rowIndex) == MCORE_INVALIDINDEX32)
            {
                rowIndices.Add(rowIndex);
            }
        }

        // sort the array to copy the item in order
        rowIndices.Sort();

        // get the number of selected rows
        const uint32 numSelectedRows = rowIndices.GetLength();

        // genereate the clipboard text
        QString clipboardText;
        const uint32 lastIndex = numSelectedRows - 1;
        for (uint32 i = 0; i < numSelectedRows; ++i)
        {
            const QString time = item(rowIndices[i], 0)->text();
            const QString message = item(rowIndices[i], 1)->text();
            clipboardText.append(time + ' ' + message);
            if (i < lastIndex)
            {
                clipboardText.append('\n');
            }
        }

        // set the clipboard text
        QApplication::clipboard()->setText(clipboardText);
    }


    // select all
    void LogWindowCallback::SelectAll()
    {
        selectAll();
    }


    // unselect all
    void LogWindowCallback::UnselectAll()
    {
        clearSelection();
    }


    // clear
    void LogWindowCallback::Clear()
    {
        setRowCount(0);
        setColumnWidth(1, 0);
        mMaxSecondColumnWidth = 0;
    }


    // context menu
    void LogWindowCallback::contextMenuEvent(QContextMenuEvent* event)
    {
        // get the selected item
        const QList<QTableWidgetItem*> items = selectedItems();
        const int numRows = rowCount();

        // create the menu
        QMenu menu(this);

        // add actions
        if (items.size() > 0)
        {
            QAction* copyAction = menu.addAction("Copy");
            connect(copyAction, &QAction::triggered, this, &LogWindowCallback::Copy);
        }
        if (numRows > 0)
        {
            QAction* selectAllAction = menu.addAction("Select All");
            connect(selectAllAction, &QAction::triggered, this, &LogWindowCallback::SelectAll);
        }
        if (items.size() > 0)
        {
            QAction* UnselectAllAction = menu.addAction("Unselect All");
            connect(UnselectAllAction, &QAction::triggered, this, &LogWindowCallback::UnselectAll);
        }
        if (numRows > 0)
        {
            QAction* clearAction = menu.addAction("Clear");
            connect(clearAction, &QAction::triggered, this, &LogWindowCallback::Clear);
        }

        // execute the menu
        if (menu.isEmpty() == false)
        {
            menu.exec(event->globalPos());
        }
    }

    void LogWindowCallback::paintEvent(QPaintEvent* event)
    {
        if (m_scrollToBottom)
        {
            scrollToBottom();
            m_scrollToBottom = false;
        }

        QTableWidget::paintEvent(event);
    }

} // namespace EMStudio
