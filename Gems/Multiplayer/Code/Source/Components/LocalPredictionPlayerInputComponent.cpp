/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

namespace Multiplayer
{
    AZ_CVAR(AZ::TimeMs, cl_InputRateMs, AZ::TimeMs{ 33 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Rate at which to sample and process client inputs");
    AZ_CVAR(AZ::TimeMs, cl_MaxRewindHistoryMs, AZ::TimeMs{ 2000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum number of milliseconds to keep for server correction rewind and replay");
#ifndef AZ_RELEASE_BUILD
    AZ_CVAR(float, cl_DebugHackTimeMultiplier, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Scalar value used to simulate clock hacking cheats for validating bank time system and anticheat");
    AZ_CVAR(bool, cl_EnableDesyncDebugging, false, nullptr, AZ::ConsoleFunctorFlags::Null, "If enabled, debug logs will contain verbose information on detected state desyncs");
#endif

    AZ_CVAR(bool, sv_EnableCorrections, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables server corrections on autonomous proxy desyncs");
    AZ_CVAR(double, sv_MaxBankTimeWindowSec, 0.2, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum bank time we allow before we start rejecting autonomous proxy move inputs due to anticheat kicking in");
    AZ_CVAR(double, sv_BankTimeDecay, 0.025, nullptr, AZ::ConsoleFunctorFlags::Null, "Amount to decay bank time by, in case of more permanent shifts in client latency");
    AZ_CVAR(AZ::TimeMs, sv_MinCorrectionTimeMs, AZ::TimeMs{ 100 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum time to wait between sending out corrections in order to avoid flooding corrections on high-latency connections");
    AZ_CVAR(AZ::TimeMs, sv_InputUpdateTimeMs, AZ::TimeMs{ 5 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum time between component updates");

    // Debug helper functions
    AZStd::string GetInputString(NetworkInput& input)
    {
        AzNetworking::StringifySerializer serializer(',', false);
        input.Serialize(serializer);
        return serializer.GetString();
    }

    AZStd::string GetCorrectionDataString(NetBindComponent* netBindComponent)
    {
        AzNetworking::StringifySerializer serializer(',', false);
        netBindComponent->SerializeEntityCorrection(serializer);
        return serializer.GetString();
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
            GetParent().GetNetBindComponent()->AddEntityMigrationStartEventHandler(m_migrateStartHandler);
            GetParent().GetNetBindComponent()->AddEntityMigrationEndEventHandler(m_migrateEndHandler);
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
        const AZ::HashValue32& stateHash,
        [[maybe_unused]] const AzNetworking::PacketEncodingBuffer& clientState
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
        const double clientInputRateSec = static_cast<double>(static_cast<AZ::TimeMs>(cl_InputRateMs)) / 1000.0;
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
                    ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), invokingConnection->GetConnectionId());
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

        if (sv_EnableCorrections && (currentTimeMs - m_lastCorrectionSentTimeMs > sv_MinCorrectionTimeMs))
        {
            m_lastCorrectionSentTimeMs = currentTimeMs;

            AzNetworking::HashSerializer hashSerializer;
            GetNetBindComponent()->SerializeEntityCorrection(hashSerializer);

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
                AzNetworking::NetworkInputSerializer serializer(correction.GetBuffer(), correction.GetCapacity());

                // only deserialize if we have data (for client/server profile/debug mismatches)
                if (correction.GetSize() > 0)
                {
                    GetNetBindComponent()->SerializeEntityCorrection(serializer);
                }

                correction.Resize(serializer.GetSize());

                // Send correction
                SendClientInputCorrection(GetLastInputId(), correction);

#ifndef AZ_RELEASE_BUILD
                AZStd::string clientStateString;
                AZStd::string serverStateString;
                if (cl_EnableDesyncDebugging)
                {
                    // In debug, show which states caused the correction
                    // Write in client state
                    AzNetworking::NetworkOutputSerializer clientStateSerializer(clientState.GetBuffer(), clientState.GetSize());
                    GetNetBindComponent()->SerializeEntityCorrection(clientStateSerializer);

                    // Read out state values
                    AzNetworking::StringifySerializer clientValues;
                    GetNetBindComponent()->SerializeEntityCorrection(clientValues);

                    // Restore server state
                    AzNetworking::NetworkOutputSerializer serverStateSerializer(correction.GetBuffer(), correction.GetSize());
                    GetNetBindComponent()->SerializeEntityCorrection(serverStateSerializer);

                    // Read out state values
                    AzNetworking::StringifySerializer serverValues;
                    GetNetBindComponent()->SerializeEntityCorrection(serverValues);

                    AZStd::map<AZStd::string, AZStd::pair<AZStd::string, AZStd::string>> mapComparison;

                    // put the server value in the first part of the pair
                    for (const auto& pair : serverValues.GetValueMap())
                    {
                        mapComparison[pair.first].first = pair.second;
                    }

                    // put the client value in the second part of the pair
                    for (const auto& pair : clientValues.GetValueMap())
                    {
                        mapComparison[pair.first].second = pair.second;
                    }

                    bool firstIt = true;
                    for (const auto& mapPair : mapComparison)
                    {
                        if (mapPair.second.first != mapPair.second.second)
                        {
                            if (!firstIt)
                            {
                                clientStateString += ",";
                                serverStateString += ",";
                            }
                            firstIt = false;

                            AZStd::string clientValue = mapPair.second.second.empty() ? "<no value>" : mapPair.second.second;
                            AZStd::string serverValue = mapPair.second.first.empty() ? "<no value>" : mapPair.second.first;
                            clientStateString += mapPair.first + "=" + clientValue;
                            serverStateString += mapPair.first + "=" + serverValue;
                        }
                    }
                }
                else
                {
                    clientStateString = "available in debug only";
                    serverStateString = "available in debug only";
                }
                AZLOG_ERROR("** Autonomous proxy desync detected! ** clientState=[%s], serverState=[%s]", clientStateString.c_str(), serverStateString.c_str());
#endif
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

        const float clientInputRateSec = static_cast<float>(static_cast<AZ::TimeMs>(cl_InputRateMs)) / 1000.0;

        // Copy array so we can modify input ids
        NetworkInputMigrationVector inputArrayCopy = inputArray;

        for (uint32_t i = 0; i < inputArrayCopy.GetSize(); ++i)
        {
            NetworkInput& input = inputArrayCopy[i];

            ++ModifyLastInputId();
            input.SetClientInputId(GetLastInputId());

            ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), invokingConnection->GetConnectionId());
            GetNetBindComponent()->ProcessInput(input, clientInputRateSec);

            AZLOG
            (
                NET_Prediction,
                "Migrated InputId=%d - i=[%s] o=[%s]",
                aznumeric_cast<int32_t>(input.GetClientInputId()),
                GetInputString(input).c_str(),
                GetCorrectionDataString(GetNetBindComponent()).c_str()
            );

            // Don't bother checking for corrections here, the next regular input will trigger any corrections if necessary
            // Also don't bother with any cheat detection here, because the input array is limited in size and at most and can only be sent once
            // So this highly constrains anything a malicious client can do
        }
    }

    void LocalPredictionPlayerInputComponentController::HandleSendClientInputCorrection
    (
        AzNetworking::IConnection* invokingConnection,
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
        AzNetworking::TrackChangedSerializer<AzNetworking::NetworkOutputSerializer> serializer(correction.GetBuffer(), correction.GetSize());
        GetNetBindComponent()->SerializeEntityCorrection(serializer);
        m_correctionEvent.Signal();

        AZLOG
        (
            NET_Prediction,
            "Corrected InputId=%d - o=[%s]",
            aznumeric_cast<int32_t>(m_lastCorrectionInputId),
            GetCorrectionDataString(GetNetBindComponent()).c_str()
        );

        const uint32_t inputHistorySize = m_inputHistory.Size();
        const uint32_t historicalDelta = aznumeric_cast<uint32_t>(m_clientInputId - inputId); // Do not replay the move we just corrected, that was already processed by the server

        // If this correction is for a move outside our input history window, just start replaying from the oldest move we have available
        const uint32_t startReplayIndex = (inputHistorySize > historicalDelta) ? (inputHistorySize - historicalDelta) : 0;

        // Flag that we are replaying inputs
        struct ScopedReplayingInput
        {
            ScopedReplayingInput(LocalPredictionPlayerInputComponentController* instance)
                : m_instance(instance)
            {
                m_instance->m_replayingInput = true;
            }
            ~ScopedReplayingInput()
            {
                m_instance->m_replayingInput = false;
            }
            LocalPredictionPlayerInputComponentController* m_instance;
        };
        ScopedReplayingInput markReplayingInput(this);

        const float clientInputRateSec = static_cast<float>(static_cast<AZ::TimeMs>(cl_InputRateMs)) / 1000.0;
        for (uint32_t replayIndex = startReplayIndex; replayIndex < inputHistorySize; ++replayIndex)
        {
            // Reprocess the input for this frame
            NetworkInput& input = m_inputHistory[replayIndex];
            ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), invokingConnection->GetConnectionId());
            GetNetBindComponent()->ProcessInput(input, clientInputRateSec);

            AZLOG
            (
                NET_Prediction,
                "Replayed InputId=%d - i=[%s] o=[%s]",
                aznumeric_cast<int32_t>(input.GetClientInputId()),
                GetInputString(input).c_str(),
                GetCorrectionDataString(GetNetBindComponent()).c_str()
            );
        }
    }

    bool LocalPredictionPlayerInputComponentController::IsReplayingInput() const
    {
        return m_replayingInput;
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

    void LocalPredictionPlayerInputComponentController::CorrectionEventAddHandle(CorrectionEvent::Handler& handler)
    {
        handler.Connect(m_correctionEvent);
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
        const double deltaTime = static_cast<double>(deltaTimeMs) / 1000.0;
        const double inputRate = static_cast<double>(static_cast<AZ::TimeMs>(cl_InputRateMs)) / 1000.0;
        const double maxRewindHistory = static_cast<double>(static_cast<AZ::TimeMs>(cl_MaxRewindHistoryMs)) / 1000.0;

#ifndef AZ_RELEASE_BUILD
        m_moveAccumulator += deltaTime * cl_DebugHackTimeMultiplier;
#else
        m_moveAccumulator += deltaTime;
#endif

        const uint32_t maxClientInputs = inputRate > 0.0 ? static_cast<uint32_t>(maxRewindHistory / inputRate) : 0;

        IMultiplayer* multiplayer = GetMultiplayer();
        INetworkTime* networkTime = GetNetworkTime();
        while (m_moveAccumulator >= inputRate)
        {
            m_moveAccumulator -= inputRate;
            ++m_clientInputId;

            NetworkInputArray inputArray(GetEntityHandle());
            NetworkInput& input = inputArray[0];

            input.SetClientInputId(m_clientInputId);
            input.SetHostFrameId(networkTime->GetHostFrameId());
            input.SetHostTimeMs(multiplayer->GetCurrentHostTimeMs());

            // Allow components to form the input for this frame
            GetNetBindComponent()->CreateInput(input, inputRate);

            // Process the input for this frame
            GetNetBindComponent()->ProcessInput(input, inputRate);

            AZLOG
            (
                NET_Prediction,
                "Processed InutId=%d - i=[%s] o=[%s]",
                aznumeric_cast<int32_t>(m_clientInputId),
                GetInputString(input).c_str(),
                GetCorrectionDataString(GetNetBindComponent()).c_str()
            );

            // Generate a hash based on the current client predicted states
            AzNetworking::HashSerializer hashSerializer;
            GetNetBindComponent()->SerializeEntityCorrection(hashSerializer);

            // In debug, send the entire client output state to the server to make it easier to debug desync issues
            AzNetworking::PacketEncodingBuffer processInputResult;
#ifndef AZ_RELEASE_BUILD
            if (cl_EnableDesyncDebugging)
            {
                AzNetworking::NetworkInputSerializer processInputResultSerializer(processInputResult.GetBuffer(), processInputResult.GetCapacity());
                GetNetBindComponent()->SerializeEntityCorrection(processInputResultSerializer);
                processInputResult.Resize(processInputResultSerializer.GetSize());
            }
#endif

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
                inputArray[i] = m_inputHistory[historyIndex];
            }

            // Send the input to server (only when we are not migrating)
            if (!IsMigrating())
            {
                SendClientInput(inputArray, hashSerializer.GetHash(), processInputResult);
            }
        }
    }

    void LocalPredictionPlayerInputComponentController::UpdateBankedTime(AZ::TimeMs deltaTimeMs)
    {
        const double deltaTime = static_cast<double>(deltaTimeMs) / 1000.0;
        const double inputRate = static_cast<double>(static_cast<AZ::TimeMs>(cl_InputRateMs)) / 1000.0;
        const double maxRewindHistory = static_cast<double>(static_cast<AZ::TimeMs>(cl_MaxRewindHistoryMs)) / 1000.0;

        // Update banked time accumulator
        m_clientBankedTime -= deltaTime;

        // Forcibly tick any clients who are too far behind our variable latency window
        // Client may be slow hacking
        if (m_clientBankedTime < -sv_MaxBankTimeWindowSec)
        {
            m_clientBankedTime = -sv_MaxBankTimeWindowSec; // clamp to boundary

            NetworkInput& input = m_lastInputReceived[0];
            {
                ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), AzNetworking::InvalidConnectionId);
                GetNetBindComponent()->ProcessInput(input, inputRate);
            }

            AZLOG
            (
                NET_Prediction,
                "Forced InputId=%d - i=[%s] o=[%s]",
                aznumeric_cast<int32_t>(input.GetClientInputId()),
                GetInputString(input).c_str(),
                GetCorrectionDataString(GetNetBindComponent()).c_str()
            );
        }

        // Decay our bank time window, in case the remote endpoint has suffered a more persistent shift in latency, this should cause the client to eventually recover
        m_clientBankedTime = m_clientBankedTime * (1.0 - sv_BankTimeDecay);
    }
}
