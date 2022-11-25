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
#include <AtomToolsFramework/GraphView/GraphView.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <GraphCanvas/Components/Nodes/Comment/CommentBus.h>
#include <GraphCanvas/Types/ConstructPresets.h>
#endif

namespace MaterialCanvas
{
    //! The implementation of the graph view requires construct presets in order to be able to create node groups and comment blocks.
    class MaterialCanvasGraphConstructPresets : public GraphCanvas::EditorConstructPresets
    {
    public:
        AZ_RTTI(MaterialCanvasGraphConstructPresets, "{8E349BC8-1D8B-4A1B-8DE0-FFD61438DBBD}", GraphCanvas::EditorConstructPresets);
        AZ_CLASS_ALLOCATOR(MaterialCanvasGraphConstructPresets, AZ::SystemAllocator, 0);

        MaterialCanvasGraphConstructPresets() = default;
        ~MaterialCanvasGraphConstructPresets() = default;
        void InitializeConstructType(GraphCanvas::ConstructType constructType) override;
    };

    //! MaterialCanvasGraphView handles displaying and managing interactions for a single graph
    class MaterialCanvasGraphView
        : public AtomToolsFramework::GraphView
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasGraphView, AZ::SystemAllocator, 0);

        MaterialCanvasGraphView(
            const AZ::Crc32& toolId,
            const AZ::Uuid& documentId,
            const AtomToolsFramework::GraphViewConfig& graphViewConfig,
            QWidget* parent = 0);
        ~MaterialCanvasGraphView();

    protected:
        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;

        // GraphCanvas::AssetEditorSettingsRequestBus::Handler overrides...
        GraphCanvas::EditorConstructPresets* GetConstructPresets() const override;
        const GraphCanvas::ConstructTypePresetBucket* GetConstructTypePresetBucket(GraphCanvas::ConstructType constructType) const override;

        const AZ::Uuid m_documentId;
        bool m_openedBefore = false;
        mutable MaterialCanvasGraphConstructPresets m_materialCanvasGraphConstructPresets;
    };
} // namespace MaterialCanvas
