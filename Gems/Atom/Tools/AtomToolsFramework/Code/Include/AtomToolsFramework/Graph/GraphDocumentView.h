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
#include <AtomToolsFramework/Graph/GraphView.h>
#include <AzCore/Settings/SettingsRegistry.h>
#endif

namespace AtomToolsFramework
{
    //! GraphDocumentView bridges document system managed graph model graphs with a single graph view per document 
    class GraphDocumentView
        : public GraphView
        , private AtomToolsDocumentNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GraphDocumentView, AZ::SystemAllocator);

        GraphDocumentView(
            const AZ::Crc32& toolId, const AZ::Uuid& documentId, GraphViewSettingsPtr graphViewSettingsPtr, QWidget* parent = 0);
        ~GraphDocumentView();

    protected:
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;

        void OnSettingsChanged();

        const AZ::Uuid m_documentId;
        bool m_openedBefore = false;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_graphViewSettingsNotifyEventHandler;
    };
} // namespace AtomToolsFramework
