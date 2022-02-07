/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindow.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#endif

namespace AtomToolsFramework
{
    //! AtomToolsDocumentMainWindow
    class AtomToolsDocumentMainWindow
        : public AtomToolsMainWindow
        , private AtomToolsDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(AtomToolsDocumentMainWindow, AZ::SystemAllocator, 0);

        using Base = AtomToolsMainWindow;

        AtomToolsDocumentMainWindow(QWidget* parent = 0);
        ~AtomToolsDocumentMainWindow();

    protected:
        void AddDocumentMenus();
        void AddDocumentTabBar();

        QString GetDocumentPath(const AZ::Uuid& documentId) const;

        AZ::Uuid GetDocumentTabId(const int tabIndex) const;

        void AddDocumentTab(
            const AZ::Uuid& documentId,
            const AZStd::string& label,
            const AZStd::string& toolTip);

        void RemoveDocumentTab(const AZ::Uuid& documentId);

        void UpdateDocumentTab(
            const AZ::Uuid& documentId, const AZStd::string& label, const AZStd::string& toolTip, bool isModified);

        void SelectPrevDocumentTab();
        void SelectNextDocumentTab();

        virtual QWidget* CreateDocumentTabView(const AZ::Uuid& documentId);
        virtual void OpenDocumentTabContextMenu();
        virtual bool GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath);
        virtual bool GetOpenDocumentParams(AZStd::string& openPath);

        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentModified(const AZ::Uuid& documentId) override;
        void OnDocumentUndoStateChanged(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;

        void closeEvent(QCloseEvent* closeEvent) override;

        template<typename Functor>
        QAction* CreateAction(const QString& text, Functor functor, const QKeySequence& shortcut = 0);

        QAction* m_actionNew = {};
        QAction* m_actionOpen = {};
        QAction* m_actionClose = {};
        QAction* m_actionCloseAll = {};
        QAction* m_actionCloseOthers = {};
        QAction* m_actionSave = {};
        QAction* m_actionSaveAsCopy = {};
        QAction* m_actionSaveAsChild = {};
        QAction* m_actionSaveAll = {};

        QAction* m_actionUndo = {};
        QAction* m_actionRedo = {};

        QAction* m_actionNextTab = {};
        QAction* m_actionPreviousTab = {};

        AzQtComponents::TabWidget* m_tabWidget = {};
    };
} // namespace AtomToolsFramework
