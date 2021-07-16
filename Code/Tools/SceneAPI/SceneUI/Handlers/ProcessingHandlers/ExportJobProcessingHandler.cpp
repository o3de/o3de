/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QRegExp>
#include <AzCore/Casting/numeric_cast.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/UI/Logging/LogEntry.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneUI/Handlers/ProcessingHandlers/ExportJobProcessingHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            ExportJobProcessingHandler::ExportJobProcessingHandler(Uuid traceTag, const AZStd::string& sourceAssetPath, QObject* parent)
                : ProcessingHandler(traceTag, parent)
                , m_sourceAssetPath(sourceAssetPath)
            {
            }

            void ExportJobProcessingHandler::BeginProcessing()
            {
                m_jobWatcher.reset(new JobWatcher(m_sourceAssetPath, m_traceTag));
                connect(m_jobWatcher.get(), &JobWatcher::JobProcessingComplete, this, &ExportJobProcessingHandler::OnJobProcessingComplete);
                connect(m_jobWatcher.get(), &JobWatcher::AllJobsComplete, this, &ExportJobProcessingHandler::OnAllJobsComplete);
                m_jobWatcher->StartMonitoring();

                emit StatusMessageUpdated("File processing...");
            }
            
            void ExportJobProcessingHandler::OnJobQueryFailed([[maybe_unused]] const char* message)
            {
                emit StatusMessageUpdated("Processing failed.");
                AZ_TracePrintf(Utilities::ErrorWindow, "%s", message);
                emit ProcessingComplete();
            }

            void ExportJobProcessingHandler::OnJobProcessingComplete(const AZStd::string& platform, [[maybe_unused]] AZ::u64 jobId, bool success, const AZStd::string& fullLogText)
            {
                AZ_TraceContext("Platform", platform);

                if (!fullLogText.empty())
                {
                    bool parseResult = AzToolsFramework::Logging::LogEntry::ParseLog(fullLogText.c_str(), aznumeric_cast<AZ::u64>(fullLogText.length()),
                        [this](const AzToolsFramework::Logging::LogEntry& entry)
                        {
                            emit AddLogEntry(entry);
                        });

                    if (!parseResult)
                    {
                        AZ_TracePrintf(Utilities::ErrorWindow, "Failed to parse log. See Asset Processor for more info.");
                    }
                }

                if (success)
                {
                    AZ_TracePrintf(Utilities::SuccessWindow, "Job #%i compiled successfully", jobId);
                }
                else
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Job #%i failed", jobId);
                }
            }
            
            void ExportJobProcessingHandler::OnAllJobsComplete()
            {
                emit StatusMessageUpdated("All jobs completed.");
                emit ProcessingComplete();
            }
        }
    }
}

#include <Handlers/ProcessingHandlers/moc_ExportJobProcessingHandler.cpp>
