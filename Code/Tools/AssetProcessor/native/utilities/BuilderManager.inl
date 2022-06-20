/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/StringFunc/StringFunc.h>

namespace AssetProcessor
{
    //! Sends the job over to the builder and blocks until the response is received or the builder crashes/times out
    template<typename TNetRequest, typename TNetResponse, typename TRequest, typename TResponse>
    BuilderRunJobOutcome Builder::RunJob(const TRequest& request, TResponse& response, AZ::u32 processTimeoutLimitInSeconds, const AZStd::string& task, const AZStd::string& modulePath, AssetBuilderSDK::JobCancelListener* jobCancelListener /*= nullptr*/, AZStd::string tempFolderPath /*= AZStd::string()*/) const
    {
        TNetRequest netRequest;
        TNetResponse netResponse;
        netRequest.m_request = request;

        struct BuildTracker final
        {
            BuildTracker(const Builder& builder, const AZStd::string& sourceFile, const AZStd::string& task)
                : m_builder(builder)
                , m_sourceFile(sourceFile)
                , m_task(task)
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Request started builder [%s] task (%s) %s \n",
                    m_builder.UuidString().c_str(), m_task.c_str(), m_sourceFile.c_str());
            }

            ~BuildTracker()
            {
                AZ_TracePrintf(AssetProcessor::ConsoleChannel, "Request stopped builder [%s] task (%s) %s \n",
                    m_builder.UuidString().c_str(), m_task.c_str(), m_sourceFile.c_str());
            }

            const Builder& m_builder;
            const AZStd::string& m_sourceFile;
            const AZStd::string& m_task;
        };
        BuildTracker tracker(*this, request.m_sourceFile, task);

        [[maybe_unused]] AZ::u32 type;
        QByteArray data;
        AZStd::binary_semaphore wait;

        unsigned int serial;
        AssetProcessor::ConnectionBus::EventResult(serial, m_connectionId, &AssetProcessor::ConnectionBusTraits::SendRequest, netRequest, [&](AZ::u32 msgType, QByteArray msgData)
        {
            type = msgType;
            data = msgData;
            wait.release();
        });

        BuilderRunJobOutcome result = WaitForBuilderResponse(jobCancelListener, processTimeoutLimitInSeconds, &wait);

        if (result != BuilderRunJobOutcome::Ok)
        {
            // Clear out the response handler so it doesn't get triggered after the variables go out of scope (also to clean up the memory)
            AssetProcessor::ConnectionBus::Event(m_connectionId, &AssetProcessor::ConnectionBusTraits::RemoveResponseHandler, serial);
            return result;
        }

        AZ_Assert(type == netRequest.GetMessageType(), "Response type does not match");

        if (!AZ::Utils::LoadObjectFromBufferInPlace(data.data(), data.length(), netResponse))
        {
            AZ_Error("Builder", false, "Failed to deserialize processJobs response");
            return BuilderRunJobOutcome::FailedToDecodeResponse;
        }

        if (!netResponse.m_response.Succeeded() || s_createRequestFileForSuccessfulJob)
        {
            // we write the request out to disk for failure or debugging
            if (!DebugWriteRequestFile(tempFolderPath.c_str(), request, task, modulePath))
            {
                return BuilderRunJobOutcome::FailedToWriteDebugRequest;
            }
        }

        response = AZStd::move(netResponse.m_response);

        return result;
    }

    template<typename TRequest>
    bool Builder::DebugWriteRequestFile(QString tempFolderPath, const TRequest& request, const AZStd::string& task, const AZStd::string& modulePath) const
    {
        if (tempFolderPath.isEmpty())
        {
            if (!AssetUtilities::CreateTempWorkspace(tempFolderPath))
            {
                AZ_Error("Builder", false, "Failed to create temporary workspace to execute builder task");
                return false;
            }
        }
        const QDir tempFolder = QDir(tempFolderPath);
        const AZStd::string jobRequestFile = tempFolder.filePath("request.xml").toStdString().c_str();
        const AZStd::string jobResponseFile = tempFolder.filePath("response.xml").toStdString().c_str();

        if (!AZ::Utils::SaveObjectToFile(jobRequestFile, AZ::DataStream::ST_XML, &request))
        {
            AZ_Error("Builder", false, "Failed to save request to file: %s", jobRequestFile.c_str());
            return false;
        }

        auto params = BuildParams(task.c_str(), modulePath.c_str(), "", jobRequestFile, jobResponseFile, false);
        AZStd::string paramString;
        AZ::StringFunc::Join(paramString, params.begin(), params.end(), " ");

        AZ_TracePrintf(AssetProcessor::DebugChannel, "Job request written to %s\n", jobRequestFile.c_str());
        AZ_TracePrintf(AssetProcessor::DebugChannel, "To re-run this request manually, run AssetBuilder with the following parameters:\n");
        AZ_TracePrintf(AssetProcessor::DebugChannel, "%s\n", paramString.c_str());

        return true;
    }
} // namespace AssetProcessor
