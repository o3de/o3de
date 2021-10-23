/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/Components/LocalPredictionPlayerInputComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzNetworking/Serialization/HashSerializer.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/Serialization/StringifySerializer.h>
#include <AzNetworking/Serialization/TrackChangedSerializer.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>

namespace Multiplayer
{
    AZ_CVAR(AZ::TimeMs, cl_InputRateMs, AZ::TimeMs{ 33 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Rate at which to sample and process client inputs");
    AZ_CVAR(AZ::TimeMs, cl_MaxRewindHistoryMs, AZ::TimeMs{ 2000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum number of milliseconds to keep for server correction rewind and replay");
#ifndef AZ_RELEASE_BUILD
    AZ_CVAR(float, cl_DebugHackTimeMultiplier, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Scalar value used to simulate clock hacking cheats for validating bank time system and anticheat");
    AZ_CVAR(bool, cl_EnableDesyncDebugging, true, nullptr, AZ::ConsoleFunctorFlags::Null, "If enabled, debug logs will contain verbose information on detected state desyncs");
    AZ_CVAR(uint32_t, cl_PredictiveStateHistorySize, 120, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls how many inputs of predictive state should be retained for debugging desyncs");
#endif

    AZ_CVAR(bool, sv_ForceCorrections, false, nullptr, AZ::ConsoleFunctorFlags::Null, "If enabled, the server will force a correction for every input received for debugging");
    AZ_CVAR(bool, sv_EnableCorrections, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables server corrections on autonomous proxy desyncs");
    AZ_CVAR(double, sv_MaxBankTimeWindowSec, 0.2, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum bank time we allow before we start rejecting autonomous proxy move inputs due to anticheat kicking in");
    AZ_CVAR(double, sv_BankTimeDecay, 0.025, nullptr, AZ::ConsoleFunctorFlags::Null, "Amount to decay bank time by, in case of more permanent shifts in client latency");
    AZ_CVAR(AZ::TimeMs, sv_MinCorrectionTimeMs, AZ::TimeMs{ 100 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum time to wait between sending out corrections in order to avoid flooding corrections on high-latency connections");
    AZ_CVAR(AZ::TimeMs, sv_InputUpdateTimeMs, AZ::TimeMs{ 5 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum time between component updates");

    void PrintCorrectionDifferences(const AzNetworking::StringifySerializer& client, const AzNetworking::StringifySerializer& server)
    {
        const auto& clientMap = client.GetValueMap();
        const auto& serverMap = server.GetValueMap();

        AzNetworking::StringifySerializer::ValueMap differences = clientMap;
        for (auto iter = server.GetValueMap().begin(); iter != server.GetValueMap().end(); ++iter)
        {
            if (iter->second == differences[iter->first])
            {
                differences.erase(iter->first);
            }
        }

        if (differences.empty())
        {
            AZLOG_ERROR("The hash mismatched, but no differences were found.")
        }

        for (auto iter = differences.begin(); iter != differences.end(); ++iter)
        {
            auto clientValueIter = clientMap.find(iter->first);
            auto serverValueIter = serverMap.find(iter->first);
            if (clientValueIter == clientMap.end() || serverValueIter == serverMap.end())
            {
                AZLOG_ERROR("    %s (Not found in server and/or client value map!)", iter->first.c_str());
                continue;
            }

            AZLOG_ERROR("    %s Server=%s Client=%s", iter->first.c_str(), serverValueIter->second.c_str(), clientValueIter->second.c_str());
        }
    }

    inline double ConvertTimeMsToSeconds(AZ::TimeMs value)
    {
        return static_cast<double>(static_cast<AZ::TimeMs>(value)) / 1000.0;
    }

    void LocalPredictionPlayerInputComponent::LocalPredictionPlayerInputComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<LocalPredictionPlayerInputComponent, LocalPredictionPlayerInputComponentBase>()
                ->Version(1);
        }
        LocalPredictionPlayerInputComponentBase::Reflect(context);
    }

    void LocalPredictionPlayerInputComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        LocalPredictionPlayerInputComponentBase::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("MultiplayerInputDriver"));
    }

    void LocalPredictionPlayerInputComponent::OnInit()
    {
        ;
    }

    void LocalPredictionPlayerInputComponent::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void LocalPredictionPlayerInputComponent::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    LocalPredictionPlayerInputComponentController::LocalPredictionPlayerInputComponentController(LocalPredictionPlayerInputComponent& parent)
        : LocalPredictionPlayerInputComponentControllerBase(parent)
        , m_autonomousUpdateEvent([this]() { UpdateAutonomous(m_autonomousUpdateEvent.TimeInQueueMs()); }, AZ::Name("AutonomousUpdate Event"))
        , m_updateBankedTimeEvent([this]() { UpdateBankedTime(m_updateBankedTimeEvent.TimeInQueueMs()); }, AZ::Name("BankTimeUpdate Event"))
        , m_migrateStartHandler([this](ClientInputId migratedInputId) { OnMigrateStart(migratedInputId); })
        , m_migrateEndHandler([this]() { OnMigrateEnd(); })
    {
        ;
    }

    void LocalPredictionPlayerInputComponentController::OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        if (entityIsMigrating == EntityIsMigrating::True)
        {
            m_allowMigrateClientInput = true;
            m_serverMigrateFrameId = GetNetworkTime()->GetHostFrameId();
        }

        if (IsAutonomous())
        {
            m_autonomousUpdateEvent.Enqueue(AZ::TimeMs{ 1 }, true);
            GetMultiplayer()->AddClientMigrationStartEventHandler(m_migrateStartHandler);
            GetMultiplayer()->AddClientMigrationEndEventHandler(m_migrateEndHandler);
        }
    }

    void LocalPredictionPlayerInputComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
        ;
    }

    void LocalPredictionPlayerInputComponentController::HandleSendClientInput
    (
        AzNetworking::IConnection* invokingConnection, 
        const Multiplayer::NetworkInputArray& inputArray,
        const AZ::HashValue32& stateHash
    )
    {
        if (invokingConnection == nullptr)
        {
            // Discard any input messages that were locally dispatched or sent by disconnected clients
            return;
        }

        const ClientInputId clientInputId = inputArray[0].GetClientInputId();
        if (clientInputId <= m_lastClientInputId)
        {
            AZLOG(NET_Prediction, "Discarding old or out of order move input (current: %u, received %u)",
                aznumeric_cast<uint32_t>(m_lastClientInputId), aznumeric_cast<uint32_t>(clientInputId));
            return;
        }

        // After receiving the first input from the client, start the update event to check for slow hacking
        if (!m_updateBankedTimeEvent.IsScheduled())
        {
            m_updateBankedTimeEvent.Enqueue(sv_InputUpdateTimeMs, true);
        }

        const AZ::TimeMs currentTimeMs = AZ::GetElapsedTimeMs();
        const double clientInputRateSec = ConvertTimeMsToSeconds(cl_InputRateMs);
        m_lastInputReceivedTimeMs = currentTimeMs;

        // Keep track of last inputs received, also allows us to update frame ids
        m_lastInputReceived = inputArray;
        SetLastInputId(m_lastInputReceived[0].GetClientInputId()); // Set this variable in case of migration

        while (m_lastClientInputId < clientInputId)
        {
            ++m_lastClientInputId;

            // Figure out which index from the input array we want
            // If we have skipped an id, check if it was sent to us in the array. If we have lost too many, just use the oldest one in the array
            const uint32_t deltaFrameId = aznumeric_cast<uint32_t>(clientInputId - m_lastClientInputId); // always >= 0 because of while loop check
            const uint32_t inputArrayIdx = AZStd::min(deltaFrameId, NetworkInputArray::MaxElements - 1);
            const bool     lostInput = deltaFrameId >= NetworkInputArray::MaxElements; // For logging only

            NetworkInput &input = m_lastInputReceived[inputArrayIdx];
            input.SetClientInputId(m_lastClientInputId);

            // Anticheat, if we're receiving too many inputs, and fall outside our variable latency input window
            // Discard move input events, client may be speed hacking
            if (m_clientBankedTime < sv_MaxBankTimeWindowSec)
            {
                m_clientBankedTime = AZStd::min(m_clientBankedTime + clientInputRateSec, (double)sv_MaxBankTimeWindowSec); // clamp to boundary
                {
                    ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), input.GetHostBlendFactor(), invokingConnection->GetConnectionId());
                    GetNetBindComponent()->ProcessInput(input, static_cast<float>(clientInputRateSec));
                }

                if (lostInput)
                {
                    AZLOG(NET_Prediction, "InputLost InputId=%u", aznumeric_cast<uint32_t>(input.GetClientInputId()));
                }
                else
                {
                    AZLOG(NET_Prediction, "Processed InputId=%u", aznumeric_cast<uint32_t>(input.GetClientInputId()));
                }
            }
            else
            {
                AZLOG(NET_Prediction, "Dropped InputId=%u", aznumeric_cast<uint32_t>(input.GetClientInputId()));
            }
        }

        if (sv_ForceCorrections || (sv_EnableCorrections && (currentTimeMs - m_lastCorrectionSentTimeMs > sv_MinCorrectionTimeMs)))
        {
            m_lastCorrectionSentTimeMs = currentTimeMs;

            AzNetworking::HashSerializer hashSerializer;
            SerializeEntityCorrection(hashSerializer);

            const AZ::HashValue32 localAuthorityHash = hashSerializer.GetHash();

            AZLOG
            (
                NET_Prediction,
                "Hash values for ProcessInput: client=%u, server=%u",
                aznumeric_cast<uint32_t>(stateHash), 
                aznumeric_cast<uint32_t>(localAuthorityHash)
            );

            if (stateHash != localAuthorityHash)
            {
                // Produce correction for client
                AzNetworking::PacketEncodingBuffer correction;
                correction.Resize(correction.GetCapacity());
                AzNetworking::NetworkInputSerializer serializer(correction.GetBuffer(), static_cast<uint32_t>(correction.GetCapacity()));

                // only deserialize if we have data (for client/server profile/debug mismatches)
                if (correction.GetSize() > 0)
                {
                    SerializeEntityCorrection(serializer);
                }

                correction.Resize(serializer.GetSize());

                // Send correction
                SendClientInputCorrection(GetLastInputId(), correction);
            }
        }
    }

    void LocalPredictionPlayerInputComponentController::HandleSendMigrateClientInput
    (
        AzNetworking::IConnection* invokingConnection, 
        const Multiplayer::NetworkInputMigrationVector& inputArray
    )
    {
        if (!m_allowMigrateClientInput)
        {
            AZLOG_ERROR("Client attempting to SendMigrateClientInput message when server was not expecting it. This may be an attempt to cheat");
            return;
        }

        // We only allow the client to send this message exactly once, when the component has been migrated
        // Any further processing of these messages from the client would be exploitable
        m_allowMigrateClientInput = false;

        if (invokingConnection == nullptr)
        {
            // Discard any input migration messages that were locally dispatched or sent by disconnected clients
            return;
        }

        const double clientInputRateSec = ConvertTimeMsToSeconds(cl_InputRateMs);

        // Copy array so we can modify input ids
        NetworkInputMigrationVector inputArrayCopy = inputArray;

        for (uint32_t i = 0; i < inputArrayCopy.GetSize(); ++i)
        {
            NetworkInput& input = inputArrayCopy[i];

            ++ModifyLastInputId();
            input.SetClientInputId(GetLastInputId());

            ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), input.GetHostBlendFactor(), invokingConnection->GetConnectionId());
            GetNetBindComponent()->ProcessInput(input, static_cast<float>(clientInputRateSec));

            AZLOG(NET_Prediction, "Migrated InputId=%d", aznumeric_cast<int32_t>(input.GetClientInputId()));

            // Don't bother checking for corrections here, the next regular input will trigger any corrections if necessary
            // Also don't bother with any cheat detection here, because the input array is limited in size and at most and can only be sent once
            // So this highly constrains anything a malicious client can do
        }
    }

    void LocalPredictionPlayerInputComponentController::HandleSendClientInputCorrection
    (
        [[maybe_unused]] AzNetworking::IConnection* invokingConnection,
        const Multiplayer::ClientInputId& inputId,
        const AzNetworking::PacketEncodingBuffer& correction
    )
    {
        AZ_Assert(inputId <= m_clientInputId, "Invalid correction frame id, correction is for a move the client has not yet submitted to the server");
        if (inputId > m_clientInputId)
        {
            AZLOG_ERROR("Discarding correction for non-existent move, correction represents a move we haven't sent to the server yet");
            return;
        }

        if (inputId <= m_lastCorrectionInputId)
        {
            AZLOG(NET_Prediction, "Discarding old correction for client frame %u", aznumeric_cast<uint32_t>(inputId));
            return;
        }

        m_lastCorrectionInputId = inputId;

        // Apply the correction
        AzNetworking::TrackChangedSerializer<AzNetworking::NetworkOutputSerializer> serializer(correction.GetBuffer(), static_cast<uint32_t>(correction.GetSize()));
        SerializeEntityCorrection(serializer);
        GetNetBindComponent()->NotifyCorrection();

#ifndef AZ_RELEASE_BUILD
        if (cl_EnableDesyncDebugging)
        {
            AZLOG_INFO("** Autonomous Desync - Corrected clientInputId=%d ", aznumeric_cast<int32_t>(inputId));
            auto iter = m_predictiveStateHistory.find(inputId);
            if (iter != m_predictiveStateHistory.end())
            {
                // Read out state values
                AzNetworking::StringifySerializer serverValues;
                SerializeEntityCorrection(serverValues);
                PrintCorrectionDifferences(*iter->second, serverValues);
            }
            else
            {
                AZLOG_INFO("Received correction that is too old to diff, increase cl_PredictiveStateHistorySize");
            }
        }
#endif

        const uint32_t inputHistorySize = static_cast<uint32_t>(m_inputHistory.Size());
        const uint32_t historicalDelta = aznumeric_cast<uint32_t>(m_clientInputId - inputId); // Do not replay the move we just corrected, that was already processed by the server

        // If this correction is for a move outside our input history window, just start replaying from the oldest move we have available
        const uint32_t startReplayIndex = (inputHistorySize > historicalDelta) ? (inputHistorySize - historicalDelta) : 0;

        const double clientInputRateSec = ConvertTimeMsToSeconds(cl_InputRateMs);
        for (uint32_t replayIndex = startReplayIndex; replayIndex < inputHistorySize; ++replayIndex)
        {
            // Reprocess the input for this frame
            NetworkInput& input = m_inputHistory[replayIndex];
            ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), input.GetHostBlendFactor(), invokingConnection->GetConnectionId());
            GetNetBindComponent()->ReprocessInput(input, static_cast<float>(clientInputRateSec));

            AZLOG(NET_Prediction, "Replayed InputId=%d", aznumeric_cast<int32_t>(input.GetClientInputId()));
        }
    }

