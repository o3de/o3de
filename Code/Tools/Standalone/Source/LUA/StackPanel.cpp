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

#include "StandaloneTools_precompiled.h"

#include "StackPanel.hxx"
#include <Source/LUA/moc_StackPanel.cpp>

DHStackWidget::DHStackWidget(QWidget* parent)
    : QTableWidget(parent)
{
    connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnDoubleClicked(const QModelIndex &)));
    LUAStackTrackerMessages::Handler::BusConnect();
}
DHStackWidget::~DHStackWidget()
{
    LUAStackTrackerMessages::Handler::BusDisconnect();
    clearContents();
    setRowCount(0);
}

void DHStackWidget::DeleteAll()
{
    clearContents();
    setRowCount(0);
}

//////////////////////////////////////////////////////////////////////////
//Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
void DHStackWidget::StackUpdate(const LUAEditor::StackList& stackList)
{
    DeleteAll();

    for (LUAEditor::StackList::const_iterator it = stackList.begin(); it != stackList.end(); ++it)
    {
        AppendStackEntry(it->m_blob, it->m_blobLine);
    }
}

void DHStackWidget::StackClear()
{
    DeleteAll();
}

void DHStackWidget::AppendStackEntry(const AZStd::string& debugName, int lineNumber)
{
    int newRow = rowCount();
    insertRow(newRow);

    // magic number column #0 is the line number, 1 is the script file name
    QTableWidgetItem* newItem = new QTableWidgetItem(debugName.c_str());
    newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(newRow, 1, newItem);
    newItem = new QTableWidgetItem(QString().setNum(lineNumber + 1));  // +1 offset to match editor numbering
    newItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    setItem(newRow, 0, newItem);
}

// QT table view messages
void DHStackWidget::OnDoubleClicked(const QModelIndex& modelIdx)
{
    // magic number column #0 is the line number, 1 is the script file name
    QTableWidgetItem* line = item(modelIdx.row(), 0);
    QTableWidgetItem* file = item(modelIdx.row(), 1);

    EBUS_EVENT(LUAEditor::LUAStackRequestMessages::Bus, RequestStackClicked, AZStd::string(file->data(Qt::DisplayRole).toString().toUtf8().data()), line->data(Qt::DisplayRole).toInt());
}
