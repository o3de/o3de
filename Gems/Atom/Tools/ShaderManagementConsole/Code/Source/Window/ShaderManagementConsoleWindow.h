/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentMainWindow.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <Window/ShaderManagementConsoleBrowserWidget.h>
#include <Window/ToolBar/ShaderManagementConsoleToolBar.h>

#include <QStandardItemModel>
AZ_POP_DISABLE_WARNING
#endif

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleWindow is the main class. Its responsibility is limited to initializing and connecting
    //! its panels, managing selection of assets, and performing high-level actions like saving. It contains...
    class ShaderManagementConsoleWindow
        : public AtomToolsFramework::AtomToolsDocumentMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleWindow, AZ::SystemAllocator, 0);

        using Base = AtomToolsFramework::AtomToolsDocumentMainWindow;

        ShaderManagementConsoleWindow(QWidget* parent = 0);
        ~ShaderManagementConsoleWindow() = default;

    protected:
        QWidget* CreateDocumentTabView(const AZ::Uuid& documentId) override;

        ShaderManagementConsoleToolBar* m_toolBar = nullptr;
    };
} // namespace ShaderManagementConsole