    void LocalPredictionPlayerInputComponentController::ForceEnableAutonomousUpdate()
    {
        m_autonomousUpdateEvent.Enqueue(AZ::TimeMs{ 1 }, true);
    }

    void LocalPredictionPlayerInputComponentController::ForceDisableAutonomousUpdate()
    {
        m_autonomousUpdateEvent.RemoveFromQueue();
    }

    bool LocalPredictionPlayerInputComponentController::IsMigrating() const
    {
        return m_lastMigratedInputId != ClientInputId{ 0 };
    }

    ClientInputId LocalPredictionPlayerInputComponentController::GetLastInputId() const
    {
        return m_lastClientInputId;
    }

    HostFrameId LocalPredictionPlayerInputComponentController::GetInputFrameId(const NetworkInput& input) const
    {
        // If the client has sent us an invalid server frame id
        // this is because they are in the process of migrating from one server to another
        // In this situation, use whatever the server frame id was when this component was migrated
        // This will match the closest state to what the client sees
        return (input.GetHostFrameId() == InvalidHostFrameId) ? m_serverMigrateFrameId : input.GetHostFrameId();
    }

    void LocalPredictionPlayerInputComponentController::OnMigrateStart(ClientInputId migratedInputId)
    {
        m_lastMigratedInputId = migratedInputId;
    }

