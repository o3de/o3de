/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/FileIOEventBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <Editor/View/Windows/Tools/UpgradeTool/FileSaver.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>

namespace FileSaverCpp
{
    class FileEventHandler
        : public AZ::IO::FileIOEventBus::Handler
    {
    public:
        int m_errorCode = 0;
        AZStd::string m_fileName;

        FileEventHandler()
        {
            BusConnect();
        }

        ~FileEventHandler()
        {
            BusDisconnect();
        }

        void OnError(const AZ::IO::SystemFile* /*file*/, const char* fileName, int errorCode) override
        {
            m_errorCode = errorCode;

            if (fileName)
            {
                m_fileName = fileName;
            }
        }
    };
}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        FileSaver::FileSaver
            ( AZStd::function<bool()> onReadOnlyFile
            , AZStd::function<void(const FileSaveResult& result)> onComplete)
            : m_onReadOnlyFile(onReadOnlyFile)
            , m_onComplete(onComplete)
        {}

        const SourceHandle& FileSaver::GetSource() const
        {
            return m_source;
        }

        void FileSaver::PerformMove
            ( AZStd::string tmpFileName
            , AZStd::string target
            , size_t remainingAttempts)
        {
            FileSaverCpp::FileEventHandler fileEventHandler;

            if (remainingAttempts == 0)
            {
                AZ::SystemTickBus::QueueFunction([this, tmpFileName]()
                    {
                        FileSaveResult result;
                        result.fileSaveError = "Failed to move updated file from temporary location to original destination.";
                        result.tempFileRemovalError = RemoveTempFile(tmpFileName);
                        m_onComplete(result);
                    });
            }
            else if (remainingAttempts == 2)
            {
                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                // before the last attempt, flush all the caches
                AZ::IO::FileRequestPtr flushRequest = streamer->FlushCaches();
                streamer->SetRequestCompleteCallback(flushRequest
                    , [this, remainingAttempts, tmpFileName, target]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                    {
                        // One last try
                        AZ::SystemTickBus::QueueFunction(
                            [this, remainingAttempts, tmpFileName, target]() { PerformMove(tmpFileName, target, remainingAttempts - 1); });
                    });
                streamer->QueueRequest(flushRequest);
            }
            else
            {
                // the actual move attempt
                auto moveResult = AZ::IO::SmartMove(tmpFileName.c_str(), target.c_str());
                if (moveResult.GetResultCode() == AZ::IO::ResultCode::Success)
                {
                    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                    AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
                    // Bump the slice asset up in the asset processor's queue.
                    AzFramework::AssetSystemRequestBus::Broadcast
                        (&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, target.c_str());
                    
                    AZ::SystemTickBus::QueueFunction([this, tmpFileName]()
                        {
                            FileSaveResult result;
                            result.tempFileRemovalError = RemoveTempFile(tmpFileName);
                            m_onComplete(result);
                        });
                }
                else
                {
                    AZ_Warning(ScriptCanvas::k_VersionExplorerWindow.data(), false
                        , "moving converted file to tmpFileName destination failed: %s, trying again", target.c_str());
                    auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                    AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(target.c_str());
                    streamer->SetRequestCompleteCallback(flushRequest
                    , [this, tmpFileName, target, remainingAttempts]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                    {
                        // Continue saving.
                        AZ::SystemTickBus::QueueFunction(
                            [this, tmpFileName, target, remainingAttempts]() { PerformMove(tmpFileName, target, remainingAttempts - 1); });
                    });
                    streamer->QueueRequest(flushRequest);
                }
            }
        }

        void FileSaver::OnSourceFileReleased(const SourceHandle& source)
        {
            AZStd::string fullPath = source.Path().c_str();
            AZStd::string tmpFileName;
            // here we are saving the graph to a temp file instead of the original file and then copying the temp file to the original file.
            // This ensures that AP will not a get a file change notification on an incomplete graph file causing it to fail processing.
            // Temp files are ignored by AP.
            if (!AZ::IO::CreateTempFileName(fullPath.c_str(), tmpFileName))
            {
                FileSaveResult result;
                result.fileSaveError = "Failure to create temporary file name";
                m_onComplete(result);
                return;
            }

            AZStd::string saveError;

            AZ::IO::FileIOStream fileStream(tmpFileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);
            if (fileStream.IsOpen())
            {
                auto saveOutcome = ScriptCanvasEditor::SaveToStream(source, fileStream);
                if (!saveOutcome.IsSuccess())
                {
                    saveError = saveOutcome.TakeError();
                }

                fileStream.Close();
            }

            if (!saveError.empty())
            {
                FileSaveResult result;
                result.fileSaveError = AZStd::string::format("Save asset data to temporary file failed: %s", saveError.c_str());
                m_onComplete(result);
                return;
            }

            AzToolsFramework::SourceControlCommandBus::Broadcast
                ( &AzToolsFramework::SourceControlCommandBus::Events::RequestEdit
                , fullPath.c_str()
                , true
                , [this, fullPath, tmpFileName]([[maybe_unused]] bool success, const AzToolsFramework::SourceControlFileInfo& info)
            {
                constexpr const size_t k_maxAttemps = 10;

                if (!info.IsReadOnly())
                {
                    PerformMove(tmpFileName, fullPath, k_maxAttemps);
                }
                else if (m_onReadOnlyFile && m_onReadOnlyFile())
                {
                    AZ::IO::SystemFile::SetWritable(info.m_filePath.c_str(), true);
                    PerformMove(tmpFileName, fullPath, k_maxAttemps);
                }
                else
                {
                    FileSaveResult result;
                    result.fileSaveError = "Source file was and remained read-only";
                    result.tempFileRemovalError = RemoveTempFile(tmpFileName);
                    m_onComplete(result);
                }
            });
        }

        AZStd::string FileSaver::RemoveTempFile(AZStd::string_view tempFile)
        {
            AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            if (!fileIO)
            {
                return "No FileIO instance";
            }

            if (fileIO->Exists(tempFile.data()) && !fileIO->Remove(tempFile.data()))
            {
                return AZStd::string::format("Failed to remove temporary file: %s", tempFile.data());
            }

            return "";
        }

        void FileSaver::Save(const SourceHandle& source)
        {
            m_source = source;

            if (source.Path().empty())
            {
                FileSaveResult result;
                result.fileSaveError = "No save location specified";
                m_onComplete(result);
            }
            else
            {
                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                AZ::IO::FileRequestPtr flushRequest = streamer->FlushCache(source.Path().c_str());
                streamer->SetRequestCompleteCallback(flushRequest, [this]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                    if (!m_sourceFileReleased)
                    {
                        m_sourceFileReleased = true;
                        AZ::SystemTickBus::QueueFunction([this]() { this->OnSourceFileReleased(m_source); });
                    }
                });

                streamer->QueueRequest(flushRequest);
            }
        }
    }
}
