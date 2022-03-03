/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/LocalPredictionPlayerInputComponent.AutoComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <AzNetworking/Serialization/StringifySerializer.h>

namespace Multiplayer
{
    class LocalPredictionPlayerInputComponent
        : public LocalPredictionPlayerInputComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::LocalPredictionPlayerInputComponent, s_localPredictionPlayerInputComponentConcreteUuid, Multiplayer::LocalPredictionPlayerInputComponentBase);

        static void Reflect([[maybe_unused]] AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        void OnInit() override;
        void OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override;
    };

    class LocalPredictionPlayerInputComponentController
        : public LocalPredictionPlayerInputComponentControllerBase
    {
    public:
        LocalPredictionPlayerInputComponentController(LocalPredictionPlayerInputComponent& parent);

        void OnActivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate([[maybe_unused]] Multiplayer::EntityIsMigrating entityIsMigrating) override;

        void HandleSendClientInput
        (
            AzNetworking::IConnection* invokingConnection,
            const Multiplayer::NetworkInputArray& inputArray,
            const AZ::HashValue32& stateHash
        ) override;

        void HandleSendMigrateClientInput
        (
            AzNetworking::IConnection* invokingConnection,
            const Multiplayer::NetworkInputMigrationVector& inputArray
        ) override;

        void HandleSendClientInputCorrection
        (
            AzNetworking::IConnection* invokingConnection,
            const Multiplayer::ClientInputId& inputId,
            const AzNetworking::PacketEncodingBuffer& correction
        ) override;

        //! Forcibly enables ProcessInput to execute on the entity.
        //! Note that this function is quite dangerous and should normally never be used
        void ForceEnableAutonomousUpdate();

        //! Forcibly disables ProcessInput from executing on the entity.
        //! Note that this function is quite dangerous and should normally never be used
        void ForceDisableAutonomousUpdate();

        //! Return true if we're currently migrating from one host to another.
        //! @return boolean true if we're currently migrating from one host to another
        bool IsMigrating() const;

        ClientInputId GetLastInputId() const;
        HostFrameId GetInputFrameId(const NetworkInput& input) const;

    private:

        void OnMigrateStart(ClientInputId migratedInputId);
        void OnMigrateEnd();
        void UpdateAutonomous(AZ::TimeMs deltaTimeMs);
        void UpdateBankedTime(AZ::TimeMs deltaTimeMs);

        bool SerializeEntityCorrection(AzNetworking::ISerializer& serializer);

        using StateHistoryItem = AZStd::unique_ptr<AzNetworking::StringifySerializer>;
        AZStd::map<ClientInputId, StateHistoryItem> m_predictiveStateHistory;

        // Implicitly sorted player input history, back() is the input that corresponds to the latest client input Id
        NetworkInputHistory m_inputHistory;

        // Anti-cheat accumulator for clients who purposely mess with their clock rate
        NetworkInputArray m_lastInputReceived;

        AZ::ScheduledEvent m_autonomousUpdateEvent; // Drives autonomous input collection
        AZ::ScheduledEvent m_updateBankedTimeEvent; // Drives authority bank time updates

        ClientMigrationStartEvent::Handler m_migrateStartHandler;
        ClientMigrationEndEvent::Handler m_migrateEndHandler;

        double m_moveAccumulator = 0.0;
        double m_clientBankedTime = 0.0;

        AZ::TimeMs m_lastInputReceivedTimeMs = AZ::Time::ZeroTimeMs;
        AZ::TimeMs m_lastCorrectionSentTimeMs = AZ::Time::ZeroTimeMs;

        ClientInputId m_clientInputId = ClientInputId{ 0 }; // Clients incrementing inputId
        ClientInputId m_lastClientInputId = ClientInputId{ 0 }; // Last inputId processed by the server
        ClientInputId m_lastCorrectionInputId = ClientInputId{ 0 };
        ClientInputId m_lastMigratedInputId = ClientInputId{ 0 }; // Used to resend inputs that were queued during a migration event
        HostFrameId   m_serverMigrateFrameId = InvalidHostFrameId;

        bool m_allowMigrateClientInput = false; // True if this component was migrated, we will allow the client to send us migrated inputs (one time only)
    };
}