    void LocalPredictionPlayerInputComponentController::OnMigrateEnd()
    {
        NetworkInputMigrationVector inputArray;

        // Roll up all inputs that the new server doesn't have and send them now
        for (AZStd::size_t i = 0; i < m_inputHistory.Size(); ++i)
        {
            NetworkInput& input = m_inputHistory[i];

            // New server already has these inputs
            if (input.GetClientInputId() <= m_lastMigratedInputId)
            {
                continue;
            }

            // Clear out the old server frame id
            // We don't know what server frame ids to use for the new server yet, but the new server will figure out how to deal with this
            input.SetHostFrameId(InvalidHostFrameId);

            // New server doesn't have these inputs
            if (!inputArray.PushBack(input))
            {
                break; // Reached capacity
            }
        }

        // Send these inputs to the server
        SendMigrateClientInput(inputArray);

        // Done migrating
        m_lastMigratedInputId = ClientInputId{ 0 };
    }

    void LocalPredictionPlayerInputComponentController::UpdateAutonomous(AZ::TimeMs deltaTimeMs)
    {
        const double deltaTime = ConvertTimeMsToSeconds(deltaTimeMs);
        const double clientInputRateSec = ConvertTimeMsToSeconds(cl_InputRateMs);
        const double maxRewindHistory = ConvertTimeMsToSeconds(cl_MaxRewindHistoryMs);

#ifndef AZ_RELEASE_BUILD
        m_moveAccumulator += deltaTime * cl_DebugHackTimeMultiplier;
#else
        m_moveAccumulator += deltaTime;
#endif

        const uint32_t maxClientInputs = clientInputRateSec > 0.0 ? static_cast<uint32_t>(maxRewindHistory / clientInputRateSec) : 0;

        IMultiplayer* multiplayer = GetMultiplayer();
        INetworkTime* networkTime = GetNetworkTime();
        while (m_moveAccumulator >= clientInputRateSec)
        {
            m_moveAccumulator -= clientInputRateSec;
            ++m_clientInputId;

            NetworkInputArray inputArray(GetEntityHandle());
            NetworkInput& input = inputArray[0];
            const float blendFactor = AZStd::min(AZStd::max(0.f, multiplayer->GetCurrentBlendFactor()), 1.0f);
            const AZ::TimeMs blendMs = AZ::TimeMs(static_cast<float>(static_cast<AZ::TimeMs>(cl_InputRateMs)) * (1.0f - blendFactor));

            input.SetClientInputId(m_clientInputId);
            input.SetHostFrameId(networkTime->GetHostFrameId());
            // Account for the client blending from previous frame to current
            input.SetHostTimeMs(multiplayer->GetCurrentHostTimeMs() - blendMs);
            input.SetHostBlendFactor(multiplayer->GetCurrentBlendFactor());

            // Allow components to form the input for this frame
            GetNetBindComponent()->CreateInput(input, static_cast<float>(clientInputRateSec));

            // Process the input for this frame
            GetNetBindComponent()->ProcessInput(input, static_cast<float>(clientInputRateSec));

            AZLOG(NET_Prediction, "Processed InputId=%d", aznumeric_cast<int32_t>(m_clientInputId));

            // Generate a hash based on the current client predicted states
            AzNetworking::HashSerializer hashSerializer;
            SerializeEntityCorrection(hashSerializer);

            // Save this input and discard move history outside our client rewind window
            m_inputHistory.PushBack(input);
            while (m_inputHistory.Size() > maxClientInputs)
            {
                m_inputHistory.PopFront();
            }

            const int64_t inputHistorySize = aznumeric_cast<int64_t>(m_inputHistory.Size());

            // Form the rest of the input array using the n most recent elements in the history buffer
            // NOTE: inputArray[0] has already been initialized hence start at i = 1
            for (int64_t i = 1; i < aznumeric_cast<int64_t>(NetworkInputArray::MaxElements); ++i)
            {
                // Clamp to oldest element if history is too small
                const int64_t historyIndex = AZStd::max<int64_t>(inputHistorySize - 1 - i, 0);
                inputArray[static_cast<uint32_t>(i)] = m_inputHistory[historyIndex];
            }

#ifndef AZ_RELEASE_BUILD
            if (cl_EnableDesyncDebugging)
            {
                StateHistoryItem inputHistory = AZStd::make_unique<AzNetworking::StringifySerializer>();
                while (m_predictiveStateHistory.size() > cl_PredictiveStateHistorySize)
                {
                    m_predictiveStateHistory.erase(m_predictiveStateHistory.begin());
                }
                SerializeEntityCorrection(*inputHistory);
                m_predictiveStateHistory.emplace(m_clientInputId, AZStd::move(inputHistory));
            }
#endif

            // Send the input to server (only when we are not migrating)
            if (!IsMigrating())
            {
                SendClientInput(inputArray, hashSerializer.GetHash());
            }
        }
    }

