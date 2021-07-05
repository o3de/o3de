/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"
#include "LogControl.h"
#include "LoggingCommon.h"

#include <QAction>
#include <QIcon>
#include <QClipboard>
#include <QHeaderView>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QHBoxLayout>
AZ_POP_DISABLE_WARNING
#include <QScrollBar>
#include <QTableView>
#include <QHeaderView>

#include "LogPanel_Panel.h"

namespace AzToolsFramework
{
    namespace LogPanel
    {
        int BaseLogView::s_panelRefCount = 0; // non-atomic, its only called from the single GUI thread.
        QIcon* BaseLogView::s_criticalIcon = nullptr;
        QIcon* BaseLogView::s_warningIcon = nullptr;
        QIcon* BaseLogView::s_informationIcon = nullptr;
        QIcon* BaseLogView::s_debugIcon = nullptr;

        BaseLogView::BaseLogView(QWidget* pParent)
            : QWidget(pParent)
            , m_currentItemExpandsToFit(false)
        {
            ++s_panelRefCount;
            if (s_panelRefCount == 1) // we are first panel that exists
            {
                // load the icons.  note that Qt internally refcounts the icon data themselves, so we're really just getting a refcount to the original here.
                // we do this here because while we are using standardicon here, we might not always do so.
                // and if we load our custom icons, we certainly don't want to reload them in every log line.
                s_criticalIcon = new QIcon(QStringLiteral(":/stylesheet/img/logging/error.svg"));
                s_warningIcon = new QIcon(QStringLiteral(":/stylesheet/img/logging/warning.svg"));
                s_informationIcon = new QIcon(QStringLiteral(":/stylesheet/img/logging/information.svg"));
                s_debugIcon = new QIcon(QStringLiteral(":/stylesheet/img/logging/debug.svg"));
            }
            setLayout(new QHBoxLayout());
            layout()->setContentsMargins(0, 0, 0, 0);

            m_ptrLogView = new QTableView(this);
            m_ptrLogView->setContentsMargins(0, 0, 0, 0);
            m_ptrLogView->setSelectionBehavior(QAbstractItemView::SelectRows);

            layout()->addWidget(m_ptrLogView);
            CreateContextMenu();
        }


        BaseLogView::~BaseLogView()
        {
            --s_panelRefCount;
            if (s_panelRefCount == 0)
            {
                // last one out, free the icons.  Doing so drops our refcount on the icons in Qt, and if we're the last one out, they will unload.
                delete s_criticalIcon;
                delete s_warningIcon;
                delete s_informationIcon;
                delete s_debugIcon;
                s_criticalIcon = nullptr;
                s_warningIcon = nullptr;
                s_informationIcon = nullptr;
                s_debugIcon = nullptr;
            }
        }

        QIcon& BaseLogView::GetInformationIcon()
        {
            AZ_Assert(s_informationIcon, "Attempt to read an icon before any log views exist.");
            return *s_informationIcon;
        }

        QIcon& BaseLogView::GetWarningIcon()
        {
            AZ_Assert(s_warningIcon, "Attempt to read an icon before any log views exist.");
            return *s_warningIcon;
        }

        QIcon& BaseLogView::GetErrorIcon()
        {
            AZ_Assert(s_criticalIcon, "Attempt to read an icon before any log views exist.");
            return *s_criticalIcon;
        }

        QIcon& BaseLogView::GetDebugIcon()
        {
            AZ_Assert(s_debugIcon, "Attempt to read an icon before any log views exist.");
            return *s_debugIcon;
        }


        void BaseLogView::CreateContextMenu()
        {
            actionSelectAll = new QAction(tr("Select All"), this);
            connect(actionSelectAll, SIGNAL(triggered()), this, SLOT(SelectAll()));

            actionSelectNone = new QAction(tr("Select None"), this);
            connect(actionSelectNone, SIGNAL(triggered()), this, SLOT(SelectNone()));

            actionCopySelected = new QAction(tr("Copy"), this);
            actionCopySelected->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(actionCopySelected, SIGNAL(triggered()), this, SLOT(CopySelected()));

            actionCopyAll = new QAction(tr("Copy All"), this);
            connect(actionCopyAll, SIGNAL(triggered()), this, SLOT(CopyAll()));

            addAction(actionSelectAll);
            addAction(actionSelectNone);
            addAction(actionCopySelected);
            addAction(actionCopyAll);

            setContextMenuPolicy(Qt::ActionsContextMenu);
        }

