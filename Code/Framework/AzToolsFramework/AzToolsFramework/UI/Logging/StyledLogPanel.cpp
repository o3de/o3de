/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StyledLogPanel.h"
#include "NewLogTabDialog.h"

#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <QAction>
#include <QClipboard>
#include <QScrollBar>
#include <QItemSelectionModel>
#include <QMenu>
#include <QCursor>
#include <QApplication>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        StyledLogPanel::StyledLogPanel(QWidget* parent)
            : AzQtComponents::TabWidget(parent)
        {
            setActionToolBarVisible(true);
            setTabsClosable(true);
            setMovable(true);

            QAction* pCopyAllAction = new QAction(QIcon(QStringLiteral(":/stylesheet/img/logging/copy.svg")), tr("Copy all"), this);
            addAction(pCopyAllAction);

            QAction* pResetAction = new QAction(QIcon(QStringLiteral(":/stylesheet/img/logging/reset.svg")), tr("Reset"), this);
            addAction(pResetAction);

            QAction* pAddFilterAction = new QAction(QIcon(QStringLiteral(":/stylesheet/img/logging/add-filter.svg")), tr("Add..."), this);
            addAction(pAddFilterAction);

            connect(this, &QTabWidget::tabCloseRequested, this, &StyledLogPanel::onTabCloseButtonPressed);
            connect(pAddFilterAction, &QAction::triggered, this, &StyledLogPanel::onAddTriggered);
            connect(pResetAction, &QAction::triggered, this, &StyledLogPanel::onResetTriggered);
            connect(pCopyAllAction, &QAction::triggered, this, &StyledLogPanel::onCopyAllTriggered);

            m_noTabsWidget = new QWidget(this);
            m_noTabsWidget->setVisible(false);
        }

        StyledLogPanel::~StyledLogPanel()
        {
            // disconnect this, otherwise, we'll get a destroyed signal after the inherited QObject destructor runs
            // and deletes it's children, including the tabs, which will try to reference
            // data from this object, which has already been destructed
            int tabCount = count();
            for (int i = 0; i < tabCount; i++)
            {
                QWidget* tab = widget(i);
                if (tab != m_noTabsWidget)
                {
                    disconnect(tab, &QObject::destroyed, this, &StyledLogPanel::tabDestroyed);
                }
            }
        }

        void StyledLogPanel::AddLogTab(const TabSettings& settings)
        {
            auto newTab = qobject_cast<StyledLogTab*>(CreateTab(settings));

            if (newTab)
            {
                int newTabIndex = addTab(newTab, QString::fromUtf8(settings.m_tabName.c_str()));
                setCurrentIndex(newTabIndex);
                m_settingsForTabs.insert(AZStd::make_pair(qobject_cast<QObject*>(newTab), settings));

                connect(newTab, &QObject::destroyed, this, &StyledLogPanel::tabDestroyed);
                connect(newTab, &StyledLogTab::onLinkActivated, this, &StyledLogPanel::onLinkActivated);
            }
        }

        void StyledLogPanel::tabDestroyed(QObject* tabObject)
        {
            m_settingsForTabs.erase(tabObject);
        }

        void StyledLogPanel::onTabCloseButtonPressed(int whichTab)
        {
            closeTab(whichTab);

            insertBlankTabIfNeeded();
        }

        void StyledLogPanel::closeTab(int whichTab)
        {
            QWidget* pWidget = widget(whichTab);

            if (pWidget != m_noTabsWidget)
            {
                removeTab(whichTab);
                delete pWidget;
            }
        }

        void StyledLogPanel::insertBlankTabIfNeeded()
        {
            // insert a blank tab in if need be, so that the buttons to add/reset/etc stay visible and active
            if (count() < 1)
            {
                setTabsClosable(false);
                m_noTabsWidget->setVisible(true);
                addTab(m_noTabsWidget, "");
            }
        }

        void StyledLogPanel::removeBlankTab()
        {
            int noTabWidgetIndex = indexOf(m_noTabsWidget);
            if (noTabWidgetIndex >= 0)
            {
                removeTab(noTabWidgetIndex);
                m_noTabsWidget->setVisible(false);

                setTabsClosable(true);
            }
        }

        void StyledLogPanel::tabInserted(int /*index*/)
        {
            if (count() > 1)
            {
                removeBlankTab();
            }
        }

        bool StyledLogPanel::LoadState()
        {
            AZStd::intrusive_ptr<SavedState> savedState;

            if (m_storageID != 0)
            {
                savedState = AZ::UserSettings::Find<SavedState>(m_storageID, AZ::UserSettings::CT_LOCAL);
            }

            if (savedState)
            {
                if (savedState->m_tabSettings.empty())
                {
                    return false;
                }

                while (count())
                {
                    QWidget* pWidget = widget(0);
                    removeTab(0);
                    delete pWidget;
                }

                for (const TabSettings& settings : savedState->m_tabSettings)
                {
                    AddLogTab(settings);
                }
            }
            else
            {
                return false;
            }

            return true;
        }

        void StyledLogPanel::SaveState()
        {
            if (m_storageID == 0)
            {
                AZ_TracePrintf("Debug", "A log window not storing its state because it has not been assigned a storage ID.");
                return;
            }

            AZStd::intrusive_ptr<SavedState> myState = AZ::UserSettings::CreateFind<SavedState>(m_storageID, AZ::UserSettings::CT_LOCAL);
            myState->m_tabSettings.clear(); // because it might find an existing state!

            for (auto pair : m_settingsForTabs)
            {
                myState->m_tabSettings.push_back(pair.second);
            }
        }

        void StyledLogPanel::onAddTriggered(bool /*checked*/)
        {
            // user clicked the "Add..." button

            NewLogTabDialog newDialog(this);
            if (newDialog.exec() == QDialog::Accepted)
            {
                // add a new tab with those settings.
                TabSettings settings(newDialog.m_tabName.toUtf8().data(),
                    newDialog.m_windowName.toUtf8().data(),
                    newDialog.m_textFilter.toUtf8().data(),
                    newDialog.m_checkNormal,
                    newDialog.m_checkWarning,
                    newDialog.m_checkError,
                    newDialog.m_checkDebug);

                AddLogTab(settings);
            }
        }

        void StyledLogPanel::onResetTriggered(bool /*checked*/)
        {
            // user clicked the "Reset" button

            removeBlankTab();

            while (widget(0))
            {
                closeTab(0);
            }

            Q_EMIT TabsReset();

            insertBlankTabIfNeeded();
        }

        void StyledLogPanel::onCopyAllTriggered()
        {
            QWidget* currentTab = currentWidget();
            if (currentTab)
            {
                QMetaObject::invokeMethod(currentTab, "CopyAll", Qt::QueuedConnection);
            }
        }

        void StyledLogPanel::SetStorageID(AZ::u32 id)
        {
            m_storageID = id;
        }

        void StyledLogPanel::Reflect(AZ::ReflectContext* reflection)
        {
            SavedState::Reflect(reflection);
        }

        QWidget* StyledLogPanel::CreateTab(const TabSettings& settings)
        {
            return aznew StyledLogTab(settings, this);
        }

        StyledLogTab::StyledLogTab(const TabSettings& settings, QWidget* parent)
            : AzQtComponents::TableView(parent)
            , m_settings(settings)
        {
            setModel(new Logging::LogTableModel(this));
            setItemDelegate(new Logging::LogTableItemDelegate(this));
            setExpandOnSelection(true);

            CreateActions();

            setContextMenuPolicy(Qt::DefaultContextMenu);
        }

        void StyledLogTab::CreateActions()
        {
            m_actionSelectAll = new QAction(tr("Select All"), this);
            connect(m_actionSelectAll, &QAction::triggered, this, &StyledLogTab::selectAll);

            m_actionSelectNone = new QAction(tr("Select None"), this);
            connect(m_actionSelectNone, &QAction::triggered, this, &StyledLogTab::clearSelection);

            m_actionCopySelected = new QAction(tr("Copy"), this);
            m_actionCopySelected->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            connect(m_actionCopySelected, &QAction::triggered, this, &StyledLogTab::CopySelected);

            m_actionCopyAll = new QAction(tr("Copy All"), this);
            connect(m_actionCopyAll, &QAction::triggered, this, &StyledLogTab::CopyAll);

            addAction(m_actionSelectAll);
            addAction(m_actionSelectNone);
            addAction(m_actionCopySelected);
            addAction(m_actionCopyAll);
        }

        bool StyledLogTab::IsAtMaxScroll() const
        {
            return (verticalScrollBar()->value() == verticalScrollBar()->maximum());
        }

        // override this if you want to provide an implementation that decorates the text in some way.
        QString StyledLogTab::ConvertRowToText(const QModelIndex& row)
        {
            QAbstractItemModel* pModel = model();
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

        void StyledLogTab::contextMenuEvent(QContextMenuEvent* /*ev*/)
        {
            auto actionList = actions();

            // if we're only doing single selection, select all makes no sense
            if (selectionMode() == SingleSelection)
            {
                actionList.removeAll(m_actionSelectAll);
            }

            QMenu::exec(actionList, QCursor::pos(), nullptr, this);
        }

        void StyledLogTab::CopySelected()
        {
            QAbstractItemModel* pModel = model();
            if (!pModel)
            {
                return;
            }

            QString accumulator;
            QItemSelectionModel* selectionModel = this->selectionModel();
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

        void StyledLogTab::CopyAll()
        {
            QAbstractItemModel* pModel = model();
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

        TabSettings StyledLogTab::settings() const
        {
            return m_settings;
        }

    } // namespace LogPanel
} // namespace AzToolsFramework

#include "UI/Logging/moc_StyledLogPanel.cpp"

