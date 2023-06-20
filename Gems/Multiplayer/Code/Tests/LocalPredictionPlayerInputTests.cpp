/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonBenchmarkSetup.h>
#include <CommonHierarchySetup.h>
#include <MockInterfaces.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Name/Name.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Time/ITime.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Spawnable/SpawnableSystemComponent.h>
#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzNetworking/Framework/NetworkingSystemComponent.h>
#include <AzTest/AzTest.h>
#include <MultiplayerSystemComponent.h>
#include <IMultiplayerConnectionMock.h>
#include <IMultiplayerSpawnerMock.h>
#include <ConnectionData/ServerToClientConnectionData.h>
#include <ReplicationWindows/ServerToClientReplicationWindow.h>
#include <Multiplayer/Components/LocalPredictionPlayerInputComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/MultiplayerConstants.h>
#include <Multiplayer/Session/SessionConfig.h>
#include <Tests/TestMultiplayerComponent.h>

namespace Multiplayer
{
    class LocalPredictionPlayerInputTests : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            AZ::NameDictionary::Create();

            m_ComponentApplicationRequests = AZStd::make_unique<BenchmarkComponentApplicationRequests>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_ComponentApplicationRequests.get());

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());

            m_timeSystem.reset();
            m_timeSystem = AZStd::make_unique<::testing::NiceMock<AZ::MockTimeSystem>>();

            // For convenience, reroute these methods to all return m_mockElapsedTime so that our tests can
            // precisely control the passing of time.
            ON_CALL(*m_timeSystem, GetElapsedTimeUs())
                .WillByDefault([this]() { return AZ::TimeMsToUs(m_mockElapsedTime); });
            ON_CALL(*m_timeSystem, GetRealElapsedTimeUs())
                .WillByDefault([this]() { return AZ::TimeMsToUs(m_mockElapsedTime); });
            ON_CALL(*m_timeSystem, GetElapsedTimeMs())
                .WillByDefault([this]() { return m_mockElapsedTime; });
            ON_CALL(*m_timeSystem, GetRealElapsedTimeMs())
                .WillByDefault([this]() { return m_mockElapsedTime; });

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformDescriptor->Reflect(m_serializeContext.get());
            m_netBindDescriptor.reset(NetBindComponent::CreateDescriptor());
            m_netBindDescriptor->Reflect(m_serializeContext.get());
            m_netTransformDescriptor.reset(NetworkTransformComponent::CreateDescriptor());
            m_netTransformDescriptor->Reflect(m_serializeContext.get());
            m_localPredictionDescriptor.reset(LocalPredictionPlayerInputComponent::CreateDescriptor());
            m_localPredictionDescriptor->Reflect(m_serializeContext.get());
            m_testMultiplayerComponentDescriptor.reset(MultiplayerTest::TestMultiplayerComponent::CreateDescriptor());
            m_testMultiplayerComponentDescriptor->Reflect(m_serializeContext.get());
            m_testInputDriverComponentDescriptor.reset(MultiplayerTest::TestInputDriverComponent::CreateDescriptor());
            m_testInputDriverComponentDescriptor->Reflect(m_serializeContext.get());


            m_netComponent = new AzNetworking::NetworkingSystemComponent();
            m_mpComponent = new Multiplayer::MultiplayerSystemComponent();
            m_mpComponent->Reflect(m_serializeContext.get());
            m_mpComponent->Reflect(m_behaviorContext.get());

            m_mpComponent->Activate();
            m_eventScheduler = new AZ::EventSchedulerSystemComponent();
            m_eventScheduler->Reflect(m_serializeContext.get());
            m_eventScheduler->Activate();

            m_playerEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "Test");
            m_playerNetworkEntityTracker = AZStd::make_unique<NetworkEntityTracker>();
            m_playerEntity->CreateComponent<AzFramework::TransformComponent>();
            m_playerEntity->CreateComponent<NetworkTransformComponent>();
            m_playerEntity->CreateComponent<MultiplayerTest::TestMultiplayerComponent>();
            m_playerEntity->CreateComponent<MultiplayerTest::TestInputDriverComponent>();
            m_localPredictionComponent = m_playerEntity->CreateComponent<LocalPredictionPlayerInputComponent>();
        }

        void ActivatePlayerEntity(NetEntityRole role)
        {
            const auto playerNetBindComponent = m_playerEntity->CreateComponent<NetBindComponent>();
            playerNetBindComponent->PreInit(
                m_playerEntity.get(), PrefabEntityId{ AZ::Name("test"), 1 }, NetEntityId{ 1 }, role);
            m_playerNetworkEntityTracker->RegisterNetBindComponent(m_playerEntity.get(), playerNetBindComponent);
            m_playerEntity->Init();
            m_playerEntity->Activate();
        }

        void TearDown() override
        {
            m_playerEntity->Deactivate();
            m_playerNetworkEntityTracker.reset();
            m_mpComponent->Deactivate();
            m_eventScheduler->Deactivate();
            m_localPredictionComponent = nullptr;
            m_playerEntity.reset();
            delete m_mpComponent;
            delete m_netComponent;
            delete m_eventScheduler;
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();
            m_timeSystem.reset();
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_ComponentApplicationRequests.get());
            m_ComponentApplicationRequests.reset();
            AZ::NameDictionary::Destroy();

            m_testInputDriverComponentDescriptor.reset();
            m_testMultiplayerComponentDescriptor.reset();
            m_localPredictionDescriptor.reset();
            m_transformDescriptor.reset();
            m_netTransformDescriptor.reset();
            m_netBindDescriptor.reset();
            m_serializeContext.reset();
            m_behaviorContext.reset();
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netTransformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_localPredictionDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testMultiplayerComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testInputDriverComponentDescriptor;
        AZStd::unique_ptr<AZ::IConsole> m_console;
        AZStd::unique_ptr<::testing::NiceMock<AZ::MockTimeSystem>> m_timeSystem;

        AZ::TimeMs m_mockElapsedTime = AZ::TimeMs(0);

        AzNetworking::NetworkingSystemComponent* m_netComponent = nullptr;
        Multiplayer::MultiplayerSystemComponent* m_mpComponent = nullptr;
        AZ::EventSchedulerSystemComponent* m_eventScheduler = nullptr;
        Multiplayer::LocalPredictionPlayerInputComponent* m_localPredictionComponent = nullptr;

        AZStd::unique_ptr<BenchmarkComponentApplicationRequests> m_ComponentApplicationRequests;

        AZStd::unique_ptr<AZ::Entity> m_playerEntity;
        AZStd::unique_ptr<NetworkEntityTracker> m_playerNetworkEntityTracker;

        IMultiplayerSpawnerMock m_mpSpawnerMock;
    };

    TEST_F(LocalPredictionPlayerInputTests, TestChildController)
    {
        ActivatePlayerEntity(NetEntityRole::Autonomous);
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        MultiplayerController* parentController = m_localPredictionComponent->GetController();
        LocalPredictionPlayerInputComponentController* controllerPtr =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(parentController);
        LocalPredictionPlayerInputComponentController childCotroller(*controllerPtr);
    }

    TEST_F(LocalPredictionPlayerInputTests, TestUpdateAutonomous)
    {
        ActivatePlayerEntity(NetEntityRole::Autonomous);
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());
        controller->ForceEnableAutonomousUpdate();
        m_mockElapsedTime = AZ::TimeMs(1000);
        m_eventScheduler->OnTick(1000, AZ::ScriptTimePoint());
        controller->ForceDisableAutonomousUpdate();
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendClientInput)
    {
        ActivatePlayerEntity(NetEntityRole::Autonomous);
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        ::testing::NiceMock<IMultiplayerConnectionMock> connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        Multiplayer::NetworkInputArray netInputArray;
        netInputArray[0].SetHostBlendFactor(0.8f);
        netInputArray[0].SetHostTimeMs(AZ::TimeMs(1));
        netInputArray[0].SetHostFrameId(HostFrameId(1));
        AZ::HashValue32 dummyHash = AZ::HashValue32(0);

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());
        controller->HandleSendClientInput(nullptr, netInputArray, dummyHash);
        controller->HandleSendClientInput(&connection, netInputArray, dummyHash);
        netInputArray[0].SetClientInputId(ClientInputId(1));
        // Force update to increment client input ID
        m_mockElapsedTime = AZ::TimeMs(1000);
        controller->HandleSendClientInput(&connection, netInputArray, dummyHash);
        // Force update to update banked time
        m_mockElapsedTime = AZ::TimeMs(1010);
        m_eventScheduler->OnTick(1000, AZ::ScriptTimePoint());

        EXPECT_EQ(controller->GetInputFrameId(netInputArray[0]), netInputArray[0].GetHostFrameId());
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendClientInputCorrection)
    {
        ActivatePlayerEntity(NetEntityRole::Autonomous);
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        AzNetworking::PacketEncodingBuffer buffer;

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());

        // Sending an input correction with a host frame id that hasn't been generated yet client-side should produce an error.
        AZ_TEST_START_TRACE_SUPPRESSION;
        controller->HandleSendClientInputCorrection(nullptr, HostFrameId(1), ClientInputId(1), buffer);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        ::testing::NiceMock<IMultiplayerConnectionMock> connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        // Force update to increment client input id
        controller->ForceEnableAutonomousUpdate();
        m_mockElapsedTime = AZ::TimeMs(1000);
        m_eventScheduler->OnTick(100, AZ::ScriptTimePoint());

        // Input corrections with a host frame id <= current host frame id should both be processed and generate no errors.
        controller->HandleSendClientInputCorrection(&connection, HostFrameId(1), ClientInputId(0), buffer);
        controller->HandleSendClientInputCorrection(&connection, HostFrameId(1), ClientInputId(1), buffer);
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendMigrateClientInput)
    {
        ActivatePlayerEntity(NetEntityRole::Autonomous);
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        ::testing::NiceMock<IMultiplayerConnectionMock> connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        Multiplayer::NetworkInputArray netInputArray;
        netInputArray[0].SetHostBlendFactor(0.8f);
        netInputArray[0].SetHostTimeMs(AZ::TimeMs(1));
        netInputArray[0].SetHostFrameId(HostFrameId(1));
        Multiplayer::NetworkInputMigrationVector netMigrationVector;
        netMigrationVector.PushBack(netInputArray[0]);

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());
        controller->OnDeactivate(EntityIsMigrating::False);
        controller->OnActivate(EntityIsMigrating::True);
        controller->HandleSendMigrateClientInput(&connection, netMigrationVector);
        controller->HandleSendMigrateClientInput(&connection, netMigrationVector);
        controller->OnDeactivate(EntityIsMigrating::False);
        controller->OnActivate(EntityIsMigrating::True);
        controller->HandleSendMigrateClientInput(nullptr, netMigrationVector);
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendClientInputWithIdWraparound)
    {
        // The ClientInputId is defined as uint16_t, so the values in it can wrap around in <20 minutes at 60 fps.
        // There was a bug with HandleSendClientInput where it would stop processing inputs correctly once the ClientInputId
        // reached the max uint16_t value and wrapped around to 0. This unit test verifies that there are no regressions
        // and the processing happens correctly during the wraparound.
        //
        // This also verifies a secondary regression in which ProcessInput would get called multiple times on the very
        // first input handled if the latest ClientInputId received was anything other than 0, even if the other entries
        // in the array were all identical. The correct behavior is that it should only process multiple entries if there
        // are actually multiple different entries in the array.

        // For this test, set the player as authority-only, so that UpdateAutonomous never gets called.
        // Otherwise, we'll get ProcessInput callbacks both from the "client" and the "server", which will make the test logic
        // more confusing and harder to validate.
        ActivatePlayerEntity(NetEntityRole::Authority);
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        ::testing::NiceMock<IMultiplayerConnectionMock> connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        // Initialize the starting time to an arbitrary value
        m_mockElapsedTime = AZ::TimeMs(1000);

        // Verify that we don't get any calls to CreateInput, since we're running as authority-only.
        auto createInputCallback = [](
                                        [[maybe_unused]] NetEntityId netEntityId,
                                        [[maybe_unused]] Multiplayer::NetworkInput& input,
                                        [[maybe_unused]] float deltaTime)
        {
            AZ_Assert(false, "CreateInput should not be called when the player entity is set to authority-only.");
        };

        // On each call to ProcessInput, verify that the ClientInputId and HostFrameId is the same one we're trying to process.
        // Also, track the total number of times called to avoid a "false positive" of appearing successful if it never gets called
        // or if it gets called multiple times in the same frame unexpectedly.
        size_t numProcessedInputs = 0;
        ClientInputId expectedInputId;
        HostFrameId hostFrameId = HostFrameId(0);
        auto processInputCallback =
            [&expectedInputId, &hostFrameId, &numProcessedInputs](
                                        [[maybe_unused]] NetEntityId netEntityId,
                                        Multiplayer::NetworkInput& input,
                                        [[maybe_unused]] float deltaTime)
        {
            EXPECT_EQ(static_cast<uint32_t>(input.GetHostFrameId()), static_cast<uint32_t>(hostFrameId));
            EXPECT_EQ(static_cast<uint32_t>(input.GetClientInputId()), static_cast<uint32_t>(expectedInputId));
            numProcessedInputs++;
        };

        // Set the callbacks for creating and processing input so that we can validate that input processing behaves correctly when
        // the client Ids wrap around.
        m_playerEntity->FindComponent<MultiplayerTest::TestMultiplayerComponent>()->m_createInputCallback = createInputCallback;
        m_playerEntity->FindComponent<MultiplayerTest::TestMultiplayerComponent>()->m_processInputCallback = processInputCallback;

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());

        // Since we're not doing anything with the inputs, the hash value won't be used for anything.
        constexpr AZ::HashValue32 dummyHash = AZ::HashValue32(0);

        // Pick starting and ending ClientInputId values to process that will wrap around through 0.
        constexpr ClientInputId StartingLargeInputId =
            ClientInputId{ AZStd::numeric_limits<AZStd::underlying_type<ClientInputId>::type>::max() - 10 };
        constexpr ClientInputId EndingWraparoundInputId = ClientInputId{ 10 };

        Multiplayer::NetworkInputArray netInputArray;

        // Initialize all the history in netInputArray to the same entry so that all entries are valid and match expectations
        // on the first call to HandleSendClientInput. (It always assumes that *all* entries in the array are valid)
        for (uint32_t index = 0; index < Multiplayer::NetworkInputArray::MaxElements; index++)
        {
            netInputArray[index].SetClientInputId(StartingLargeInputId);
            netInputArray[index].SetHostFrameId(hostFrameId);
            netInputArray[index].SetHostBlendFactor(0.8f);
            netInputArray[index].SetHostTimeMs(AZ::TimeMs(1));
        }

        // Loop through each client id and handle our mocked inputs.
        for (expectedInputId = StartingLargeInputId; expectedInputId != EndingWraparoundInputId; expectedInputId++, hostFrameId++)
        {
            // On each iteration, bump our inputs back one in the array to preserve an accurate history of entries.
            for (uint32_t index = 1; index < Multiplayer::NetworkInputArray::MaxElements; index++)
            {
                netInputArray[index] = netInputArray[index - 1];
            }

            // Set the latest entry to the current client input ID and host frame ID.
            netInputArray[0].SetClientInputId(expectedInputId);
            netInputArray[0].SetHostFrameId(hostFrameId);

            // Handle the mocked input. This should call ProcessInput to process only the latest entry in the array,
            // which inside the callback above will verify that we've been given the current expectedInputId to process.
            controller->HandleSendClientInput(&connection, netInputArray, dummyHash);
            m_mockElapsedTime += AZ::TimeMs(10);

            // Force UpdateBankedTime to get called. Without this, the client inputs would eventually stop processing because
            // the banked time will get too large and the test will fail.
            m_eventScheduler->OnTick(1000, AZ::ScriptTimePoint());
            m_mockElapsedTime += AZ::TimeMs(10);
        }

        // Verify that ProcessInput actually got called the correct number of times.
        constexpr size_t TotalExpectedProcessedInputs = static_cast<size_t>(EndingWraparoundInputId - StartingLargeInputId);
        EXPECT_EQ(numProcessedInputs, TotalExpectedProcessedInputs);
    }

    TEST_F(LocalPredictionPlayerInputTests, TestHandleSendClientInputCorrectionWithIdWraparound)
    {
        // The ClientInputId is defined as uint16_t, so the values in it can wrap around in <20 minutes at 60 fps.
        // There was a bug with HandleSendClientInputCorrection where it would only process input corrections if the
        // id was strictly <= the current id. This means that input corrections that wrapped around (ex: a correction of 65530
        // when we're currently on 10) would never process.
        // This unit test verifies that there are no regressions and the correction processing happens correctly during the wraparound.

        ActivatePlayerEntity(NetEntityRole::Autonomous);
        m_mpComponent->InitializeMultiplayer(MultiplayerAgentType::DedicatedServer);
        EXPECT_EQ(m_mpComponent->GetAgentType(), MultiplayerAgentType::DedicatedServer);

        LocalPredictionPlayerInputComponentController* controller =
            dynamic_cast<LocalPredictionPlayerInputComponentController*>(m_localPredictionComponent->GetController());

        // Force update to increment client input id
        controller->ForceEnableAutonomousUpdate();

        // Create a mock connection.
        ::testing::NiceMock<IMultiplayerConnectionMock> connection(
            ConnectionId{ 1 }, IpAddress("127.0.0.1", DefaultServerPort, ProtocolType::Udp), ConnectionRole::Connector);
        ServerToClientConnectionData connectionUserData(&connection, *m_mpComponent);
        connection.SetUserData(&connectionUserData);

        // Track the number of inputs that we create so that we can verify that we've created our desired starting condition
        // for the test, where we've got an input history that spans the wraparound.
        uint64_t numCreatedInputs = 0;
        auto createInputCallback = [&numCreatedInputs](
                                        [[maybe_unused]] NetEntityId netEntityId,
                                        [[maybe_unused]] Multiplayer::NetworkInput& input,
                                        [[maybe_unused]] float deltaTime)
        {
            numCreatedInputs++;
        };
        m_playerEntity->FindComponent<MultiplayerTest::TestMultiplayerComponent>()->m_createInputCallback = createInputCallback;

        // We want to generate (65535 + 10) inputs, so that we have a wrapped-around input history with both large and small ids in it.
        // If we set the elapsed time to (65535 + 10) * (cl_inputRateMs), we should get our desired number of inputs created.
        
        // Set the cl_InputRateMs to an arbitrary but nice round number for testing.
        constexpr int ArbitraryInputRateMs = 10;
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("cl_InputRateMs", { AZStd::string::format("%d", ArbitraryInputRateMs) });
        // Turn off desync debugging and delta serialization so that generating (65535 + 10) inputs doesn't take obnoxiously long.
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("cl_EnableDesyncDebugging", {"false"});
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand("net_useInputDeltaSerialization", { "false" });

        constexpr uint64_t desiredInputCount = AZStd::numeric_limits<AZStd::underlying_type<ClientInputId>::type>::max() + 10;
        m_mockElapsedTime += AZ::TimeMs(desiredInputCount * ArbitraryInputRateMs);
        m_eventScheduler->OnTick(100, AZ::ScriptTimePoint());
        EXPECT_EQ(numCreatedInputs, desiredInputCount);

        // We'll request a correction from a little before the wraparound, so that HandleSendClientInputCorrection will replay through
        // the wraparound to the last input we created above.
        constexpr ClientInputId LargeCorrectionInputId =
            ClientInputId{ AZStd::numeric_limits<AZStd::underlying_type<ClientInputId>::type>::max() - 10 };

        uint64_t numInputCorrectionsProcessed = 0;
        // The first processed input id is one past the the correction
        ClientInputId expectedCorrectionId = LargeCorrectionInputId + ClientInputId(1);
        auto processInputCallback = [&numInputCorrectionsProcessed, &expectedCorrectionId](
                                       [[maybe_unused]] NetEntityId netEntityId,
                                       Multiplayer::NetworkInput& input,
                                       [[maybe_unused]] float deltaTime)
        {
            EXPECT_EQ(static_cast<uint32_t>(input.GetClientInputId()), static_cast<uint32_t>(expectedCorrectionId));
            numInputCorrectionsProcessed++;
            expectedCorrectionId++;
        };
        m_playerEntity->FindComponent<MultiplayerTest::TestMultiplayerComponent>()->m_processInputCallback = processInputCallback;

        AzNetworking::PacketEncodingBuffer buffer;
        controller->HandleSendClientInputCorrection(&connection, HostFrameId(0), ClientInputId(LargeCorrectionInputId), buffer);

        // The total number of corrections processed should be the number of inputs generated *past* the id we sent in the correction for
        EXPECT_EQ(numInputCorrectionsProcessed, desiredInputCount - static_cast<uint64_t>(LargeCorrectionInputId));
    }

} // namespace Multiplayer
