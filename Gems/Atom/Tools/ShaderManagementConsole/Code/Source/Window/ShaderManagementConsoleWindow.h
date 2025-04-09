/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentMainWindow.h>

namespace ShaderManagementConsole
{
    struct DocumentVerificationResult;

    //! ShaderManagementConsoleWindow is the main class. Its responsibility is limited to initializing and connecting
    //! its panels, managing selection of assets, and performing high-level actions like saving. It contains...
    class ShaderManagementConsoleWindow : public AtomToolsFramework::AtomToolsDocumentMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleWindow, AZ::SystemAllocator);

        using Base = AtomToolsFramework::AtomToolsDocumentMainWindow;

        ShaderManagementConsoleWindow(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~ShaderManagementConsoleWindow() = default;

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::AtomToolsDocumentMainWindow overrides...
        AZStd::string GetSaveDocumentParams(const AZStd::string& initialPath, const AZ::Uuid& documentId) const override;

        // AtomToolsMainWindowRequestBus::Handler overrides...
        void CreateMenus(QMenuBar* menuBar) override;

        void GenerateStatisticView();

        void ShowContextMenu(const QPoint& pos);

        void ErrorMessageBoxesForDocumentVerification(const DocumentVerificationResult& verification);

    private:
        void closeEvent(QCloseEvent* closeEvent) override;

        AtomToolsFramework::AtomToolsDocumentInspector* m_documentInspector = {};
        QWidget* m_statisticView = nullptr;
    };
} // namespace ShaderManagementConsole
