/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BreakpointPanel.hxx"
#include <Source/LUA/moc_BreakpointPanel.cpp>
#include <AzCore/Debug/Trace.h>

#include <QAction>

DHBreakpointsWidget::DHBreakpointsWidget(QWidget* parent)
    : QTableWidget(parent)
    , m_PauseUpdates(false)
{
    connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnDoubleClicked(const QModelIndex &)));
    LUABreakpointTrackerMessages::Handler::BusConnect();
    CreateContextMenu();
}
DHBreakpointsWidget::~DHBreakpointsWidget()
{
    LUABreakpointTrackerMessages::Handler::BusDisconnect();
    clearContents();
    setRowCount(0);
}

void DHBreakpointsWidget::PullFromContext()
{
    const LUAEditor::BreakpointMap* myData = NULL;
    EBUS_EVENT_RESULT(myData, LUAEditor::LUABreakpointRequestMessages::Bus, RequestBreakpoints);
    AZ_Assert(myData, "Nobody responded to the request breakpoints message.");
    BreakpointsUpdate(*myData);
}

void DHBreakpointsWidget::CreateContextMenu()
{
    actionDeleteAll = new QAction(tr("Delete All"), this);
    connect(actionDeleteAll, SIGNAL(triggered()), this, SLOT(DeleteAll()));
    actionDeleteSelected = new QAction(tr("Delete Selected"), this);
    connect(actionDeleteSelected, SIGNAL(triggered()), this, SLOT(DeleteSelected()));

    addAction(actionDeleteAll);
    addAction(actionDeleteSelected);
    setContextMenuPolicy(Qt::ActionsContextMenu);
}
void DHBreakpointsWidget::DeleteAll()
{
    while (rowCount())
    {
        RemoveRow(0);
    }
}
void DHBreakpointsWidget::DeleteSelected()
{
    m_PauseUpdates = true;

    QList<QTableWidgetItem*> list = selectedItems();

    for (int i = list.size() - 1; i >= 0; i -= 2) // magic number 2 is the column count, will be 3 if the empty first column ever gets contents
    {
        RemoveRow(list.at(i)->row());
    }

    m_PauseUpdates = false;

    PullFromContext();
}


//////////////////////////////////////////////////////////////////////////
//Debugger Messages, from the LUAEditor::LUABreakpointTrackerMessages::Bus
void DHBreakpointsWidget::BreakpointsUpdate(const LUAEditor::BreakpointMap& uniqueBreakpoints)
{
    if (!m_PauseUpdates)
    {
        // not using DeleteAll() here, this is an outside message so we only need to do internal housekeeping

        while (rowCount())
        {
            removeRow(0);
        }

        for (LUAEditor::BreakpointMap::const_iterator it = uniqueBreakpoints.begin(); it != uniqueBreakpoints.end(); ++it)
        {
            const LUAEditor::Breakpoint& bp = it->second;

            // sanity check to hopefully bypass corrupted entries
            // in a pure world this should never trigger
            //if ( (bp.m_documentLine >= 0) && (bp.m_blob.length() >= 5) ) // magic number 5
            {
                CreateBreakpoint(bp.m_assetName, bp.m_documentLine);
            }
            //else
            //{
            //  AZ_TracePrintf("BP", "Corrupted Breakpoint %s at line %d Was Stripped From Incoming Data\n", bp.m_blob, bp.m_documentLine);
            //}
        }
    }
}
void DHBreakpointsWidget::BreakpointHit(const LUAEditor::Breakpoint& bp)
{
    // clear any previous hit
    selectionModel()->clearSelection();

    // scroll to and highlight this one
    QList<QTableWidgetItem*> list = findItems(bp.m_assetName.c_str(), Qt::MatchExactly);
    QString q;
    q.setNum(bp.m_documentLine + 1); // +1 offset to match editor numbering

    for (int i = list.size() - 1; i >= 0; --i)
    {
        // magic number column #0 is the line number, 1 is the script file name
        QTableWidgetItem* line = item(list.at(i)->row(), 0);
        if (line->text() == q)
        {
            QModelIndex indexInModel = indexFromItem(line);
            selectionModel()->select(indexInModel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            setCurrentIndex(indexInModel);
            break;
        }
    }
}
void DHBreakpointsWidget::BreakpointResume()
{
    // no op
}


void DHBreakpointsWidget::CreateBreakpoint(const AZStd::string& debugName, int lineNumber)
{
    //AZ_TracePrintf("BP", "CreateBreakpoint %s at line %d\n", debugName.c_str(), lineNumber);

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
void DHBreakpointsWidget::RemoveBreakpoint(const AZStd::string& debugName, int lineNumber)
{
    //AZ_TracePrintf("BP", "RemoveBreakpoint %s at line %d\n", debugName.c_str(), lineNumber);

    QList<QTableWidgetItem*> list = findItems(debugName.c_str(), Qt::MatchExactly);
    QString q;
    q.setNum(lineNumber + 1); // +1 offset to match editor numbering

    for (int i = list.size() - 1; i >= 0; --i)
    {
        // magic number column #0 is the line number, 1 is the script file name
        QTableWidgetItem* line = item(list.at(i)->row(), 0);
        if (line->text() == q)
        {
            RemoveRow(list.at(i)->row());
            break;
        }
    }
}

// QT table view messages
void DHBreakpointsWidget::OnDoubleClicked(const QModelIndex& modelIdx)
{
    //AZ_TracePrintf("BP", "OnDoubleClicked() %d, %d\n", modelIdx.row(), modelIdx.column());

    // magic number column #0 is the line number, 1 is the script file name
    QTableWidgetItem* line = item(modelIdx.row(), 0);
    QTableWidgetItem* file = item(modelIdx.row(), 1);

    EBUS_EVENT(LUAEditor::LUABreakpointRequestMessages::Bus, RequestEditorFocus, AZStd::string(file->data(Qt::DisplayRole).toString().toUtf8().data()), line->data(Qt::DisplayRole).toInt());
}

void DHBreakpointsWidget::RemoveRow(int which)
{
    // magic number column #0 is the line number, 1 is the script file name
    QTableWidgetItem* line = item(which, 0);
    QTableWidgetItem* file = item(which, 1);

    QByteArray fileName = file->data(Qt::DisplayRole).toString().toUtf8().data();
    int lineNumber = line->data(Qt::DisplayRole).toInt();

    EBUS_EVENT(LUAEditor::LUABreakpointRequestMessages::Bus, RequestDeleteBreakpoint, AZStd::string(fileName), lineNumber);
}
