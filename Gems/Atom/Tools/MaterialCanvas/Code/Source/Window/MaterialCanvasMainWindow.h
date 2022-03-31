/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentMainWindow.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <Viewport/MaterialCanvasViewportWidget.h>
#include <Window/ToolBar/MaterialCanvasToolBar.h>
AZ_POP_DISABLE_WARNING
#endif

namespace MaterialCanvas
{
    //! MaterialCanvasMainWindow is the main class. Its responsibility is limited to initializing and connecting
    //! its panels, managing selection of assets, and performing high-level actions like saving. It contains...
    //! 2) MaterialCanvasViewport        - The user can see the selected Material applied to a model.
    //! 3) MaterialPropertyInspector  - The user edits the properties of the selected Material.
    class MaterialCanvasMainWindow : public AtomToolsFramework::AtomToolsDocumentMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasMainWindow, AZ::SystemAllocator, 0);

        using Base = AtomToolsFramework::AtomToolsDocumentMainWindow;

        MaterialCanvasMainWindow(const AZ::Crc32& toolId, QWidget* parent = 0);

    protected:
        // AtomToolsFramework::AtomToolsMainWindowRequestBus::Handler overrides...
        void ResizeViewportRenderTarget(uint32_t width, uint32_t height) override;
        void LockViewportRenderTargetSize(uint32_t width, uint32_t height) override;
        void UnlockViewportRenderTargetSize() override;

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::AtomToolsDocumentMainWindow overrides...
        bool GetCreateDocumentParams(AZStd::string& openPath, AZStd::string& savePath) override;
        bool GetOpenDocumentParams(AZStd::string& openPath) override;
        void OpenSettings() override;
        void OpenHelp() override;

        AtomToolsFramework::AtomToolsDocumentInspector* m_materialInspector = {};
        MaterialCanvasViewportWidget* m_materialViewport = {};
        MaterialCanvasToolBar* m_toolBar = {};
    };
} // namespace MaterialCanvas
