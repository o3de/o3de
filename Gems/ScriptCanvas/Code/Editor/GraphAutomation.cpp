/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/GraphAutomation.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/utility/charconv.h>
#include <AzCore/Utils/Utils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/LyViewPaneNames.h>
#include <Editor/View/Widgets/GraphTabBar.h>
#include <Editor/View/Windows/MainWindow.h>
#include <ScriptCanvas/Bus/UndoBus.h>
#include <ScriptCanvas/Components/EditorUtils.h>

#include <QApplication>
#include <QFileInfo>
#include <QThread>

namespace ScriptCanvasEditor
{
    void GraphAutomation::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->RegisterGenericType<AutomationValueMap>();
            serialize->RegisterGenericType<AutomationValueList>();
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->EBus<GraphAutomationRequestBus>("GraphAutomationRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "scriptcanvas.automation")
                ->Event("Create", &GraphAutomationRequests::Create)
                ->Event("Open", &GraphAutomationRequests::Open)
                ->Event("Reload", &GraphAutomationRequests::Reload)
                ->Event("Describe", &GraphAutomationRequests::Describe)
                ->Event("Palette", &GraphAutomationRequests::Palette)
                ->Event("AddNode", &GraphAutomationRequests::AddNode)
                ->Event("Connect", &GraphAutomationRequests::Connect)
                ->Event("SetValue", &GraphAutomationRequests::SetValue)
                ->Event("SetVariable", &GraphAutomationRequests::SetVariable)
                ->Event("RemoveNodes", &GraphAutomationRequests::RemoveNodes)
                ->Event("RemoveConnections", &GraphAutomationRequests::RemoveConnections)
                ->Event("Save", &GraphAutomationRequests::Save)
                ->Event("PollSave", &GraphAutomationRequests::PollSave)
                ->Event("CancelSave", &GraphAutomationRequests::CancelSave)
                ->Event("Attach", &GraphAutomationRequests::Attach)
                ->Event("Attachment", &GraphAutomationRequests::Attachment)
                ->Event("Undo", &GraphAutomationRequests::Undo)
                ->Event("Redo", &GraphAutomationRequests::Redo);
        }
    }

    void GraphAutomation::Activate()
    {
        GraphAutomationNotificationBus::Handler::BusConnect();
        GraphAutomationRequestBus::Handler::BusConnect();
    }

    void GraphAutomation::Deactivate()
    {
        GraphAutomationRequestBus::Handler::BusDisconnect();
        GraphAutomationNotificationBus::Handler::BusDisconnect();
        CancelSave();
    }

    GraphAutomation::WindowResult GraphAutomation::Window()
    {
        if (!qApp || QThread::currentThread() != qApp->thread())
        {
            return AZ::Failure(AZStd::string("ScriptCanvas automation requires the editor main thread"));
        }

        bool playing = false;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            playing, &AzToolsFramework::EditorEntityContextRequests::IsEditorRunningGame);
        if (playing)
        {
            return AZ::Failure(AZStd::string("Stop play before editing ScriptCanvas"));
        }

        if (m_saving)
        {
            return AZ::Failure(AZStd::string("Consume or cancel the active graph save first"));
        }

        auto* window = AzToolsFramework::GetViewPaneWidget<MainWindow>(LyViewPane::ScriptCanvas);
        if (!window)
        {
            AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);
            window = AzToolsFramework::GetViewPaneWidget<MainWindow>(LyViewPane::ScriptCanvas);
        }

        if (!window)
        {
            return AZ::Failure(AZStd::string("ScriptCanvas editor view is unavailable"));
        }

        if (window->m_fileSaver)
        {
            return AZ::Failure(AZStd::string("ScriptCanvas native save is still in progress"));
        }

        return AZ::Success(window);
    }

    GraphAutomation::WindowResult GraphAutomation::GraphWindow(const AZStd::string& graph)
    {
        auto result = Window();
        if (!result)
        {
            return result;
        }

        const AZ::EntityId graphId = EntityId(graph);
        if (!graphId.IsValid() || result.GetValue()->GetActiveScriptCanvasId() != graphId)
        {
            return AZ::Failure(AZStd::string("Graph is not the active open ScriptCanvas document; stale or wrong-domain id"));
        }

        return result;
    }

    AZStd::string GraphAutomation::IdText(AZ::EntityId id)
    {
        return AZStd::to_string(static_cast<AZ::u64>(id));
    }

    AZ::EntityId GraphAutomation::EntityId(const AZStd::string& text)
    {
        const char* const end = text.data() + text.size();
        AZ::u64 value = 0;
        const auto parsed = AZStd::from_chars(text.data(), end, value);
        if (parsed.ec != AZStd::errc{} || parsed.ptr != end)
        {
            return AZ::EntityId();
        }

        return AZ::EntityId(value);
    }

    AZ::Outcome<AZ::IO::Path, AZStd::string> GraphAutomation::SourcePath(const AZStd::string& text)
    {
        if (text.empty() || text.find('\0') != AZStd::string::npos)
        {
            return AZ::Failure(AZStd::string("Expected a nonempty source path without NUL"));
        }

        const AZ::IO::Path path = AZ::IO::Path(text).LexicallyNormal();
        const AZ::IO::Path project(AZ::Utils::GetProjectPath());
        if (!path.IsAbsolute() || !path.IsRelativeTo(project) || path.Extension() != SourceDescription::GetFileExtension())
        {
            return AZ::Failure(AZStd::string("Expected an absolute project .scriptcanvas source path"));
        }

        // The file may not exist yet, so resolve symbolic links on its closest existing ancestor.
        QFileInfo ancestor(QString::fromUtf8(path.c_str()));
        while (!ancestor.exists())
        {
            ancestor.setFile(ancestor.absolutePath());
        }

        const AZ::IO::Path canonical(ancestor.canonicalFilePath().toUtf8().constData());
        const QFileInfo projectInfo(QString::fromUtf8(project.c_str()));
        const AZ::IO::Path projectCanonical(projectInfo.canonicalFilePath().toUtf8().constData());
        if (!canonical.IsRelativeTo(projectCanonical))
        {
            return AZ::Failure(AZStd::string("Source path escapes the project through a symbolic link"));
        }

        return AZ::Success(path);
    }

    AutomationValueMap GraphAutomation::Identity(MainWindow& window)
    {
        const AZ::EntityId graph = window.GetActiveScriptCanvasId();
        const AZ::IO::Path& path = window.m_activeGraph.AbsolutePath();

        bool canUndo = false;
        UndoRequestBus::EventResult(canUndo, graph, &UndoRequests::CanUndo);

        bool canRedo = false;
        UndoRequestBus::EventResult(canRedo, graph, &UndoRequests::CanRedo);

        AZ::Data::AssetInfo sourceInfo;
        AZStd::string scanFolder;
        bool registered = false;
        if (!path.empty())
        {
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                registered,
                &AzToolsFramework::AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath,
                path.c_str(),
                sourceInfo,
                scanFolder);
        }

        const auto metadata = window.m_tabBar->GetTabData(window.m_activeGraph);
        const bool dirty = metadata && metadata->m_fileState != Tracker::ScriptCanvasFileState::UNMODIFIED;
        const AZStd::string sourceUuid = registered ? sourceInfo.m_assetId.m_guid.ToString<AZStd::string>() : AZStd::string();

        AutomationValueMap identity;
        identity["graph"] = IdText(graph);
        identity["graphCanvas"] = IdText(window.GetActiveGraphCanvasGraphId());
        identity["path"] = path.Native();
        identity["sourceRegistered"] = registered;
        identity["sourceUuid"] = sourceUuid;
        identity["documentUuid"] = window.m_activeGraph.Id().ToString<AZStd::string>();
        identity["dirty"] = dirty;
        identity["canUndo"] = canUndo;
        identity["canRedo"] = canRedo;
        return identity;
    }

    AutomationResult GraphAutomation::Create(const AZStd::string& text)
    {
        auto path = SourcePath(text);
        if (!path)
        {
            return AZ::Failure(path.GetError());
        }

        if (AZ::IO::SystemFile::Exists(path.GetValue().c_str()))
        {
            return AZ::Failure(AZStd::string("Create refuses an existing graph source"));
        }

        auto window = Window();
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        auto created = window.GetValue()->CreateScriptCanvasAsset(path.GetValue().Native());
        if (!created)
        {
            return AZ::Failure(created.GetError());
        }

        AutomationValueMap identity = Identity(*window.GetValue());
        identity.emplace("suggestedPath", path.GetValue().Native());
        return AZ::Success(AZStd::move(identity));
    }

    AutomationResult GraphAutomation::Open(const AZStd::string& text)
    {
        auto path = SourcePath(text);
        if (!path)
        {
            return AZ::Failure(path.GetError());
        }

        auto window = Window();
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        auto source = CreateFromAnyPath(SourceHandle(), path.GetValue());
        if (!source)
        {
            return AZ::Failure(AZStd::string("Asset Processor has not registered the ScriptCanvas source"));
        }

        auto opened = window.GetValue()->OpenScriptCanvasAsset(*source, Tracker::ScriptCanvasFileState::UNMODIFIED);
        if (!opened)
        {
            return AZ::Failure(opened.GetError());
        }

        return AZ::Success(Identity(*window.GetValue()));
    }

    AutomationResult GraphAutomation::Reload(const AZStd::string& graph, bool discard)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        MainWindow& owner = *window.GetValue();
        const auto metadata = owner.m_tabBar->GetTabData(owner.m_activeGraph);
        const bool dirty = metadata && metadata->m_fileState != Tracker::ScriptCanvasFileState::UNMODIFIED;
        if (dirty && !discard)
        {
            return AZ::Failure(AZStd::string("Save or explicitly discard changes before reloading the graph"));
        }

        const AZStd::string path = owner.m_activeGraph.AbsolutePath().Native();
        if (path.empty() || !AZ::IO::SystemFile::Exists(path.c_str()))
        {
            return AZ::Failure(AZStd::string("Graph has no saved source to reload"));
        }

        owner.ForceCloseActiveAsset();
        return Open(path);
    }

    AutomationResult GraphAutomation::Save(const AZStd::string& graph, const AZStd::string& text)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        auto path = SourcePath(text);
        if (!path)
        {
            return AZ::Failure(path.GetError());
        }

        MainWindow& owner = *window.GetValue();
        if (!AssetHelpers::IsValidSourceFile(path.GetValue().Native(), owner.GetActiveScriptCanvasId()))
        {
            return AZ::Failure(AZStd::string("Native ScriptCanvas source type validation refused the path"));
        }

        if (AZ::IO::SystemFile::Exists(path.GetValue().c_str()) && owner.m_activeGraph.AbsolutePath() != path.GetValue())
        {
            return AZ::Failure(AZStd::string("Save refuses overwriting a different existing graph"));
        }

        if (!AZ::IO::SystemFile::CreateDir(AZ::IO::Path(path.GetValue().ParentPath()).c_str()))
        {
            return AZ::Failure(AZStd::string("Cannot create graph source directory"));
        }

        owner.PrepareAssetForSave(owner.m_activeGraph);

        m_saving = true;
        m_saveResult.reset();

        // MainWindow::OnSaveCallBack reports the result on GraphAutomationNotificationBus.
        owner.m_automationSave = true;
        owner.SaveAs(path.GetValue().Native(), owner.m_activeGraph);

        return AZ::Success(AutomationValueMap{ { "pending", true } });
    }

    void GraphAutomation::OnAutomationSaveComplete(const AZStd::string& path, const AZStd::string& error)
    {
        // Only a save started by Save is observed.
        if (!m_saving)
        {
            return;
        }

        if (error.empty())
        {
            m_saveResult = AutomationResult(AZ::Success(AutomationValueMap{ { "pending", false }, { "path", path } }));
        }
        else
        {
            m_saveResult = AutomationResult(AZ::Failure(error));
        }
    }

    AutomationResult GraphAutomation::PollSave()
    {
        if (!m_saving)
        {
            return AZ::Failure(AZStd::string("No observed ScriptCanvas save is active"));
        }

        if (!m_saveResult)
        {
            return AZ::Success(AutomationValueMap{ { "pending", true } });
        }

        AutomationResult result = AZStd::move(*m_saveResult);
        CancelSave();
        return result;
    }

    void GraphAutomation::CancelSave()
    {
        m_saving = false;
        m_saveResult.reset();
    }

    AutomationResult GraphAutomation::Undo(const AZStd::string& graph)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        bool available = false;
        UndoRequestBus::EventResult(available, EntityId(graph), &UndoRequests::CanUndo);
        if (!available)
        {
            return AZ::Failure(AZStd::string("Graph has no undoable change"));
        }

        window.GetValue()->TriggerUndo();
        return Describe(graph);
    }

    AutomationResult GraphAutomation::Redo(const AZStd::string& graph)
    {
        auto window = GraphWindow(graph);
        if (!window)
        {
            return AZ::Failure(window.GetError());
        }

        bool available = false;
        UndoRequestBus::EventResult(available, EntityId(graph), &UndoRequests::CanRedo);
        if (!available)
        {
            return AZ::Failure(AZStd::string("Graph has no redoable change"));
        }

        window.GetValue()->TriggerRedo();
        return Describe(graph);
    }
} // namespace ScriptCanvasEditor
