/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include "LogPanel_Panel.h" // for TabSettings
#include "LogTableModel.h"
#include "LogTableItemDelegate.h"

#include <AzQtComponents/Components/Widgets/TableView.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#endif

namespace AzToolsFramework
{
    namespace LogPanel
    {
        // Replaces BaseLogPanel
        class StyledLogPanel
            : public AzQtComponents::TabWidget
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(StyledLogPanel, AZ::SystemAllocator);
            explicit StyledLogPanel(QWidget* parent = nullptr);
            ~StyledLogPanel();

            //! SaveState uses the storageID to store which tabs were active and what their settings were
            void SaveState();
            //! LoadState reads which tabs were active last time.
            bool LoadState();

            // call this to manually add a tab.  Usually you load state instead, though, or you rely on user clicking the add button.
            void AddLogTab(const TabSettings& settings);

            //! SetStorageID is used to save and load state
            //! Give it a unique id (best using AZ_CRC) and it will remember its state next time it starts
            //! by calling SetStorageID next time, to the same number, and then calling LoadState().
            void SetStorageID(AZ::u32 id);

            static void Reflect(AZ::ReflectContext* reflection);

        Q_SIGNALS:
            void TabsReset(); // all tabs have been deleted because user clicked Reset
            void onLinkActivated(const QString& link);

        protected:
            // override this to create your tab class:
            virtual QWidget* CreateTab(const TabSettings& settings);

            void tabInserted(int index) override;

        private Q_SLOTS:
            void onTabCloseButtonPressed(int whichTab);
            void onAddTriggered(bool checked);
            void onResetTriggered(bool checked);
            void onCopyAllTriggered();

        private:
            void tabDestroyed(QObject* tabObject);
            void closeTab(int whichTab);

            void removeBlankTab();
            void insertBlankTabIfNeeded();

            int m_storageID = 0;
            AZStd::unordered_map<QObject*, TabSettings> m_settingsForTabs;

            QWidget* m_noTabsWidget = nullptr;
        };

        // Replaces BaseLogView
        class StyledLogTab
            : public AzQtComponents::TableView
        {
            Q_OBJECT
        public:
            AZ_CLASS_ALLOCATOR(StyledLogTab, AZ::SystemAllocator);
            explicit StyledLogTab(const TabSettings& settings, QWidget* pParent = nullptr);

            // utility function.  You can call this to determine if you need to continue to keep scrolling to the bottom whenever
            // you alter the data.  call scrollToBottom to scroll to the bottom!
            bool IsAtMaxScroll() const;

            TabSettings settings() const;

        protected:
            // override this if you want to provide an implementation that decorates the text in some way.
            // its only used in copy and paste, so this is what formats for the clipboard.
            // the default implementation simply concatenates all columns of text with " - " separating them.
            virtual QString ConvertRowToText(const QModelIndex& row);

            void contextMenuEvent(QContextMenuEvent* ev) override;

        private:

            void CreateActions();

            // Backing code to the context menu
            QAction* m_actionSelectAll = nullptr;
            QAction* m_actionSelectNone = nullptr;
            QAction* m_actionCopySelected = nullptr;
            QAction* m_actionCopyAll = nullptr;

            TabSettings m_settings;

        public slots:
            // Backing code to the context menu.  You can override these to do what ever
            // the default implementation will call ConvertRowToText().
            virtual void CopyAll();
            virtual void CopySelected();

        signals:
            // connect to this signal if you want to know when someone clicked a link in a URL in a rich text box.
            void onLinkActivated(const QString& link);
        };

    } // namespace LogPanel
} // namespace AzToolsFramework
