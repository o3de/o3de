/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ErrorReportDialog.h"

// Qt
#include <QClipboard>
#include <QDesktopServices>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QUrl>
#include <QDateTime>

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>

// Editor
#include "ErrorReportTableModel.h"
#include "Viewport.h"
#include "Util/Mailer.h"
#include "GameEngine.h"
#include "LyViewPaneNames.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_ErrorReportDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING



//////////////////////////////////////////////////////////////////////////
CErrorReportDialog* CErrorReportDialog::m_instance = nullptr;

// CErrorReportDialog dialog

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::RegisterViewClass()
{
    AzToolsFramework::ViewPaneOptions options;
    options.showInMenu = false;
    AzToolsFramework::RegisterViewPane<CErrorReportDialog>(LyViewPane::ErrorReport, LyViewPane::CategoryOther, options);
}

CErrorReportDialog::CErrorReportDialog(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CErrorReportDialog)
    , m_errorReportModel(new CErrorReportTableModel(this))
    , m_sortIndicatorColumn(-1)
    , m_sortIndicatorOrder(Qt::AscendingOrder)
{
    ui->setupUi(this);
    ui->treeView->setModel(m_errorReportModel);

    ui->treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView->header()->setSectionsMovable(true);
    ui->treeView->viewport()->setMouseTracking(true);
    ui->treeView->viewport()->installEventFilter(this);

    ui->treeView->header()->setSectionResizeMode(CErrorReportTableModel::ColumnSeverity, QHeaderView::ResizeToContents);
    ui->treeView->header()->resizeSection(CErrorReportTableModel::ColumnCount, 30);
    ui->treeView->header()->resizeSection(CErrorReportTableModel::ColumnText, 200);
    ui->treeView->header()->resizeSection(CErrorReportTableModel::ColumnFile, 150);
    ui->treeView->header()->resizeSection(CErrorReportTableModel::ColumnObject, 150);
    ui->treeView->header()->resizeSection(CErrorReportTableModel::ColumnModule, 100);
    ui->treeView->header()->resizeSection(CErrorReportTableModel::ColumnDescription, 100);
    ui->treeView->header()->resizeSection(CErrorReportTableModel::ColumnAssetScope, 200);

    connect(ui->treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(OnReportItemClick(QModelIndex)));
    connect(ui->treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(OnReportItemDblClick(QModelIndex)));
    connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnReportItemRClick()));
    connect(ui->treeView->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnReportColumnRClick()));
    connect(ui->treeView->header(), SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(OnSortIndicatorChanged(int,Qt::SortOrder)));

    ui->treeView->AddGroup(CErrorReportTableModel::ColumnModule);

    ui->treeView->header()->setSortIndicator(-1, Qt::AscendingOrder);

    m_instance = this;
    //CErrorReport *report,
    //m_pErrorReport = report;
    m_pErrorReport = nullptr;
}

