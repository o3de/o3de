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
#endif

namespace MaterialCanvas
{
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

        const AZ::Uuid m_documentId;
        bool m_openedBefore = false;
    };
} // namespace MaterialCanvas
