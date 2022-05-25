/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Grammar/ParsingUtilitiesScriptEventExtension.h>

#include <ScriptEvents/ScriptEventsBus.h>

namespace ScriptEvents
{
    namespace Editor
    {
        using namespace ScriptCanvas;

        AZStd::pair<bool, AZStd::string> MakeHelpersAction([[maybe_unused]] const ScriptCanvas::SourceHandle& sourceHandle)
        {
            /*
            if (!sourceHandle.Mod())
            {
                return { false, "no active graph" };
            }

            // redo this part
            // return GraphCanvas::ContextMenuAction::SceneReaction::PostUndo;

            if (graph)
            {
                graph->MarkScriptEventExtension();
            }

            AddScriptEventHelpers(*sourceHandle.Mod());
            */

            return { true, "" };
        }

        void OpenAction()
        {
            /*
            AZ::IO::FixedMaxPath resolvedProjectRoot;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedProjectRoot, "@projectroot@");

            AZStd::string errorMessage;

            const QStringList nameFilters = { "ScriptEvent Files Saved from the ScriptCanvas Editor (*.scriptevents)" };
            QFileDialog dialog(nullptr, tr("Open a single file..."), resolvedProjectRoot.c_str());
            dialog.setFileMode(QFileDialog::ExistingFiles);
            dialog.setNameFilters(nameFilters);

            if (dialog.exec() == QDialog::Accepted && dialog.selectedFiles().count() == 1)
            {
                const AZ::IO::Path path(dialog.selectedFiles().begin()->toUtf8().constData());
                AZ::Outcome<ScriptEvents::ScriptEvent, AZStd::string> loadOutcome = AZ::Failure(AZStd::string("failed to save"));
                ScriptEventBus::BroadcastResult
                    ( loadOutcome
                    , &ScriptEvents::ScriptEventRequests::LoadDefinitionSource
                    , path);

                if (loadOutcome.IsSuccess())
                {
                    using namespace ScriptCanvas::ScriptEventGrammar;
                    ScriptCanvas::ScriptCanvasId scriptCanvasId;
                    GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);
                    ScriptCanvas::Graph* graph = nullptr;
                    ScriptCanvas::GraphRequestBus::EventResult(graph, scriptCanvasId, &ScriptCanvas::GraphRequests::GetGraph);

                    auto deserializeResult = Deserialize(loadOutcome.GetValue().GetScriptCanvasSerializationData(), MakeInternalGraphEntitiesUnique::Yes);
                    if (!deserializeResult.m_isSuccessful)
                    {
                        errorMessage = "failed to deserialize script canvas source data";
                    }

                    if (graph)
                    {
                        graph->MarkScriptEventExtension();
                    }

                    // send open file thing to main window
                    // now open create an open a scriptcanvas graph from the script event
                    return;
                }
                else
                {
                    errorMessage = "Failed to load selected file.";
                }
            }
            else
            {
                errorMessage = "Failed to select file.";
            }

            QMessageBox mb
                ( QMessageBox::Warning
                , tr("Failed to open ScriptEvent file into ScriptCanvas Editor.")
                , errorMessage.c_str()
                , QMessageBox::Close
                , nullptr);
                */
        }

        AZStd::pair<bool, AZStd::string> SaveAsAction([[maybe_unused]] const ScriptCanvas::SourceHandle& sourceHandle)
        {
            /*
            if (graph)
            {
                graph->MarkScriptEventExtension();
            }

            using namespace ScriptCanvas::ScriptEventGrammar;

            AZStd::string errorWindowTitle;
            AZStd::string errorMessage;
            AZStd::vector<AZStd::string> parseErrors;

            if (sourceHandle.Get())
            {
                GraphToScriptEventsResult result = ParseScriptEventsDefinition(*sourceHandle.Get());

                if (result.m_isScriptEvents)
                {
                    AZ::IO::FixedMaxPath resolvedProjectRoot;
                    AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedProjectRoot, "@projectroot@");
                    const auto fileName = QFileDialog::getSaveFileName
                        ( nullptr, tr("Save As..."), resolvedProjectRoot.c_str(), tr("All ScriptEvent Files (*.scriptevents)"));

                    if (!fileName.isEmpty())
                    {
                        AZ::IO::Path path(fileName.toUtf8().constData());
                        // use the ScriptEventBus, also to get fundamental types(?)
                        AZ::Outcome<void, AZStd::string> saveOutcome = AZ::Failure(AZStd::string("failed to save"));
                        ScriptEvents::ScriptEventBus::BroadcastResult
                            ( saveOutcome
                            , &ScriptEvents::ScriptEventRequests::SaveDefinitionSourceFile
                            , result.m_event
                            , path);

                        if (!saveOutcome.IsSuccess())
                        {
                            errorMessage = saveOutcome.TakeError();
                        }
                    }
                    else
                    {
                        errorMessage = "Invalid file name.";
                    }
                }
                else
                {
                    errorMessage = "Changes are required to properly parse graph as ScriptEvents file.";
                    parseErrors = AZStd::move(parseErrors);
                }
            }
            else
            {
                errorMessage = "Graph could not be found with ID: ";
                errorMessage += graphId.ToString();
                errorMessage += ".";
            }

            if (!parseErrors.empty())
            {
                for (auto& error : parseErrors)
                {
                    errorMessage += "\n * ";
                    errorMessage += error;
                }

                errorWindowTitle = "Changes are required to properly parse graph as ScriptEvents file.";
            }
            else if (!errorMessage.empty())
            {
                errorWindowTitle = "Failed to save ScriptEvents file.";
            }

            QMessageBox mb
                ( QMessageBox::Warning
                , errorWindowTitle.c_str()
                , errorMessage.c_str()
                , QMessageBox::Close
                , nullptr);
                */
            return { true, "" };
        }

        MenuItemsEnabled UpdateMenuItemsEnabled([[maybe_unused]] const ScriptCanvas::SourceHandle& sourceHandle)
        {
            MenuItemsEnabled menuItemsEnabled;

            // add helpers enabled
            /*
            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);
            ScriptCanvas::Graph* graph = nullptr;
            ScriptCanvas::GraphRequestBus::EventResult(graph, scriptCanvasId, &ScriptCanvas::GraphRequests::GetGraph);

            if (graph)
            {
                graph->MarkScriptEventExtension();
            }

            setEnabled(graph && !ScriptCanvas::ScriptEventGrammar::ParseMinimumScriptEventArtifacts(*graph).m_isScriptEvents);
            */

            // save enabled
            /* using namespace ScriptCanvas::ScriptEventGrammar;
        // ask the model if is enabled, change name to reason why...?
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphId);

        ScriptCanvas::Graph* graph = nullptr;
        ScriptCanvas::GraphRequestBus::EventResult(graph, scriptCanvasId, &ScriptCanvas::GraphRequests::GetGraph);

        if (graph)
        {
            if (const GraphToScriptEventsResult result = ParseScriptEventsDefinition(*graph); !result.m_isScriptEvents)
            {
                AZ_TracePrintf("ScriptEvents", "Failed to parse graph as ScriptEvents: ");
                for (auto& reason : result.m_parseErrors)
                {
                    AZ_TracePrintf("ScriptEvents", reason.c_str());
                }

                setEnabled(false);
                return;
            }
        }
        else
        {
            AZ_Error("ScriptEvents", false, "No valid graph was discovered for SaveAsScriptEventAction::RefreshAction");
            setEnabled(false);
            return;
        }

        setEnabled(true);
        */

            return menuItemsEnabled;
        }
    }
}