CErrorReportDialog::~CErrorReportDialog()
{
    m_instance = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::Open(CErrorReport* pReport)
{
    if (!m_instance)
    {
        GetIEditor()->OpenView(LyViewPane::ErrorReport);
    }

    if (!m_instance)
    {
        return;
    }

    m_instance->SetReport(pReport);
    m_instance->UpdateErrors();
    m_instance->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::Close()
{
    if (m_instance)
    {
        /*
        CCryMemFile memFile( new BYTE[256], 256, 256 );
        CArchive ar( &memFile, CArchive::store );
        m_instance->m_wndReport.SerializeState( ar );
        ar.Close();

        UINT nSize = (UINT)memFile.GetLength();
        LPBYTE pbtData = memFile.Detach();
        CXTRegistryManager regManager;
        regManager.WriteProfileBinary( "Dialogs\\ErrorReport", "Configuration", pbtData, nSize);
        if ( pbtData )
            delete [] pbtData;
*/
        m_instance->close();
    }
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::Clear()
{
    if (m_instance)
    {
        m_instance->SetReport(nullptr);
        m_instance->UpdateErrors();
    }
}

bool CErrorReportDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseMove && watched == ui->treeView->viewport())
    {
        QMouseEvent* ev = static_cast<QMouseEvent*>(event);
        const QModelIndex index = ui->treeView->indexAt(ev->pos());
        QRect rect = ui->treeView->visualRect(index);
        rect.moveTopLeft(ui->treeView->viewport()->mapToGlobal(rect.topLeft()));
        const QRect target = fontMetrics().boundingRect(rect, index.data(Qt::TextAlignmentRole).toInt(), index.data().toString());

        if (index.column() == CErrorReportTableModel::ColumnObject && target.contains(QCursor::pos()))
        {
            ui->treeView->viewport()->setCursor(Qt::PointingHandCursor);
        }
        else
        {
            ui->treeView->viewport()->setCursor(Qt::ArrowCursor);
        }
    }
    return QWidget::eventFilter(watched, event);
}

// CErrorReportDialog message handlers

void CErrorReportDialog::SetReport(CErrorReport* report)
{
    m_pErrorReport = report;
    m_errorReportModel->setErrorReport(report);
}


void CErrorReportDialog::UpdateErrors()
{
    m_errorReportModel->setErrorReport(m_pErrorReport);
}

//////////////////////////////////////////////////////////////////////////
#define ID_REMOVE_ITEM  1
#define ID_SORT_ASC     2
#define ID_SORT_DESC        3
#define ID_GROUP_BYTHIS 4
#define ID_SHOW_GROUPBOX        5
#define ID_SHOW_FIELDCHOOSER 6
#define ID_COLUMN_BESTFIT       7
#define ID_COLUMN_ARRANGEBY 100
#define ID_COLUMN_ALIGMENT  200
#define ID_COLUMN_ALIGMENT_LEFT ID_COLUMN_ALIGMENT + 1
#define ID_COLUMN_ALIGMENT_RIGHT    ID_COLUMN_ALIGMENT + 2
#define ID_COLUMN_ALIGMENT_CENTER   ID_COLUMN_ALIGMENT + 3
#define ID_COLUMN_SHOW      500

void CErrorReportDialog::OnReportColumnRClick()
{
    QHeaderView* header = ui->treeView->header();

    int column = header->logicalIndexAt(ui->treeView->mapFromGlobal(QCursor::pos()));
    if (column < 0)
    {
        return;
    }

    QMenu menu;
    QAction* actionSortAscending = menu.addAction(tr("Sort &Ascending"));
    QAction* actionSortDescending = menu.addAction(tr("Sort Des&cending"));
    menu.addSeparator();
    QAction* actionGroupByThis = menu.addAction(tr("&Group by this field"));
    QAction* actionGroupByBox = menu.addAction(tr("Group &by box"));
    menu.addSeparator();
    QAction* actionRemoveItem = menu.addAction(tr("&Remove column"));
    QAction* actionFieldChooser = menu.addAction(tr("Field &Chooser"));
    menu.addSeparator();
    QAction* actionBestFit = menu.addAction(tr("Best &Fit"));

    actionGroupByBox->setCheckable(true);
    actionGroupByBox->setChecked(ui->treeView->IsGroupsShown());

    // create arrange by items
    QMenu menuArrange;
    const int nColumnCount = m_errorReportModel->columnCount();
    for (int nColumn = 0; nColumn < nColumnCount; nColumn++)
    {
        if (!header->isSectionHidden(nColumn))
        {
            const QString sCaption = m_errorReportModel->headerData(nColumn, Qt::Horizontal).toString();
            if (!sCaption.isEmpty())
            {
                menuArrange.addAction(sCaption)->setData(nColumn);
            }
        }
    }

    menuArrange.addSeparator();

    QAction* actionClearGroups = menuArrange.addAction(tr("Clear groups"));
    menuArrange.setTitle(tr("Arrange By"));
    menu.insertMenu(actionSortAscending, &menuArrange);

    // create columns items
    QMenu menuColumns;
    for (int nColumn = 0; nColumn < nColumnCount; nColumn++)
    {
        const QString sCaption = m_errorReportModel->headerData(nColumn, Qt::Horizontal).toString();
        //if (!sCaption.isEmpty())
        QAction* action = menuColumns.addAction(sCaption);
        action->setCheckable(true);
        action->setChecked(!ui->treeView->header()->isSectionHidden(nColumn));
    }

    menuColumns.setTitle(tr("Columns"));
    menu.insertMenu(menuArrange.menuAction(), &menuColumns);

    //create Text alignment submenu
    QMenu menuAlign;

    QAction* actionAlignLeft = menuAlign.addAction(tr("Align Left"));
    QAction* actionAlignRight = menuAlign.addAction(tr("Align Right"));
    QAction* actionAlignCenter = menuAlign.addAction(tr("Align Center"));
    actionAlignLeft->setCheckable(true);
    actionAlignRight->setCheckable(true);
    actionAlignCenter->setCheckable(true);

    const int alignment = m_errorReportModel->headerData(column, Qt::Horizontal, Qt::TextAlignmentRole).toInt();
    actionAlignLeft->setChecked(alignment & Qt::AlignLeft);
    actionAlignRight->setChecked(alignment & Qt::AlignRight);
    actionAlignCenter->setChecked(alignment & Qt::AlignHCenter);

    menuAlign.setTitle(tr("&Alignment"));
    menu.insertMenu(actionBestFit, &menuAlign);

    // track menu
    QAction* nMenuResult = menu.exec(QCursor::pos());

    // arrange by items
    if (menuArrange.actions().contains(nMenuResult))
    {
        // group by item
        if (actionClearGroups == nMenuResult)
        {
            ui->treeView->ClearGroups();
        }
        else
        {
            column = nMenuResult->data().toInt();
            ui->treeView->ToggleSortOrder(column);
        }
    }

    // process Alignment options
    if (nMenuResult == actionAlignLeft)
    {
        m_errorReportModel->setHeaderData(column, Qt::Horizontal, Qt::AlignLeft, Qt::TextAlignmentRole);
    }
    else if (nMenuResult == actionAlignRight)
    {
        m_errorReportModel->setHeaderData(column, Qt::Horizontal, Qt::AlignRight, Qt::TextAlignmentRole);
    }
    else if (nMenuResult == actionAlignCenter)
    {
        m_errorReportModel->setHeaderData(column, Qt::Horizontal, Qt::AlignCenter, Qt::TextAlignmentRole);
    }

    // process column selection item
    if (menuColumns.actions().contains(nMenuResult))
    {
        ui->treeView->header()->setSectionHidden(menuColumns.actions().indexOf(nMenuResult), !nMenuResult->isChecked());
    }


    // other general items
    if (nMenuResult == actionSortAscending || nMenuResult == actionSortDescending)
    {
        ui->treeView->sortByColumn(column, nMenuResult == actionSortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
    }
    else if (nMenuResult == actionFieldChooser)
    {
        //OnShowFieldChooser();
    }
    else if (nMenuResult == actionBestFit)
    {
        ui->treeView->resizeColumnToContents(column);
    }
    else if (nMenuResult == actionRemoveItem)
    {
        ui->treeView->header()->setSectionHidden(column, true);
    }
    // other general items
    else if (nMenuResult == actionGroupByThis)
    {
        ui->treeView->AddGroup(column);
        ui->treeView->ShowGroups(true);
    }
    else if (nMenuResult == actionGroupByBox)
    {
        ui->treeView->ShowGroups(!ui->treeView->IsGroupsShown());
    }
}

#define ID_POPUP_COLLAPSEALLGROUPS 1
#define ID_POPUP_EXPANDALLGROUPS 2


//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::CopyToClipboard()
{
    QString str;
    const QModelIndexList selRows = ui->treeView->selectionModel()->selectedRows();
    for (const QModelIndex& index : selRows)
    {
        const CErrorRecord* pRecord = index.data(Qt::UserRole).value<const CErrorRecord*>();
        if (pRecord)
        {
            str += pRecord->GetErrorText();
            str += QString::fromLatin1("\r\n");
        }
    }
    if (!str.isEmpty())
    {
        QApplication::clipboard()->setText(str);
    }
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::OnReportItemRClick()
{
    const QModelIndex index = ui->treeView->indexAt(ui->treeView->viewport()->mapFromGlobal(QCursor::pos()));

    if (!index.isValid())
    {
        return;
    }

    if (ui->treeView->model()->hasChildren(index))
    {
        QMenu menu;
        menu.addAction(tr("Collapse &All Groups"), ui->treeView, SLOT(collapseAll()));
        menu.addAction(tr("E&xpand All Groups"), ui->treeView, SLOT(expandAll()));

        // track menu
        menu.exec(QCursor::pos());
    }
    else
    {
        QMenu menu;
        //menu.addAction(tr("Select Object(s)")); // TODO: does nothing?
        menu.addAction(tr("Copy Warning(s) To Clipboard"), this, SLOT(CopyToClipboard()));
        menu.addAction(tr("E-mail Error Report"), this, SLOT(SendInMail()));
        menu.addAction(tr("Open in Excel"), this, SLOT(OpenInExcel()));

        // track menu
        menu.exec(QCursor::pos());
    }
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::SendInMail()
{
    if (!m_pErrorReport)
    {
        return;
    }

    // Send E-Mail.
    QString textMsg;
    for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
    {
        const CErrorRecord* err = m_errorReportModel->index(i, 0).data(Qt::UserRole).value<const CErrorRecord*>();
        textMsg += err->GetErrorText() + QString::fromLatin1("\n");
    }

    std::vector<const char*> who;
    const QString subject = QString::fromLatin1("Level %1 Error Report").arg(GetIEditor()->GetGameEngine()->GetLevelPath());
    CMailer::SendMail(subject.toUtf8().data(), textMsg.toUtf8().data(), who, who, true);
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::OpenInExcel()
{
    if (!m_pErrorReport)
    {
        return;
    }

    const QString levelName = Path::GetFileName(GetIEditor()->GetGameEngine()->GetLevelName());

    const QString filename = QString::fromLatin1("ErrorList_%1_%2.csv").arg(levelName).arg(QDateTime::currentDateTime().toString("yyyy-MM-dd-HH-mm-ss"));

    // Save to temp file.
    QFile f(filename);
    if (f.open(QIODevice::WriteOnly))
    {
        for (int i = 0; i < m_errorReportModel->rowCount(); ++i)
        {
            const CErrorRecord* err = m_errorReportModel->index(i, 0).data(Qt::UserRole).value<const CErrorRecord*>();
            QString text = err->GetErrorText();
            text.replace(',', '.');
            text.replace('\t', ',');
            f.write((text + QString::fromLatin1("\n")).toUtf8().data());
        }
        f.close();
        QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
    }
    else
    {
        Warning("Failed to save %s", (const char*)filename.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::OnReportItemClick(const QModelIndex& index)
{
    QRect rect = ui->treeView->visualRect(index);
    rect.moveTopLeft(ui->treeView->viewport()->mapToGlobal(rect.topLeft()));
    const QRect target = fontMetrics().boundingRect(rect, index.data(Qt::TextAlignmentRole).toInt(), index.data().toString());

    if (index.column() == CErrorReportTableModel::ColumnObject && target.contains(QCursor::pos()))
    {
        OnReportHyperlink(index);
    }

    /*
    if (pItemNotify->pColumn->GetItemIndex() == COLUMN_CHECK)
    {
        m_wndReport.Populate();
    }
    */
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy))
    {
        CopyToClipboard();
        event->accept();
    }
    else
    {
        QWidget::keyPressEvent(event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::OnReportItemDblClick(const QModelIndex& index)
{
    const CErrorRecord* pError = index.data(Qt::UserRole).value<const CErrorRecord*>();
    if (pError && GetIEditor()->GetActiveView())
    {
        float x, y, z;
        if (GetPositionFromString(pError->error, &x, &y, &z))
        {
            CViewport* vp = GetIEditor()->GetActiveView();
            Matrix34 tm = vp->GetViewTM();
            tm.SetTranslation(Vec3(x, y, z));
            vp->SetViewTM(tm);
        }
    }
}

void CErrorReportDialog::OnSortIndicatorChanged(int logicalIndex, Qt::SortOrder order)
{
    if (logicalIndex == 0)
    {
        ui->treeView->header()->setSortIndicator(m_sortIndicatorColumn, m_sortIndicatorOrder);
    }
    else
    {
        m_sortIndicatorColumn = logicalIndex;
        m_sortIndicatorOrder = order;
    }
}

//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::OnReportHyperlink(const QModelIndex& index)
{
    const CErrorRecord* pError = index.data(Qt::UserRole).value<const CErrorRecord*>();
    if (pError && GetIEditor()->GetActiveView())
    {
        float x, y, z;
        if (GetPositionFromString(pError->error, &x, &y, &z))
        {
            CViewport* vp = GetIEditor()->GetActiveView();
            Matrix34 tm = vp->GetViewTM();
            tm.SetTranslation(Vec3(x, y, z));
            vp->SetViewTM(tm);
        }
    }
}

/*
//////////////////////////////////////////////////////////////////////////
void CErrorReportDialog::OnShowFieldChooser()
{
    CMainFrm* pMainFrm = (CMainFrame*)AfxGetMainWnd();
    if (pMainFrm)
    {
        bool bShow = !pMainFrm->m_wndFieldChooser.IsVisible();
        pMainFrm->ShowControlBar(&pMainFrm->m_wndFieldChooser, bShow, false);
    }
}
*/

#include <moc_ErrorReportDialog.cpp>
