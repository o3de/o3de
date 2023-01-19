/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AzCore/Component/TickBus.h>

namespace AtomToolsFramework
{
    //! The manager will monitor document notifications, process queued graph compiler requests, and report status of generated files from
    //! the AP.
    class GraphCompilerManager
        : private AtomToolsDocumentNotificationBus::Handler
        , public AZ::SystemTickBus::Handler
    {
    public:
        AZ_TYPE_INFO(GraphCompilerManager, "{83FE9A69-696B-464A-A79B-CFF7C152B7D2}");

        GraphCompilerManager(const AZ::Crc32& toolId);
        ~GraphCompilerManager();
        void RegisterGraphCompiler(const AZ::Uuid& documentId, GraphCompiler* graphCompiler);
        void UnregisterGraphCompiler(const AZ::Uuid& documentId);

    private:
        // AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentSaved(const AZ::Uuid& documentId) override;
        void OnDocumentUndoStateChanged(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;

        // AZ::SystemTickBus::Handler overrides...
        void OnSystemTick() override;

        const AZ::Crc32 m_toolId = {};
        AZStd::unordered_map<AZ::Uuid, AZStd::unique_ptr<GraphCompiler>> m_graphCompilerMap;
    };
} // namespace AtomToolsFramework
