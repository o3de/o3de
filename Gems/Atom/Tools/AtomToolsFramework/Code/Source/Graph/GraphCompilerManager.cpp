/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphCompilerManager.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/RTTI.h>

namespace AtomToolsFramework
{
    GraphCompilerManager::GraphCompilerManager(const AZ::Crc32& toolId)
        : m_toolId(toolId)
    {
        AZ::SystemTickBus::Handler::BusConnect();
        AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
    }

    GraphCompilerManager::~GraphCompilerManager()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
    }

    void GraphCompilerManager::RegisterGraphCompiler(const AZ::Uuid& documentId, GraphCompiler* graphCompiler)
    {
        m_graphCompilerMap.emplace(documentId, graphCompiler);
    }

    void GraphCompilerManager::UnregisterGraphCompiler(const AZ::Uuid& documentId)
    {
        m_graphCompilerMap.erase(documentId);
    }

    void GraphCompilerManager::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        if (GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnOpen", true))
        {
            GraphCompilerRequestBus::Event(documentId, &GraphCompilerRequestBus::Events::QueueCompileGraph);
        }
    }

    void GraphCompilerManager::OnDocumentSaved(const AZ::Uuid& documentId)
    {
        if (GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnSave", true))
        {
            GraphCompilerRequestBus::Event(documentId, &GraphCompilerRequestBus::Events::QueueCompileGraph);
        }
    }

    void GraphCompilerManager::OnDocumentUndoStateChanged(const AZ::Uuid& documentId)
    {
        if (GetSettingsValue("/O3DE/AtomToolsFramework/GraphCompiler/CompileOnEdit", true))
        {
            GraphCompilerRequestBus::Event(documentId, &GraphCompilerRequestBus::Events::QueueCompileGraph);
        }
    }

    void GraphCompilerManager::OnDocumentClosed(const AZ::Uuid& documentId)
    {
        UnregisterGraphCompiler(documentId);
    }

    void GraphCompilerManager::OnDocumentDestroyed(const AZ::Uuid& documentId)
    {
        UnregisterGraphCompiler(documentId);
    }

    void GraphCompilerManager::OnSystemTick()
    {
        for (auto& graphCompilerPair : m_graphCompilerMap)
        {
            const auto& documentId = graphCompilerPair.first;

            bool isCompileGraphQueued = false;
            GraphCompilerRequestBus::EventResult(isCompileGraphQueued, documentId, &GraphCompilerRequestBus::Events::IsCompileGraphQueued);
            if (isCompileGraphQueued)
            {
                GraphCompilerRequestBus::Event(documentId, &GraphCompilerRequestBus::Events::CompileGraph);
            }

            bool isReportGeneratedFileStatusComplete = true;
            GraphCompilerRequestBus::EventResult(
                isReportGeneratedFileStatusComplete, documentId, &GraphCompilerRequestBus::Events::ReportGeneratedFileStatus);
            if (!isReportGeneratedFileStatusComplete)
            {
                break;
            }
        }
    }
} // namespace AtomToolsFramework
