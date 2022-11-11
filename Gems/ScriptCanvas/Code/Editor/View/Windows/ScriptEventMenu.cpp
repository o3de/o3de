/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
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

        AZ::IO::Path GetOpenFilePath(const AZ::IO::Path& sourceFilePath)
        {
            if (sourceFilePath.empty())
            {
                AZ::IO::FixedMaxPathString resolvedProjectRoot = AZ::Utils::GetProjectPath();
                AZStd::string errorMessage;

                const QStringList nameFilters = { "ScriptEvent Files Saved from the ScriptCanvas Editor (*.scriptevents)" };
                QFileDialog dialog(nullptr, QObject::tr("Open a single file..."), resolvedProjectRoot.c_str());
                dialog.setFileMode(QFileDialog::ExistingFiles);
                dialog.setNameFilters(nameFilters);

                if (dialog.exec() == QDialog::Accepted && dialog.selectedFiles().count() >= 1)
                {
                    return AZ::IO::Path(dialog.selectedFiles().begin()->toUtf8().constData());
                }
            }
            return sourceFilePath;
        }

        AZ::IO::Path GetSaveFilePath(const ScriptCanvas::SourceHandle& sourceHandle, bool saveInPlace)
        {
            AZ::IO::Path saveFilePath;
            if (saveInPlace)
            {
                saveFilePath = sourceHandle.AbsolutePath();
            }
            else
            {
                AZ::IO::FixedMaxPathString resolvedProjectRoot = AZ::Utils::GetProjectPath();
                const auto saveAsFilePath = QFileDialog::getSaveFileName(
                    nullptr, QObject::tr("Save As..."), resolvedProjectRoot.c_str(), QObject::tr("All ScriptEvent Files (*.scriptevents)"));
                if (!saveAsFilePath.toUtf8().isEmpty())
                {
                    saveFilePath = AZ::IO::Path(saveAsFilePath.toUtf8().constData());
                }
            }
            return saveFilePath;
        }

        ScriptCanvas::SourceHandle LoadScriptEventSource(const AZ::IO::Path& sourceFilePath, AZStd::string& outErrorMessage)
        {
            AZ::Outcome<ScriptEvents::ScriptEvent, AZStd::string> loadOutcome = AZ::Failure(AZStd::string());
            ScriptEventBus::BroadcastResult(loadOutcome, &ScriptEvents::ScriptEventRequests::LoadDefinitionSource, sourceFilePath);

            if (loadOutcome.IsSuccess())
            {
                auto deserializeResult = ScriptCanvas::Deserialize(
                    loadOutcome.GetValue().GetScriptCanvasSerializationData(), ScriptCanvas::MakeInternalGraphEntitiesUnique::Yes);
                if (deserializeResult.m_isSuccessful)
                {
                    auto result = ScriptCanvas::SourceHandle::FromRelativePath(deserializeResult.m_graphDataPtr, sourceFilePath.Filename());
                    return ScriptCanvas::SourceHandle::MarkAbsolutePath(result, sourceFilePath.Native());
                }
                else
                {
                    outErrorMessage = deserializeResult.m_errors;
                }
            }
            else
            {
                outErrorMessage = loadOutcome.GetError();
            }
            return ScriptCanvas::SourceHandle();
        }

        bool MakeHelpersAction(const ScriptCanvas::SourceHandle& sourceHandle)
        {
            if (!sourceHandle.Mod())
            {
                return false;
            }

            sourceHandle.Mod()->MarkScriptEventExtension();
            AddScriptEventHelpers(*sourceHandle.Mod());
            return true;
        }

        ScriptCanvas::SourceHandle OpenAction(const AZ::IO::Path& sourceFilePath)
        {
            ScriptCanvas::SourceHandle result;
            AZ::IO::Path filePath = GetOpenFilePath(sourceFilePath);
            if (filePath.empty())
            {
                return result;
            }

            AZStd::string errorMessage;
            result = LoadScriptEventSource(filePath, errorMessage);
            if (!result.IsGraphValid())
            {
                QMessageBox messageBox(QMessageBox::Warning, QObject::tr("Failed to open ScriptEvent file into ScriptCanvas Editor."),
                    errorMessage.c_str(), QMessageBox::Close, nullptr);
                messageBox.exec();
            }
            return result;
        }

        ScriptCanvas::SourceHandle SaveScriptEventSource(
            const ScriptCanvas::SourceHandle& sourceHandle, const AZ::IO::Path& saveFilePath, AZStd::string& outErrorMessage)
        {
            using namespace ScriptCanvas::ScriptEventGrammar;
            GraphToScriptEventsResult parsingResult = ParseScriptEventsDefinition(*sourceHandle.Get());
            if (parsingResult.m_isScriptEvents)
            {
                // use the ScriptEventBus, also to get fundamental types(?)
                AZ::Outcome<void, AZStd::string> saveOutcome = AZ::Failure(AZStd::string("failed to save"));
                ScriptEvents::ScriptEventBus::BroadcastResult(
                    saveOutcome, &ScriptEvents::ScriptEventRequests::SaveDefinitionSourceFile, parsingResult.m_event, saveFilePath);

                if (saveOutcome.IsSuccess())
                {
                    auto result = ScriptCanvas::SourceHandle::FromRelativePath(sourceHandle.Data(), saveFilePath.Filename());
                    return ScriptCanvas::SourceHandle::MarkAbsolutePath(result, saveFilePath);
                }
                else
                {
                    outErrorMessage = saveOutcome.TakeError();
                }
            }
            else
            {
                outErrorMessage = "Changes are required to properly parse graph as ScriptEvents file.";
            }
            return ScriptCanvas::SourceHandle();
        }

        ScriptCanvas::SourceHandle SaveAction(const ScriptCanvas::SourceHandle& sourceHandle, bool saveInPlace)
        {
            using namespace ScriptCanvas::ScriptEventGrammar;

            ScriptCanvas::SourceHandle result;
            AZStd::string errorMessage = "Invalid ScriptEvent graph.";

            if (sourceHandle.Get())
            {
                auto saveFilePath = GetSaveFilePath(sourceHandle, saveInPlace);
                if (saveFilePath.empty())
                {
                    return result;
                }
                result = SaveScriptEventSource(sourceHandle, saveFilePath, errorMessage);
            }

            if (!result.IsGraphValid())
            {
                QMessageBox messageBox(QMessageBox::Warning, QObject::tr("Failed to Save ScriptEvent"),
                    errorMessage.c_str(), QMessageBox::Close, nullptr);
                messageBox.exec();
            }
            return result;
        }

        MenuItemsEnabled UpdateMenuItemsEnabled([[maybe_unused]] const ScriptCanvas::SourceHandle& sourceHandle)
        {
            using namespace ScriptCanvas;

            auto graph = sourceHandle.Mod();
            MenuItemsEnabled menuItemsEnabled;
            bool isScriptEventGraph = graph && ScriptEventGrammar::ParseMinimumScriptEventArtifacts(*graph).m_isScriptEvents;
            menuItemsEnabled.m_addHelpers = graph && !isScriptEventGraph;
            menuItemsEnabled.m_clear = graph && graph->IsScriptEventExtension();
            menuItemsEnabled.m_save = isScriptEventGraph;
            menuItemsEnabled.m_saveAs = isScriptEventGraph;
            return menuItemsEnabled;
        }
    }
}
