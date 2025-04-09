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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzNetworking/ConnectionLayer/SequenceGenerator.h>
#include <AzNetworking/Serialization/HashSerializer.h>
#include <AzNetworking/Serialization/StringifySerializer.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <Multiplayer/MultiplayerDebug.h>

namespace Multiplayer
{
    AZ_CVAR(AZ::TimeMs, cl_InputRateMs, AZ::TimeMs{ 33 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Rate at which to sample and process client inputs");
    AZ_CVAR(AZ::TimeMs, cl_MaxRewindHistoryMs, AZ::TimeMs{ 2000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum number of milliseconds to keep for server correction rewind and replay");
#ifndef AZ_RELEASE_BUILD
    AZ_CVAR(float, cl_DebugHackTimeMultiplier, 1.0f, nullptr, AZ::ConsoleFunctorFlags::Null, "Scalar value used to simulate clock hacking cheats for validating bank time system and anticheat");
    AZ_CVAR(bool, cl_EnableDesyncDebugging, true, nullptr, AZ::ConsoleFunctorFlags::Null, "If enabled, debug logs will contain verbose information on detected state desyncs");
    AZ_CVAR(bool, cl_DesyncDebugging_AuditInputs, false, nullptr, AZ::ConsoleFunctorFlags::Null, "If true, adds inputs to audit trail");
    AZ_CVAR(uint32_t, cl_PredictiveStateHistorySize, 120, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls how many inputs of predictive state should be retained for debugging desyncs");
#endif

#if AZ_TRAIT_SERVER
    AZ_CVAR(bool, sv_ForceCorrections, false, nullptr, AZ::ConsoleFunctorFlags::Null, "If enabled, the server will force a correction for every input received for debugging");
    AZ_CVAR(bool, sv_EnableCorrections, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Enables server corrections on autonomous proxy desyncs");
    AZ_CVAR(double, sv_MaxBankTimeWindowSec, 0.2, nullptr, AZ::ConsoleFunctorFlags::Null, "Maximum bank time we allow before we start rejecting autonomous proxy move inputs due to anticheat kicking in");
    AZ_CVAR(double, sv_BankTimeDecay, 0.05, nullptr, AZ::ConsoleFunctorFlags::Null, "Amount to decay bank time by, in case of more permanent shifts in client latency");
    AZ_CVAR(AZ::TimeMs, sv_MinCorrectionTimeMs, AZ::TimeMs{ 100 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum time to wait between sending out corrections in order to avoid flooding corrections on high-latency connections");
    AZ_CVAR(AZ::TimeMs, sv_InputUpdateTimeMs, AZ::TimeMs{ 5 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Minimum time between component updates");
#endif

    void PrintCorrectionDifferences(const AzNetworking::StringifySerializer& client, const AzNetworking::StringifySerializer& server, MultiplayerAuditingElement* detail = nullptr)
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
            AZLOG_ERROR("The hash mismatched, but no differences were found.");
            if (detail)
            {
                detail->m_elements.emplace_back(
                    AZStd::make_unique<MultiplayerAuditingDatum<AZStd::string>>("The hash mismatched, but no differences were found."));
            }
            return;
        }

        for (auto iter = differences.begin(); iter != differences.end(); ++iter)
        {
            auto clientValueIter = clientMap.find(iter->first);
            auto serverValueIter = serverMap.find(iter->first);
            if (clientValueIter == clientMap.end() || serverValueIter == serverMap.end())
            {
                AZStd::string errorMsg;
                if (clientValueIter == clientMap.end() && serverValueIter == serverMap.end())
                {
                    errorMsg = "%s not found in server and client value map!";
                }
                else if (clientValueIter == clientMap.end())
                {
                    errorMsg = "%s not found in client value map!";
                }
                else
                {
                    errorMsg = "%s not found in server value map!";
                }
                AZLOG_ERROR(errorMsg.c_str(), iter->first.c_str());
                if (detail)
                {
                    detail->m_elements.emplace_back(AZStd::make_unique<MultiplayerAuditingDatum<AZStd::string>>(
                        AZStd::string::format(errorMsg.c_str(), iter->first.c_str())));
                }
                continue;
            }

            AZLOG_ERROR("    %s Server=%s Client=%s", iter->first.c_str(), serverValueIter->second.c_str(), clientValueIter->second.c_str());
            if (detail)
            {
                detail->m_elements.emplace_back(AZStd::make_unique<MultiplayerAuditingDatum<AZStd::string>>(iter->first,
                    clientValueIter->second, serverValueIter->second));
            }
        }
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
#if AZ_TRAIT_SERVER
        , m_updateBankedTimeEvent([this]() { UpdateBankedTime(m_updateBankedTimeEvent.TimeInQueueMs()); }, AZ::Name("BankTimeUpdate Event"))
#endif

#if AZ_TRAIT_CLIENT
        , m_autonomousUpdateEvent([this]() { UpdateAutonomous(m_autonomousUpdateEvent.TimeInQueueMs()); }, AZ::Name("AutonomousUpdate Event"))
        , m_migrateStartHandler([this](ClientInputId migratedInputId) { OnMigrateStart(migratedInputId); })
        , m_migrateEndHandler([this]() { OnMigrateEnd(); })
#endif
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

#if AZ_TRAIT_CLIENT
        if (IsNetEntityRoleAutonomous())
        {
            m_autonomousUpdateEvent.Enqueue(AZ::TimeMs{ 1 }, true);
            GetMultiplayer()->AddClientMigrationStartEventHandler(m_migrateStartHandler);
            GetMultiplayer()->AddClientMigrationEndEventHandler(m_migrateEndHandler);
        }
#endif
    }

    void LocalPredictionPlayerInputComponentController::OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating)
    {
#if AZ_TRAIT_CLIENT
        if (IsNetEntityRoleAutonomous())
        {
            m_autonomousUpdateEvent.RemoveFromQueue();
            m_migrateStartHandler.Disconnect();
            m_migrateEndHandler.Disconnect();
        }
#endif
    }

#if AZ_TRAIT_SERVER
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

        // After receiving the first input from the client, start the update event to check for slow hacking.
        // Also initialize the lastClientInputId to one before the oldest available one in the inputArray so that
        // we process everything available to us on the first call.
        if (!m_updateBankedTimeEvent.IsScheduled())
        {
            // This subtraction intentionally wraps around.
            m_lastClientInputId = inputArray[NetworkInputArray::MaxElements - 1].GetClientInputId() - ClientInputId(1);

            m_updateBankedTimeEvent.Enqueue(sv_InputUpdateTimeMs, true);
        }

        const ClientInputId clientInputId = inputArray[0].GetClientInputId();
        if (!AzNetworking::SequenceMoreRecent(clientInputId, m_lastClientInputId))
        {
            AZLOG(NET_Prediction, "Discarding old or out of order move input (current: %u, received %u)",
                aznumeric_cast<uint32_t>(m_lastClientInputId), aznumeric_cast<uint32_t>(clientInputId));
            return;
        }

        const AZ::TimeMs currentTimeMs = AZ::GetElapsedTimeMs();
        const double clientInputRateSec = AZ::TimeMsToSecondsDouble(cl_InputRateMs);
        m_lastInputReceivedTimeMs = currentTimeMs;

        // Keep track of last inputs received, also allows us to update frame ids
        m_lastInputReceived = inputArray;
        SetLastInputId(m_lastInputReceived[0].GetClientInputId()); // Set this variable in case of migration

        // Since id values can wrap around, we intentionally compare with a "!=" instead of a "<".
        while (m_lastClientInputId != clientInputId)
        {
            ++m_lastClientInputId;

            // Figure out which index from the input array we want
            // If we have skipped an id, check if it was sent to us in the array. If we have lost too many, just use the oldest one in the array
            const ClientInputId deltaInputId = clientInputId - m_lastClientInputId; // The subtraction intentionally wraps around
            const uint32_t inputArrayIdx = AZStd::min(aznumeric_cast<uint32_t>(deltaInputId), NetworkInputArray::MaxElements - 1);
            const bool lostInput = aznumeric_cast<uint32_t>(deltaInputId) >= NetworkInputArray::MaxElements; // For logging only

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
#ifndef AZ_RELEASE_BUILD
                    if (cl_EnableDesyncDebugging && cl_DesyncDebugging_AuditInputs)
                    {
                        // Add to Audit Trail here (server)
                        if (IMultiplayerDebug* mpDebug = AZ::Interface<IMultiplayerDebug>::Get())
                        {
                            AZStd::vector<MultiplayerAuditingElement> inputLogs = input.GetComponentInputDeltaLogs();
                            if (!inputLogs.empty())
                            {
                                mpDebug->AddAuditEntry(
                                    AuditCategory::Input,
                                    input.GetClientInputId(),
                                    input.GetHostFrameId(),
                                    GetEntity()->GetName(),
                                    AZStd::move(inputLogs));
                            }
                        }
                    }
#endif
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
                InputSerializer serializer(correction.GetBuffer(), static_cast<uint32_t>(correction.GetCapacity()));

                // only deserialize if we have data (for client/server profile/debug mismatches)
                if (correction.GetSize() > 0)
                {
                    SerializeEntityCorrection(serializer);
                }

                correction.Resize(serializer.GetSize());

                AZLOG_INFO(
                    "** Autonomous Desync - Corrected clientInputId=%hu at hostFrame=%u hostTime=%" PRId64,
                        static_cast<uint16_t>(m_lastClientInputId),
                        static_cast<uint32_t>(m_lastInputReceived[0].GetHostFrameId()),
                        static_cast<int64_t>(m_lastInputReceived[0].GetHostTimeMs()));

 #ifndef AZ_RELEASE_BUILD
                if (cl_EnableDesyncDebugging)
                {
                    if (IMultiplayerDebug* mpDebug = AZ::Interface<IMultiplayerDebug>::Get())
                    {
                        MultiplayerAuditingElement detail;
                        detail.m_name = AZStd::string::format(
                            "Autonomous Desync - Corrected clientInputId=%hu at hostFrame=%u hostTime=%" PRId64,
                            static_cast<uint16_t>(m_lastClientInputId),
                            static_cast<uint32_t>(m_lastInputReceived[0].GetHostFrameId()),
                            static_cast<int64_t>(m_lastInputReceived[0].GetHostTimeMs()));
                        mpDebug->AddAuditEntry(
                            AuditCategory::Desync,
                            m_lastClientInputId,
                            m_lastInputReceived[0].GetHostFrameId(),
                            GetEntity()->GetName(),
                            { AZStd::move(detail) });
                    }
                }
 #endif

                // Send correction. Include both the latest client input host frame id and the latest client input id processed so that
                // the client can ensure that it doesn't try to process out-of-order corrections. The client input id is a uint16, which
                // can roll over in (65536 / 60 fps) which is < 20 minutes. If half that time or more passes between corrections,
                // so if we only tried to rely on the client input id to detect out-of-order corrections, we wouldn't be able to tell if
                // the difference is telling us that it's out of order or if a long time had passed. By sending the host frame id too,
                // we can distinguish between the two cases.
                SendClientInputCorrection(m_lastInputReceived[0].GetHostFrameId(), m_lastClientInputId, correction);
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

        const double clientInputRateSec = AZ::TimeMsToSecondsDouble(cl_InputRateMs);

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

    void LocalPredictionPlayerInputComponentController::UpdateBankedTime(AZ::TimeMs deltaTimeMs)
    {
        const double deltaTime = AZ::TimeMsToSecondsDouble(deltaTimeMs);
        const double clientInputRateSec = AZ::TimeMsToSecondsDouble(cl_InputRateMs);

        // Update banked time accumulator
        m_clientBankedTime -= deltaTime;

        // Forcibly tick any clients who are too far behind our variable latency window
        // Client may be slow hacking
        if (m_clientBankedTime < -sv_MaxBankTimeWindowSec)
        {
            m_clientBankedTime = -sv_MaxBankTimeWindowSec + clientInputRateSec; // Clamp to boundary and advance by one input worth of time

            NetworkInput& input = m_lastInputReceived[0];
            {
                ScopedAlterTime scopedTime(input.GetHostFrameId(), input.GetHostTimeMs(), DefaultBlendFactor, GetNetBindComponent()->GetOwningConnectionId());
                GetNetBindComponent()->ProcessInput(input, static_cast<float>(clientInputRateSec));
            }

            AZLOG(NET_Prediction, "Forced InputId=%d", aznumeric_cast<int32_t>(input.GetClientInputId()));
        }

        // Decay our bank time window, in case the remote endpoint has suffered a more persistent shift in latency, this should cause the
        // client to eventually recover
        m_clientBankedTime = m_clientBankedTime * (1.0 - sv_BankTimeDecay);
    }
#endif

#if AZ_TRAIT_CLIENT
    void LocalPredictionPlayerInputComponentController::HandleSendClientInputCorrection
    (
        AzNetworking::IConnection* invokingConnection,
        const Multiplayer::HostFrameId& inputHostFrameId,
        const Multiplayer::ClientInputId& inputId,
        const AzNetworking::PacketEncodingBuffer& correction
    )
    {
        AZ_Assert(invokingConnection != nullptr, "Invalid connection, cannot reprocess corrections.");

        INetworkTime* networkTime = GetNetworkTime();

        // Corrections that have been sent backwards in time from this client's future are disallowed.
        if (inputHostFrameId > networkTime->GetHostFrameId())
        {
            AZLOG_ERROR(
                "Invalid correction frame id, newer than current client frame: current host frame %u, received host frame %u, input id %u",
                aznumeric_cast<uint32_t>(networkTime->GetHostFrameId()),
                aznumeric_cast<uint32_t>(inputHostFrameId),
                aznumeric_cast<uint32_t>(inputId));
            return;
        }

        // If this isn't the first correction we've received, verify that we're processing the correction in order.
        // We'll discard any out-of-order corrections.
        if (m_lastCorrectionHostFrameId != InvalidHostFrameId)
        {
            // Discard any corrections that arrived out-of-order based on host frame id.
            if (inputHostFrameId < m_lastCorrectionHostFrameId)
            {
                AZLOG(
                    NET_Prediction,
                    "Discarding old correction for client host frame %u input id %u, host frame is older than last processed correction.",
                    aznumeric_cast<uint32_t>(inputHostFrameId),
                    aznumeric_cast<uint32_t>(inputId));
                return;
            }
            else
            {
                // It's possible to receive corrections where the host frame is identical but the client input ids are out of sequence
                // if we sent multiple inputs in the same frame, the server received and processed them across multiple frames, and
                // we then received the corrections out-of-order.
                if (!AzNetworking::SequenceMoreRecent(inputId, m_lastCorrectionInputId))
                {
                    AZLOG(
                        NET_Prediction,
                        "Discarding old correction for client host frame %u input id %u, input id is older than last processed correction.",
                        aznumeric_cast<uint32_t>(inputHostFrameId),
                        aznumeric_cast<uint32_t>(inputId));
                    return;
                }
            }
        }

        m_lastCorrectionHostFrameId = inputHostFrameId;
        m_lastCorrectionInputId = inputId;

        // Apply the correction
        OutputSerializer serializer(correction.GetBuffer(), static_cast<uint32_t>(correction.GetSize()));
        SerializeEntityCorrection(serializer);
        GetNetBindComponent()->NotifyCorrection();

        const uint32_t inputHistorySize = static_cast<uint32_t>(m_inputHistory.Size());

        // Do not replay the move just corrected, it was already processed by the server. Start replaying one past that move.
        // (The subtraction intentionally wraps around to capture the historical delta even on id rollovers)
        const ClientInputId historicalDelta = m_clientInputId - inputId;

        // If this correction is for a move outside our input history window, just start replaying from the oldest move we have available
        const uint32_t startReplayIndex = (inputHistorySize > aznumeric_cast<uint32_t>(historicalDelta))
            ? (inputHistorySize - aznumeric_cast<uint32_t>(historicalDelta))
            : 0;

#ifndef AZ_RELEASE_BUILD
        if (cl_EnableDesyncDebugging)
        {
            int32_t inputFrameId = 0;
            if (startReplayIndex < inputHistorySize)
            {
                inputFrameId = aznumeric_cast<int32_t>(m_inputHistory[startReplayIndex].GetHostFrameId());
            }

            AZLOG_WARN("** Autonomous Desync - Correcting clientInputId=%d from host frame=%d", aznumeric_cast<int32_t>(inputId), inputFrameId);

            auto iter = m_predictiveStateHistory.find(inputId);
            if (iter != m_predictiveStateHistory.end())
            {
                // Correction starts a frame after the desync, grab the correct host frame input for book keeping
                const uint32_t correctedIndex = startReplayIndex > 0 ? startReplayIndex - 1 : 0;
                const NetworkInput& correctedInput = m_inputHistory[correctedIndex];

                // Read out state values
                AzNetworking::StringifySerializer serverValues;
                SerializeEntityCorrection(serverValues);
                MultiplayerAuditingElement detail;
                PrintCorrectionDifferences(*iter->second, serverValues, &detail);
                if (IMultiplayerDebug* mpDebug = AZ::Interface<IMultiplayerDebug>::Get())
                {
                    detail.m_name = AZStd::string::format("Autonomous Desync - Correcting clientInputId=%d from host frame=%d", aznumeric_cast<int32_t>(inputId), inputFrameId);
                    mpDebug->AddAuditEntry(AuditCategory::Desync, inputId, correctedInput.GetHostFrameId(), GetEntity()->GetName(), { AZStd::move(detail) });
                }
            }
            else
            {
                AZLOG_INFO("Received correction that is too old to diff, increase cl_PredictiveStateHistorySize");
            }
        }
#endif

        const double clientInputRateSec = AZ::TimeMsToSecondsDouble(cl_InputRateMs);
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
        const double deltaTime = AZ::TimeMsToSecondsDouble(deltaTimeMs);
        const double clientInputRateSec = AZ::TimeMsToSecondsDouble(cl_InputRateMs);
        const double maxRewindHistory = AZ::TimeMsToSecondsDouble(cl_MaxRewindHistoryMs);

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
                StateHistoryItem inputHistory = AZStd::make_shared<AzNetworking::StringifySerializer>();
                while (m_predictiveStateHistory.size() > cl_PredictiveStateHistorySize)
                {
                    m_predictiveStateHistory.erase(m_predictiveStateHistory.begin());
                }

                if (inputHistory != nullptr)
                {
                    SerializeEntityCorrection(*inputHistory);
                    m_predictiveStateHistory.emplace(m_clientInputId, inputHistory);
                }

                if (cl_DesyncDebugging_AuditInputs)
                {
                    if (IMultiplayerDebug* mpDebug = AZ::Interface<IMultiplayerDebug>::Get())
                    {
                        // Add to audit trail per input here (client)
                        AZStd::vector<MultiplayerAuditingElement> inputLogs = input.GetComponentInputDeltaLogs();
                        if (!inputLogs.empty())
                        {
                            mpDebug->AddAuditEntry(
                                AuditCategory::Input,
                                input.GetClientInputId(),
                                input.GetHostFrameId(),
                                GetEntity()->GetName(),
                                AZStd::move(inputLogs));
                        }
                    }
                }

            }
#endif

            // Send the input to server (only when we are not migrating)
            if (!IsMigrating())
            {
                SendClientInput(inputArray, hashSerializer.GetHash());
            }
        }
    }
#endif

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
}