    bool LocalPredictionPlayerInputComponentController::SerializeEntityCorrection(AzNetworking::ISerializer& serializer)
    {
        bool result = GetNetBindComponent()->SerializeEntityCorrection(serializer);

        NetworkHierarchyRootComponent* hierarchyComponent = GetParent().GetNetworkHierarchyRootComponent();
        if (result && hierarchyComponent)
        {
            result = hierarchyComponent->SerializeEntityCorrection(serializer);
        }
        return result;
    }

    void LocalPredictionPlayerInputComponentController::UpdateBankedTime(AZ::TimeMs deltaTimeMs)
    {
        const double deltaTime = static_cast<double>(deltaTimeMs) / 1000.0;
        const double clientInputRateSec = static_cast<double>(static_cast<AZ::TimeMs>(cl_InputRateMs)) / 1000.0;

        // Update banked time accumulator
        m_clientBankedTime -= deltaTime;

        // Forcibly tick any clients who are too far behind our variable latency window
        // Client may be slow hacking
        if (m_clientBankedTime < -sv_MaxBankTimeWindowSec)
        {
            m_clientBankedTime = -sv_MaxBankTimeWindowSec; // clamp to boundary

            NetworkInput& input = m_lastInputReceived[0];
            {
                ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), DefaultBlendFactor, GetNetBindComponent()->GetOwningConnectionId());
                GetNetBindComponent()->ProcessInput(input, static_cast<float>(clientInputRateSec));
            }

            AZLOG(NET_Prediction, "Forced InputId=%d", aznumeric_cast<int32_t>(input.GetClientInputId()));
        }

        // Decay our bank time window, in case the remote endpoint has suffered a more persistent shift in latency, this should cause the client to eventually recover
        m_clientBankedTime = m_clientBankedTime * (1.0 - sv_BankTimeDecay);
    }
}
