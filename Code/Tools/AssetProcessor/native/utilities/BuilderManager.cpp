/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <utilities/BuilderManager.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <native/connection/connectionManager.h>
#include <native/connection/connection.h>
#include <QCoreApplication>
#include <AssetBuilder/AssetBuilderStatic.h>

namespace AssetProcessor
{
    //! Time in milliseconds to wait after each message pump cycle
    constexpr int IdleBuilderPumpingDelayMs = 100;

    BuilderManager::BuilderManager(ConnectionManager* connectionManager)
    {
        using namespace AZStd::placeholders;
        connectionManager->RegisterService(AssetBuilder::BuilderHelloRequest::MessageType(), AZStd::bind(&BuilderManager::IncomingBuilderPing, this, _1, _2, _3, _4, _5));

        // Setup a background thread to pump the idle builders so they don't get blocked trying to output to stdout/err
        AZStd::thread_desc desc;
        desc.m_name = "BuilderManager Idle Pump";
        m_pollingThread = AZStd::thread(desc, [this]()
            {
                while (!m_quitListener.WasQuitRequested())
                {
                    PumpIdleBuilders();
                    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(IdleBuilderPumpingDelayMs));
                }
            });

        m_quitListener.BusConnect();
        BusConnect();
    }

    BuilderManager::~BuilderManager()
    {
        PrintDebugOutput();

        BusDisconnect();
        m_quitListener.BusDisconnect();
        m_quitListener.ApplicationShutdownRequested();

        if (m_pollingThread.joinable())
        {
            m_pollingThread.join();
        }
    }

    void BuilderManager::ConnectionLost(AZ::u32 connId)
    {
        AZ_Assert(connId > 0, "ConnectionId was 0");
        AZStd::lock_guard<AZStd::mutex> lock(m_buildersMutex);

        if(auto uuidString = m_builderList.RemoveByConnectionId(connId); !uuidString.empty())
        {
            AZ_TracePrintf("BuilderManager", "Lost connection to builder %s\n", uuidString.c_str());
        }
    }

    void BuilderManager::IncomingBuilderPing(AZ::u32 connId, AZ::u32 /*type*/, AZ::u32 serial, QByteArray payload, QString platform)
    {
        AssetBuilder::BuilderHelloRequest requestPing;
        AssetBuilder::BuilderHelloResponse responsePing;

        if (!AZ::Utils::LoadObjectFromBufferInPlace(payload.data(), payload.length(), requestPing))
        {
            AZ_Error("BuilderManager", false,
                "Failed to deserialize BuilderHelloRequest.\n"
                "Your builder(s) may need recompilation to function correctly as this kind of failure usually indicates that "
                "there is a disparity between the version of asset processor running and the version of builder dll files present in the "
                "'builders' subfolder.");
        }
        else
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_buildersMutex);

            auto builder = m_builderList.Find(requestPing.m_uuid);

            if (!builder)
            {
                if (m_allowUnmanagedBuilderConnections)
                {
                    AZ_TracePrintf("BuilderManager", "External builder connection accepted for ProcessJob work\n");
                    builder = AddNewBuilder(BuilderPurpose::ProcessJob); // We only accept external connections for ProcessJob builders
                }
                else
                {
                    AZ_Warning(
                        "BuilderManager",
                        false,
                        "Received request ping from builder but could not match uuid %s to list of builders started by this AssetProcessor instance.  "
                        "If you intended to connect an external builder, please set BuilderManager::m_allowUnmanagedBuilderConnections to true to allow this.",
                        requestPing.m_uuid.ToString<AZStd::string>().c_str());
                }
            }

            if (builder)
            {
                if (builder->IsConnected())
                {
                    AZ_Error("BuilderManager", false, "Builder %s is already connected and should not be sending another ping.  Something has gone wrong.  There may be multiple builders with the same UUID", builder->UuidString().c_str());
                }
                else
                {
                    AZ_TracePrintf("BuilderManager", "Builder %s connected, connId: %d\n", builder->UuidString().c_str(), connId);
                    builder->SetConnection(connId);
                    responsePing.m_accepted = true;
                    responsePing.m_uuid = builder->GetUuid();
                }
            }
        }

        AssetProcessor::ConnectionBus::Event(connId, &AssetProcessor::ConnectionBusTraits::SendResponse, serial, responsePing);
    }

    AZStd::shared_ptr<Builder> BuilderManager::AddNewBuilder(BuilderPurpose purpose)
    {
        AZ::Uuid builderUuid;
        // Make sure that we don't already have a builder with the same UUID.  If we do, try generating another one
        constexpr int MaxRetryCount = 10;
        int retriesRemaining = MaxRetryCount;

        do
        {
            builderUuid = AZ::Uuid::CreateRandom();
            --retriesRemaining;
        } while (m_builderList.Find(builderUuid) && retriesRemaining > 0);

        if(m_builderList.Find(builderUuid))
        {
            AZ_Error("BuilderManager", false, "Failed to generate a unique id for new builder after %d attempts.  All attempted random ids were already taken.", MaxRetryCount);
            return {};
        }

        auto builder = AZStd::make_shared<Builder>(m_quitListener, builderUuid);

        m_builderList.AddBuilder(builder, purpose);

        return builder;
    }

    void BuilderManager::AddAssetToBuilderProcessedList(const AZ::Uuid& builderId, const AZStd::string& sourceAsset)
    {
        m_builderDebugOutput[builderId].m_assetsProcessed.push_back(sourceAsset);
    }

    BuilderRef BuilderManager::GetBuilder(BuilderPurpose purpose)
    {
        AZStd::shared_ptr<Builder> newBuilder;
        BuilderRef builderRef;
        if (m_quitListener.WasQuitRequested())
        {
            // don't hand out new builders if we're quitting.
            return builderRef;
        }

        // the below scope is intentional, to contain the scoped lock.
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_buildersMutex);

            if (purpose != BuilderPurpose::Registration)
            {
                auto builder = m_builderList.GetFirst(purpose);

                if (builder)
                {
                    return builder;
                }
            }

            AZ_TracePrintf("BuilderManager", "Starting new builder for job request\n");

            // None found, start up a new one
            newBuilder = AddNewBuilder(purpose);

            // Grab a reference so no one else can take it while we're outside the lock
            builderRef = BuilderRef(newBuilder);
        }

        AZ::Outcome<void, AZStd::string> builderStartResult = newBuilder->Start(purpose);

        if (!builderStartResult.IsSuccess())
        {
            AZ_Error("BuilderManager", false, "Builder failed to start with error %.*s", AZ_STRING_ARG(builderStartResult.GetError()));

            AZStd::unique_lock<AZStd::mutex> lock(m_buildersMutex);

            builderRef = {}; // Release after the lock to make sure no one grabs it before we can delete it

            m_builderList.RemoveByUuid(newBuilder->GetUuid());
        }
        else
        {
            AZ_TracePrintf("BuilderManager", "Builder started successfully\n");
        }

        return builderRef;
    }

    void BuilderManager::PumpIdleBuilders()
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_buildersMutex);

        m_builderList.PumpIdleBuilders();
    }

    void BuilderManager::PrintDebugOutput()
    {
        // If debug output was tracked, print it on shutdown.
        // This prints each asset that was processed by each builder, in the order they were processed.
        // This is useful for tracing issues like memory leaks across assets processed by the same builder.
        for (auto builderInfo : m_builderDebugOutput)
        {
            AZ_TracePrintf("BuilderManager", "Builder %.*s processed these assets:\n",
                AZ_STRING_ARG(builderInfo.first.ToString<AZStd::string>()));
            for (auto asset : builderInfo.second.m_assetsProcessed)
            {
                AZ_TracePrintf(
                    "BuilderManager", "Builder with ID %.*s processed %.*s\n",
                    AZ_STRING_ARG(builderInfo.first.ToFixedString()),
                    AZ_STRING_ARG(asset));
            }
        }
    }

} // namespace AssetProcessor
