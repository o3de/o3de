/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Include/ScriptCanvas/Components/EditorGraph.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/GraphSerialization.h>
#include <ScriptCanvas/Grammar/ParsingUtilitiesScriptEventExtension.h>
#include <ScriptEvents/ScriptEventsBus.h>

#include <Editor/View/Windows/ScriptEventMenu.h>

namespace ScriptEvents
{
    namespace Editor
    {
        void ClearStatusAction(const ScriptCanvas::SourceHandle& sourceHandle)
        {
            if (sourceHandle.Mod())
            {
                sourceHandle.Mod()->ClearScriptEventExtension();
            }
        }

        AZStd::pair<bool, AZStd::string> MakeHelpersAction(const ScriptCanvas::SourceHandle& sourceHandle)
        {
            if (!sourceHandle.Mod())
            {
                return { false, "no active graph" };
            }

            sourceHandle.Mod()->MarkScriptEventExtension();
            AddScriptEventHelpers(*sourceHandle.Mod());
            return { true, "" };
        }

        AZStd::pair<ScriptCanvas::SourceHandle, AZStd::string> OpenAction()
        {
            using namespace ScriptCanvas;

            AZ::IO::FixedMaxPath resolvedProjectRoot;
            AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedProjectRoot, "@projectroot@");

            AZStd::string errorMessage;

            const QStringList nameFilters = { "ScriptEvent Files Saved from the ScriptCanvas Editor (*.scriptevents)" };
            QFileDialog dialog(nullptr, QObject::tr("Open a single file..."), resolvedProjectRoot.c_str());
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
                    auto deserializeResult = Deserialize(loadOutcome.GetValue().GetScriptCanvasSerializationData(), MakeInternalGraphEntitiesUnique::Yes);
                    if (deserializeResult.m_isSuccessful)
                    {

                        // success
                        return { SourceHandle::FromRelativePath(deserializeResult.m_graphDataPtr, path.Filename()), "" };
                    }
                    else
                    {
                        errorMessage = "failed to deserialize script canvas source data";
                    }
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

            // failure
            return { SourceHandle(), errorMessage };
        }

        AZStd::pair<bool, AZStd::vector<AZStd::string>> ParseAsAction(const ScriptCanvas::SourceHandle& sourceHandle)
        {
            using namespace ScriptCanvas::ScriptEventGrammar;

            if (sourceHandle.IsGraphValid())
            {
                GraphToScriptEventsResult result = ParseScriptEventsDefinition(*sourceHandle.Get());
                return { result.m_isScriptEvents, result.m_parseErrors };
            }
            else
            {
                return { false, { "no valid graph supplied" } };
            }
        }

        AZStd::pair<bool, AZStd::string> SaveAsAction([[maybe_unused]] const ScriptCanvas::SourceHandle& sourceHandle)
        {
            using namespace ScriptCanvas::ScriptEventGrammar;

            AZStd::string errorMessage;
            
            if (sourceHandle.Get())
            {
                GraphToScriptEventsResult result = ParseScriptEventsDefinition(*sourceHandle.Get());

                if (result.m_isScriptEvents)
                {
                    sourceHandle.Mod()->MarkScriptEventExtension();

                    AZ::IO::FixedMaxPath resolvedProjectRoot;
                    AZ::IO::FileIOBase::GetInstance()->ResolvePath(resolvedProjectRoot, "@projectroot@");
                    const auto fileName = QFileDialog::getSaveFileName
                        ( nullptr, QObject::tr("Save As..."), resolvedProjectRoot.c_str(), QObject::tr("All ScriptEvent Files (*.scriptevents)"));

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

                        if (saveOutcome.IsSuccess())
                        {
                            return { true, path.Filename().FixedMaxPathString().c_str() };
                        }
                        else
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
                }
            }
                            
            return { false, errorMessage };
        }

        MenuItemsEnabled UpdateMenuItemsEnabled([[maybe_unused]] const ScriptCanvas::SourceHandle& sourceHandle)
        {
            using namespace ScriptCanvas;

            auto graph = sourceHandle.Mod();
            MenuItemsEnabled menuItemsEnabled;
            bool isParsed = graph && ScriptEventGrammar::ParseMinimumScriptEventArtifacts(*graph).m_isScriptEvents;
            menuItemsEnabled.m_addHelpers = graph && !isParsed;
            menuItemsEnabled.m_clear = graph && graph->IsScriptEventExtension();
            menuItemsEnabled.m_parse = graph != nullptr;
            menuItemsEnabled.m_save = isParsed;
            return menuItemsEnabled;
        }
    }
}