        void BaseLogView::ConnectModelToView(QAbstractItemModel* ptrModel)
        {
            m_ptrLogView->setModel(ptrModel);
            connect(m_ptrLogView->model(), SIGNAL(rowsInserted(const QModelIndex&, int, int)), this, SLOT(rowsInserted(const QModelIndex&, int, int)));

            connect(m_ptrLogView->selectionModel(), &QItemSelectionModel::currentChanged, this, &BaseLogView::CurrentItemChanged);

            QHeaderView* pHeader = m_ptrLogView->horizontalHeader();

            int column = -1;
            if ((column = GetIconColumn()) >= 0)
            {
                pHeader->setSectionResizeMode(column, QHeaderView::Fixed); // status icon
                m_ptrLogView->setColumnWidth(column, 22);
            }

            if ((column = GetTimeColumn()) >= 0)
            {
                pHeader->setSectionResizeMode(column, QHeaderView::Fixed);
                m_ptrLogView->setColumnWidth(column, 132);
            }

            if ((column = GetWindowColumn()) >= 0)
            {
                pHeader->setSectionResizeMode(column, QHeaderView::Fixed);
                m_ptrLogView->setColumnWidth(column, 182);
            }

            if ((column = GetMessageColumn()) >= 0)
            {
                pHeader->setSectionResizeMode(column, QHeaderView::Stretch);
                m_ptrLogView->setColumnWidth(column, 132); // may as well give room for wide lines!
                m_ptrLogView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
            }
            pHeader->hide();

            m_ptrLogView->verticalHeader()->setDefaultSectionSize(25);
            m_ptrLogView->verticalHeader()->hide();
            m_ptrLogView->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
            // if you change this from FIXED it will be unbelievably slow becuase it will then call sizeHint() on every row and every column
            // instead of knowing that height = fixed_height * numRows.
            // do not change this unless the number of rows is going to be very, very small...

            auto* pDel = aznew LogPanel::LogPanelItemDelegate(m_ptrLogView, GetMessageColumn());
            m_ptrLogView->setItemDelegate(pDel);
            connect(pDel, SIGNAL(onLinkActivated(const QString&)), this, SIGNAL(onLinkActivated(const QString&)));
        }

        void BaseLogView::SetCurrentItemExpandsToFit(bool expandsToFit)
        {
            if (expandsToFit != m_currentItemExpandsToFit)
            {
                m_currentItemExpandsToFit = expandsToFit;
                CurrentItemChanged(m_ptrLogView->currentIndex(), QModelIndex());
            }
        }

        void BaseLogView::showEvent(QShowEvent *event)
        {
            Q_UNUSED(event)

            m_ptrLogView->resizeRowsToContents();
        }

        void BaseLogView::rowsInserted(const QModelIndex& /*parent*/, int start, int end)
        {
            // This is slow, only do it when we're actually showing logs
            if (isVisible())
            {
                for (int i = start; i <= end; ++i)
                {
                    m_ptrLogView->resizeRowToContents(i);
                }
            }
        }

        bool BaseLogView::IsAtMaxScroll() const
        {
            return (m_ptrLogView->verticalScrollBar()->value() == m_ptrLogView->verticalScrollBar()->maximum());
        }

        // Backing code to the context menu
        void BaseLogView::SelectAll()
        {
            m_ptrLogView->selectAll();
        }
        void BaseLogView::SelectNone()
        {
            m_ptrLogView->clearSelection();
        }

        // override this if you want to provide an implementation that decorates the text in some way.
        QString BaseLogView::ConvertRowToText(const QModelIndex& row)
        {
            QAbstractItemModel* pModel = m_ptrLogView->model();
            if (!pModel)
            {
                return QString();
            }

            int columnCount = pModel->columnCount();
            QString finalString = "";

            for (int column = 0; column < columnCount; ++column)
            {
                QString displayString = pModel->data(pModel->index(row.row(), column), Qt::DisplayRole).toString();
                if ((column != 0) && (finalString.length() > 0))
                {
                    finalString += QLatin1String(" - ");
                }
                finalString += displayString;
            }
            finalString += QLatin1String("\n");

            return finalString;
        }

        void BaseLogView::CopySelected()
        {
            QAbstractItemModel* pModel = ((QAbstractItemModel*)(m_ptrLogView->model()));
            if (!pModel)
            {
                return;
            }

            QString accumulator;
            QItemSelectionModel* selectionModel = m_ptrLogView->selectionModel();
            QModelIndexList indices = selectionModel->selectedRows();
            for (QModelIndexList::iterator iter = indices.begin(); iter != indices.end(); ++iter)
            {
                accumulator += this->ConvertRowToText(*iter);
            }

            if (accumulator.size())
            {
                QClipboard* clipboard = QApplication::clipboard();
                if (clipboard)
                {
                    clipboard->setText(accumulator);
                }
            }
        }
        void BaseLogView::CopyAll()
        {
            QAbstractItemModel* pModel = ((QAbstractItemModel*)(m_ptrLogView->model()));
            if (!pModel)
            {
                return;
            }

            QString finalString = "";

            int numRows = pModel->rowCount();
            for (int rowIdx = 0; rowIdx < numRows; ++rowIdx)
            {
                QModelIndex idx = pModel->index(rowIdx, 0);
                finalString += this->ConvertRowToText(idx);
            }

            QClipboard* clipboard = QApplication::clipboard();

            if (clipboard)
            {
                clipboard->setText(finalString);
            }
        }

        void BaseLogView::CurrentItemChanged(const QModelIndex& current, const QModelIndex& previous)
        {
            if (m_currentItemExpandsToFit)
            {
                const auto defaultSize = m_ptrLogView->verticalHeader()->sectionSizeHint(previous.row());
                m_ptrLogView->verticalHeader()->resizeSection(previous.row(), defaultSize);
                m_ptrLogView->resizeRowToContents(current.row());
            }
        }

    } // namespace LogPanel
}  // namespace AzToolsFramework

#include "UI/Logging/moc_LogControl.cpp"
