/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AtomToolsFramework/Graph/GraphDocumentRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>
#include <Document/PassGraphCompiler.h>
#include <Document/PassGraphCompilerNotificationBus.h>
#include <GraphModel/Model/Connection.h>

namespace PassCanvas
{
#if defined(AZ_ENABLE_TRACING)
    namespace
    {
        bool IsCompileLoggingEnabled()
        {
            return AtomToolsFramework::GetSettingsValue("/O3DE/Atom/PassGraphCompiler/CompileLoggingEnabled", false);
        }
    } // namespace
#endif // AZ_ENABLE_TRACING

    void PassGraphCompiler::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PassGraphCompiler>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PassGraphCompilerRequestBus>("PassGraphCompilerRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "passcanvas")
                ->Event("GetGeneratedFilePaths", &PassGraphCompilerRequests::GetGeneratedFilePaths)
                ->Event("CompileGraph", &PassGraphCompilerRequests::CompileGraph)
                ->Event("QueueCompileGraph", &PassGraphCompilerRequests::QueueCompileGraph)
                ->Event("IsCompileGraphQueued", &PassGraphCompilerRequests::IsCompileGraphQueued);
        }
    }

    PassGraphCompiler::PassGraphCompiler(const AZ::Crc32& toolId, const AZ::Uuid& documentId)
        : m_toolId(toolId)
        , m_documentId(documentId)
    {
        PassGraphCompilerRequestBus::Handler::BusConnect(documentId);
    }

    PassGraphCompiler::~PassGraphCompiler()
    {
        PassGraphCompilerRequestBus::Handler::BusDisconnect();
    }

    const AZStd::vector<AZStd::string>& PassGraphCompiler::GetGeneratedFilePaths() const
    {
        return m_generatedFiles;
    }

    bool PassGraphCompiler::CompileGraph() const
    {
        CompileGraphStarted();

        AZStd::string absolutePath;
        AtomToolsFramework::AtomToolsDocumentRequestBus::EventResult(
            absolutePath, m_documentId, &AtomToolsFramework::AtomToolsDocumentRequestBus::Events::GetAbsolutePath);

        GraphModel::GraphPtr graph;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            graph, m_documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGraph);

        AZStd::string graphName;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            graphName, m_documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGraphName);

        // Skip compilation if there is no graph or this is a template.
        if (!graph || absolutePath.ends_with("passgraphtemplate"))
        {
            CompileGraphFailed();
            return false;
        }

        CompileGraphCompleted();
        return true;
    }

    void PassGraphCompiler::CompileGraphStarted() const
    {
        m_compileGraphQueued = false;
        m_generatedFiles.clear();
        AZ_TracePrintf_IfTrue("PassGraphCompiler", IsCompileLoggingEnabled(), "Compile graph started.\n");
        PassGraphCompilerNotificationBus::Event(m_toolId, &PassGraphCompilerNotificationBus::Events::OnCompileGraphStarted, m_documentId);
    }

    void PassGraphCompiler::CompileGraphFailed() const
    {
        m_compileGraphQueued = false;
        m_generatedFiles.clear();
        AZ_TracePrintf_IfTrue("PassGraphCompiler", IsCompileLoggingEnabled(), "Compile graph failed.\n");
        PassGraphCompilerNotificationBus::Event(m_toolId, &PassGraphCompilerNotificationBus::Events::OnCompileGraphFailed, m_documentId);
    }

    void PassGraphCompiler::CompileGraphCompleted() const
    {
        m_compileGraphQueued = false;
        AZ_TracePrintf_IfTrue("PassGraphCompiler", IsCompileLoggingEnabled(), "Compile graph completed.\n");
        PassGraphCompilerNotificationBus::Event(m_toolId, &PassGraphCompilerNotificationBus::Events::OnCompileGraphCompleted, m_documentId);
    }

    void PassGraphCompiler::QueueCompileGraph() const
    {
        GraphModel::GraphPtr graph;
        AtomToolsFramework::GraphDocumentRequestBus::EventResult(
            graph, m_documentId, &AtomToolsFramework::GraphDocumentRequestBus::Events::GetGraph);

        if (graph && !m_compileGraphQueued)
        {
            m_compileGraphQueued = true;
            AZ::SystemTickBus::QueueFunction([id = m_documentId](){
                PassGraphCompilerRequestBus::Event(id, &PassGraphCompilerRequestBus::Events::CompileGraph);
            });
        }
    }

    bool PassGraphCompiler::IsCompileGraphQueued() const
    {
        return m_compileGraphQueued;
    }
} // namespace PassCanvas
