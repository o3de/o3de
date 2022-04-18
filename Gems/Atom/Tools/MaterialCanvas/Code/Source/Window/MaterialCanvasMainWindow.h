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
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsInspector.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportToolBar.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportWidget.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <QTranslator>
#endif

namespace MaterialCanvas
{
    //! MaterialCanvasMainWindow
    class MaterialCanvasMainWindow
        : public AtomToolsFramework::AtomToolsDocumentMainWindow
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasMainWindow, AZ::SystemAllocator, 0);

        using Base = AtomToolsFramework::AtomToolsDocumentMainWindow;

        MaterialCanvasMainWindow(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~MaterialCanvasMainWindow();

    protected:
        // AtomToolsFramework::AtomToolsMainWindowRequestBus::Handler overrides...
        void ResizeViewportRenderTarget(uint32_t width, uint32_t height) override;
        void LockViewportRenderTargetSize(uint32_t width, uint32_t height) override;
        void UnlockViewportRenderTargetSize() override;

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentCleared(const AZ::Uuid& documentId) override;
        void OnDocumentError(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::AtomToolsDocumentMainWindow overrides...
        void OpenSettings() override;
        void OpenHelp() override;

    private:
        GraphCanvas::GraphCanvasTreeItem* GetNodePaletteRootTreeItem() const;

        AtomToolsFramework::AtomToolsDocumentInspector* m_materialInspector = {};
        AtomToolsFramework::EntityPreviewViewportSettingsInspector* m_viewportSettingsInspector = {};
        AtomToolsFramework::EntityPreviewViewportToolBar* m_toolBar = {};
        AtomToolsFramework::EntityPreviewViewportWidget* m_materialViewport = {};
        GraphCanvas::BookmarkDockWidget* m_bookmarkDockWidget = {};
        GraphCanvas::NodePaletteDockWidget* m_nodePalette = {};
        GraphCanvas::StyleManager m_styleManager;
        QTranslator m_translator;
    };
} // namespace MaterialCanvas
