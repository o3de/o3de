/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/PythonCommon.h>
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include "PythonTraceMessageSink.h"
#include "PythonTestingUtility.h"

#include <Source/PythonSystemComponent.h>
#include <Source/PythonReflectionComponent.h>
#include <Source/PythonProxyBus.h>
#include <Source/PythonProxyObject.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace UnitTest
{
    //////////////////////////////////////////////////////////////////////////
    // test class/struts

    class FakeComponentId
    {
    public:
        AZ_TYPE_INFO(FakeComponentId, "{A0A9A069-9C3D-465A-B7AD-0D6CC803990A}");
        AZ_CLASS_ALLOCATOR(FakeComponentId, AZ::SystemAllocator, 0);

        FakeComponentId() = default;
        bool operator==(const FakeComponentId& rhs) const { return m_id == rhs.m_id; }
        bool IsValid() const { return m_id != AZ::InvalidComponentId; }
        AZStd::string ToString() const { return AZStd::string::format("[%llu]", m_id); }

        void Set(AZ::u64 id) 
        { 
            m_id = id;
        }

        AZ::ComponentId m_id = AZ::InvalidComponentId;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<FakeComponentId>()
                    ->Version(1)
                    ->Field("ComponentId", &FakeComponentId::m_id)
                    ;

                serializeContext->RegisterGenericType<AZStd::vector<FakeComponentId>>();
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<FakeComponentId>("FakeComponentId")
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "entity")
                    ->Constructor()
                    ->Method("IsValid", &FakeComponentId::IsValid)
                    ->Method("Equal", &FakeComponentId::operator==)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                    ->Method("ToString", &FakeComponentId::ToString)
                    ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                    ->Method("Set", &FakeComponentId::Set)
                    ;
            }
        }
    };

    struct PythonTestBroadcastRequests
        : AZ::EBusTraits
    {
        static const bool EnableEventQueue = true;
        virtual AZ::u32 GetBits() = 0;
        virtual void SetBits(AZ::u32 value) = 0;
        virtual void Ping() = 0;
        virtual void AcceptProxyList(const AZStd::vector<FakeComponentId>& componentIds) = 0;
    };
    using PythonTestBroadcastRequestBus = AZ::EBus<PythonTestBroadcastRequests>;

    struct PythonTestBroadcastRequestsHandler final
        : public PythonTestBroadcastRequestBus::Handler
    {
        PythonTestBroadcastRequestsHandler()
        {
            PythonTestBroadcastRequestBus::Handler::BusConnect();
        }

        virtual ~PythonTestBroadcastRequestsHandler()
        {
            PythonTestBroadcastRequestBus::Handler::BusDisconnect();
        }

        AZ::u32 m_bits = 0;

        AZ::u32 GetBits() override
        {
            return m_bits;
        }

        void SetBits(AZ::u32 value) override
        {
            m_bits |= value;
        }

        AZ::u64 m_pingCount = 0;

        void Ping() override
        {
            ++m_pingCount;
        }

        void AcceptProxyList(const AZStd::vector<FakeComponentId>& componentIds) override
        {
            AZStd::vector<AZ::Component*> components;
            for (auto componentId : componentIds)
            {
                if (componentId.IsValid())
                {
                    AZ_Printf("python", "BasicRequests_AcceptProxyList:%s", componentId.ToString().c_str());
                }
                else
                {
                    AZ_Warning("python", false, "AcceptProxyList failed - found invalid componentId.");
                }
            }
        }

        void Reflect(AZ::ReflectContext* context)
        {
            FakeComponentId::Reflect(context);

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PythonTestBroadcastRequestBus>("PythonTestBroadcastRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Event("SetBits", &PythonTestBroadcastRequestBus::Events::SetBits)
                    ->Event("GetBits", &PythonTestBroadcastRequestBus::Events::GetBits)
                    ->Event("Ping", &PythonTestBroadcastRequestBus::Events::Ping)
                    ->Event("AcceptProxyList", &PythonTestBroadcastRequestBus::Events::AcceptProxyList)
                    ;
            }
        }
    };

    //

    struct PythonTestEventRequests
        : AZ::EBusTraits
    {
        static const bool EnableEventQueue = true;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::u32;

        virtual AZ::s32 Add(AZ::s32 a, AZ::s32 b) = 0;
        virtual void Pong() = 0;
    };
    using PythonTestEventRequestBus = AZ::EBus<PythonTestEventRequests>;

    struct PythonTestEventRequestsHandler final
        : public PythonTestEventRequestBus::Handler
    {
        PythonTestEventRequestsHandler()
        {
            PythonTestEventRequestBus::Handler::BusConnect(101);
        }

        virtual ~PythonTestEventRequestsHandler()
        {
            PythonTestEventRequestBus::Handler::BusDisconnect();
        }

        AZ::s32 Add(AZ::s32 a, AZ::s32 b) override
        {
            return a + b;
        }

        AZ::u64 m_pongCount = 0;

        void Pong() override
        {
            ++m_pongCount;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PythonTestEventRequestBus>("PythonTestEventRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Event("Add", &PythonTestEventRequestBus::Events::Add)
                    ->Event("Pong", &PythonTestEventRequestBus::Events::Pong)
                    ;
            }
        }
    };

    // an example of an EBus Notification bus using a single address & BusIdType=NullBusId

    struct PythonTestSingleAddressNotifications
        : AZ::EBusTraits
    {
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using MutexType = AZStd::mutex;
        virtual ~PythonTestSingleAddressNotifications() = default;
        virtual void OnPing(AZ::u64 count) = 0;
        virtual void OnPong(AZ::u64 count) = 0;
        virtual void MultipleInputs(AZ::u64 one, AZ::s8 two, AZStd::string_view three) = 0;
        virtual AZStd::string OnAddFish(AZStd::string_view value) = 0;
        virtual void OnFire() = 0;
    };
    using PythonTestSingleAddressNotificationBus = AZ::EBus<PythonTestSingleAddressNotifications>;

    struct PythonTestNotificationHandler final
        : public PythonTestSingleAddressNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
        AZ_EBUS_BEHAVIOR_BINDER(PythonTestNotificationHandler, "{97052D15-A4E8-461B-B065-91D16E31C4F7}", AZ::SystemAllocator, 
            OnPing, OnPong, MultipleInputs, OnAddFish, OnFire);

        virtual ~PythonTestNotificationHandler() = default;

        void OnPing(AZ::u64 count) override
        {
            Call(FN_OnPing, count);
        }

        void OnPong(AZ::u64 count) override
        {
            Call(FN_OnPong, count);
        }

        void MultipleInputs(AZ::u64 one, AZ::s8 two, AZStd::string_view three) override
        {
            Call(FN_MultipleInputs, one, two, three);
        }

        AZStd::string OnAddFish(AZStd::string_view value) override
        {
            AZStd::string result;
            CallResult(result, FN_OnAddFish, value);
            return result;
        }

        void OnFire() override
        {
            Call(FN_OnFire);
        }

        static AZ::u64 s_pongCount;
        static AZ::u64 s_pingCount;

        static void DoPing()
        {
            // notify the listeners about Ping
            ++s_pingCount;
            PythonTestSingleAddressNotificationBus::Broadcast(&PythonTestSingleAddressNotificationBus::Events::OnPing, s_pingCount);
        }

        static void DoPong()
        {
            // notify the listeners about Pong
            ++s_pongCount;
            PythonTestSingleAddressNotificationBus::Broadcast(&PythonTestSingleAddressNotificationBus::Events::OnPong, s_pongCount);
        }

        static AZStd::string DoAddFish(AZStd::string value)
        {
            AZStd::string result;
            PythonTestSingleAddressNotificationBus::BroadcastResult(result, &PythonTestSingleAddressNotificationBus::Events::OnAddFish, value);
            return result;
        }

        static void DoFire()
        {
            PythonTestSingleAddressNotificationBus::Broadcast(&PythonTestSingleAddressNotificationBus::Events::OnFire);
        }

        static void DoFiresInParallel(int value)
        {
            AZStd::vector<AZStd::thread> threads;
            threads.reserve(value);
            for (size_t i = 0; i < value; ++i)
            {
                threads.emplace_back(&DoFire);
            }
            for (AZStd::thread& thread : threads)
            {
                thread.join();
            }
        }

        static void Reset()
        {
            s_pingCount = 0;
            s_pongCount = 0;
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PythonTestSingleAddressNotificationBus>("PythonTestSingleAddressNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Handler<PythonTestNotificationHandler>()
                    ->Event("on_ping", &PythonTestSingleAddressNotificationBus::Events::OnPing)
                    ->Event("on_pong", &PythonTestSingleAddressNotificationBus::Events::OnPong)
                    ->Event("MultipleInputs", &PythonTestSingleAddressNotificationBus::Events::MultipleInputs)
                    ->Event("OnAddFish", &PythonTestSingleAddressNotificationBus::Events::OnAddFish)
                    ->Event("OnFire", &PythonTestSingleAddressNotificationBus::Events::OnFire)
                    ;

                // for testing from Python to send out the events
                behaviorContext->Class<PythonTestNotificationHandler>()
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "test")
                    ->Method("do_ping", &PythonTestNotificationHandler::DoPing)
                    ->Method("do_pong", &PythonTestNotificationHandler::DoPong)
                    ->Method("do_add_fish", &PythonTestNotificationHandler::DoAddFish)
                    ->Method("do_fire", &PythonTestNotificationHandler::DoFire)
                    ->Method("do_fires_in_parallel", &PythonTestNotificationHandler::DoFiresInParallel)
                    ;
            }
        }
    };
    AZ::u64 PythonTestNotificationHandler::s_pongCount = 0;
    AZ::u64 PythonTestNotificationHandler::s_pingCount = 0;

    // an example of an EBus Notification bus connecting to a bus by id

    struct PythonTestByIdNotifications
        : public AZ::EBusTraits
    {
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AZ::s32;

        virtual void OnResult(AZ::s64 result) = 0;
    };
    using PythonTestByIdNotificationBus = AZ::EBus<PythonTestByIdNotifications>;

    struct PythonTestByIdNotificationsHandler final
        : public PythonTestByIdNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
        AZ_EBUS_BEHAVIOR_BINDER(PythonTestByIdNotificationsHandler, "{5F091D4B-86C4-4D25-B982-2ECAFD8AFF0F}", AZ::SystemAllocator, OnResult);
        virtual ~PythonTestByIdNotificationsHandler() = default;
        void OnResult(AZ::s64 result) override
        {
            Call(FN_OnResult, result);
        }

        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<PythonTestByIdNotificationBus>("PythonTestByIdNotificationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Handler<PythonTestByIdNotificationsHandler>()
                    ->Event("OnResult", &PythonTestByIdNotificationBus::Events::OnResult)
                    ;
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // fixture

    struct PythonBusProxyTests
        : public PythonTestingFixture
    {
        PythonTraceMessageSink m_testSink;

        void SetUp() override
        {
            PythonTestingFixture::SetUp();
            PythonTestingFixture::RegisterComponentDescriptors();
        }

        void TearDown() override
        {
            // clearing up memory
            m_testSink.CleanUp();
            PythonTestingFixture::TearDown();
        }
    };

    //////////////////////////////////////////////////////////////////////////
    // tests

    TEST_F(PythonBusProxyTests, ImportEbus)
    {
        enum class LogTypes
        {
            Skip = 0,
            BasicRequests_ImportEbus,
            BasicRequests_ImportEbusCount, 
            BasicRequests_AcceptProxyList
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "BasicRequests_ImportEbus"))
                {
                    return static_cast<int>(LogTypes::BasicRequests_ImportEbus);
                }
                else if (AzFramework::StringFunc::Equal(message, "BasicRequests_ImportEbusCount"))
                {
                    return static_cast<int>(LogTypes::BasicRequests_ImportEbusCount);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "BasicRequests_AcceptProxyList"))
                {
                    return static_cast<int>(LogTypes::BasicRequests_AcceptProxyList);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonTestBroadcastRequestsHandler pythonTestBroadcastRequestsHandler;
        pythonTestBroadcastRequestsHandler.Reflect(m_app.GetBehaviorContext());
        pythonTestBroadcastRequestsHandler.Reflect(m_app.GetSerializeContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                import azlmbr.entity
                import azlmbr.object

                eventType = azlmbr.bus.Event
                if (eventType != None):
                    print ('BasicRequests_ImportEbus')

                if len(azlmbr.bus.__dict__) > 0:
                    print ('BasicRequests_ImportEbusCount')

                componentId101 = azlmbr.object.create('FakeComponentId')
                componentId101.Set(101)
                componentId102 = azlmbr.object.create('FakeComponentId')
                componentId102.Set(102)
                componentList = [componentId101, componentId102]
                azlmbr.bus.PythonTestBroadcastRequestBus(azlmbr.bus.Broadcast, 'AcceptProxyList', componentList)
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::BasicRequests_ImportEbus)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::BasicRequests_ImportEbusCount)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::BasicRequests_AcceptProxyList)]);
    }

    TEST_F(PythonBusProxyTests, BroadcastRequests)
    {
        enum class LogTypes
        {
            Skip = 0,
            BroadcastRequests_SetBits,
            BroadcastRequests_GetBits
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "BroadcastRequests_SetBits"))
                {
                    return static_cast<int>(LogTypes::BroadcastRequests_SetBits);
                }
                else if (AzFramework::StringFunc::Equal(message, "BroadcastRequests_GetBits"))
                {
                    return static_cast<int>(LogTypes::BroadcastRequests_GetBits);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonTestBroadcastRequestsHandler pythonTestBroadcastRequestsHandler;
        pythonTestBroadcastRequestsHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                bits = azlmbr.bus.PythonTestBroadcastRequestBus(azlmbr.bus.Broadcast, 'GetBits')
                if (bits == 0):
                    print ('BroadcastRequests_GetBits')
                    azlmbr.bus.PythonTestBroadcastRequestBus(azlmbr.bus.Broadcast, 'SetBits', bits | 3)
                    bits = azlmbr.bus.PythonTestBroadcastRequestBus(azlmbr.bus.Broadcast, 'GetBits')
                    if (bits == 3):
                        print ('BroadcastRequests_SetBits')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::BroadcastRequests_SetBits)]);
        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::BroadcastRequests_GetBits)]);
    }

    TEST_F(PythonBusProxyTests, QueueBroadcastRequests)
    {
        PythonTestBroadcastRequestsHandler pythonTestBroadcastRequestsHandler;
        pythonTestBroadcastRequestsHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                for i in range(2019):
                    azlmbr.bus.PythonTestBroadcastRequestBus(azlmbr.bus.QueueBroadcast, 'Ping')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }

        EXPECT_EQ(0, pythonTestBroadcastRequestsHandler.m_pingCount);
        PythonTestBroadcastRequestBus::ExecuteQueuedEvents();
        EXPECT_EQ(2019, pythonTestBroadcastRequestsHandler.m_pingCount);

        e.Deactivate();
    }

    TEST_F(PythonBusProxyTests, EventRequests)
    {
        enum class LogTypes
        {
            Skip = 0,
            EventRequests_Add
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "EventRequests_Add"))
                {
                    return static_cast<int>(LogTypes::EventRequests_Add);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        PythonTestEventRequestsHandler pythonTestEventRequestsHandler;
        pythonTestEventRequestsHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                import azlmbr.test
                address = 101
                answer = azlmbr.test.PythonTestEventRequestBus(azlmbr.bus.Event, 'Add', address, 40, 2)
                if (answer == 42):
                    print ('EventRequests_Add')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::EventRequests_Add)]);
    }

    TEST_F(PythonBusProxyTests, QueueEventRequests)
    {
        PythonTestEventRequestsHandler pythonTestEventRequestsHandler;
        pythonTestEventRequestsHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                import azlmbr.test
                address = 101
                for i in range(address * 2):
                    azlmbr.test.PythonTestEventRequestBus(azlmbr.bus.QueueEvent, 'Pong', address)
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }

        EXPECT_EQ(0, pythonTestEventRequestsHandler.m_pongCount);
        PythonTestEventRequestBus::ExecuteQueuedEvents();
        EXPECT_EQ(202, pythonTestEventRequestsHandler.m_pongCount);

        e.Deactivate();
    }

    TEST_F(PythonBusProxyTests, SingleAddressNotifications)
    {
        PythonTestNotificationHandler pythonTestNotificationHandler;
        pythonTestNotificationHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            Notifications_OnPing, 
            Notifications_OnPong, 
            Notifications_Match,
            Notifications_Multi,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "Notifications_OnPing"))
                {
                    return static_cast<int>(LogTypes::Notifications_OnPing);
                }
                else if (AzFramework::StringFunc::Equal(message, "Notifications_OnPong"))
                {
                    return static_cast<int>(LogTypes::Notifications_OnPong);
                }
                else if (AzFramework::StringFunc::Equal(message, "Notifications_Match"))
                {
                    return static_cast<int>(LogTypes::Notifications_Match);
                }
                else if (AzFramework::StringFunc::StartsWith(message, "Notifications_Multi"))
                {
                    return static_cast<int>(LogTypes::Notifications_Multi);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        UnitTest::PythonTestNotificationHandler::Reset();
        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                import azlmbr.test

                pingCount = 0
                pongCount = 0

                def OnPing(parameters):
                    global pingCount
                    pingCount = parameters[0]
                    print ('Notifications_OnPing')

                def OnPong(parameters):
                    global pongCount
                    pongCount = parameters[0]
                    print ('Notifications_OnPong')

                def OnMultipleInputs(parameters):
                    if(len(parameters) == 3):
                        print ('Notifications_Multi1')
                    if(parameters[0] == 1):
                        print ('Notifications_Multi2')
                    if(parameters[1] == 2):
                        print ('Notifications_Multi3')
                    if(parameters[2] == '3'):
                        print ('Notifications_Multi4')

                handler = azlmbr.bus.NotificationHandler('PythonTestSingleAddressNotificationBus')
                handler.connect(None)
                handler.add_callback('OnPing', OnPing)
                handler.add_callback('OnPong', OnPong)
                handler.add_callback('MultipleInputs', OnMultipleInputs)

                azlmbr.test.PythonTestSingleAddressNotificationBus(azlmbr.bus.Broadcast, 'MultipleInputs', 1, 2, '3')

                for i in range(40):
                    azlmbr.test.PythonTestNotificationHandler_do_ping()

                for i in range(2):
                    azlmbr.test.PythonTestNotificationHandler_do_pong()

                if (pingCount == 40):
                    print ('Notifications_Match')

                if (pongCount == 2):
                    print ('Notifications_Match')

                if ((pingCount + pongCount) == 42):
                    print ('Notifications_Match')

                handler.disconnect()

                def OnMultipleInputsAgain(parameters):
                    if(len(parameters) == 3):
                        print ('Notifications_Multi5')
                    if(parameters[0] == 4):
                        print ('Notifications_Multi6')
                    if(parameters[1] == 5):
                        print ('Notifications_Multi7')
                    if(parameters[2] == 'six'):
                        print ('Notifications_Multi8')

                handler = azlmbr.test.PythonTestSingleAddressNotificationBusHandler()
                handler.connect(None)
                handler.add_callback('MultipleInputs', OnMultipleInputsAgain)

                azlmbr.test.PythonTestSingleAddressNotificationBus(azlmbr.bus.Broadcast, 'MultipleInputs', 4, 5, 'six')
                handler.disconnect()
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }
        e.Deactivate();

        EXPECT_EQ(40, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::Notifications_OnPing)]);
        EXPECT_EQ(2, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::Notifications_OnPong)]);
        EXPECT_EQ(3, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::Notifications_Match)]);
        EXPECT_EQ(8, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::Notifications_Multi)]);
    }

    TEST_F(PythonBusProxyTests, NotificationsAtAddress)
    {
        PythonTestByIdNotificationsHandler pythonTestByIdNotificationsHandler;
        pythonTestByIdNotificationsHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            AtAddress_Match
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "AtAddress_Match"))
                {
                    return static_cast<int>(LogTypes::AtAddress_Match);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                import azlmbr.default

                answer = 0

                def OnResult(parameters):
                    global answer
                    answer = int(parameters[0])

                handler = azlmbr.bus.NotificationHandler('PythonTestByIdNotificationBus')
                handler.connect(101)
                handler.add_callback('OnResult', OnResult)

                address = 101
                result = 40 + 2
                azlmbr.bus.PythonTestByIdNotificationBus(azlmbr.bus.Event, 'OnResult', address, result)

                if (answer == 42):
                    print ('AtAddress_Match')

                handler.disconnect()
                azlmbr.bus.PythonTestByIdNotificationBus(azlmbr.bus.Event, 'OnResult', address, 2)

                if (answer == 42):
                    print ('AtAddress_Match')
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }
        e.Deactivate();

        EXPECT_EQ(2, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::AtAddress_Match)]);
    }

    TEST_F(PythonBusProxyTests, SingleAddressNotifications_InParallel_Errors)
    {
        PythonTestNotificationHandler pythonTestNotificationHandler;
        pythonTestNotificationHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            Notifications_OnFire,
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "Notifications_OnFire"))
                {
                    return static_cast<int>(LogTypes::Notifications_OnFire);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        UnitTest::PythonTestNotificationHandler::Reset();

        const int numFiresInParallel = 220;

        const AZStd::string script = AZStd::string::format(
            "import azlmbr.bus\n"
            "import azlmbr.test\n"
            "\n"
            "def OnFire(parameters) :\n"
            "    print('Notifications_OnFire')\n"
            "\n"
            "handler = azlmbr.bus.NotificationHandler('PythonTestSingleAddressNotificationBus')\n"
            "handler.connect(None)\n"
            "handler.add_callback('OnFire', OnFire)\n"
            "\n"
            "azlmbr.test.PythonTestNotificationHandler_do_fire()\n"
            "\n"
            "azlmbr.test.PythonTestNotificationHandler_do_fires_in_parallel(%d)\n"
            "\n"
            "handler.disconnect()\n",
            numFiresInParallel);

        AZ_TEST_START_TRACE_SUPPRESSION;
        AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(&AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByString, script, false);
        AZ_TEST_STOP_TRACE_SUPPRESSION(numFiresInParallel); // Expect numFiresInParallel errors

        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::Notifications_OnFire)]);
    }

    TEST_F(PythonBusProxyTests, NotificationsWithNoAddress)
    {
        PythonTestNotificationHandler pythonTestNotificationHandler;
        pythonTestNotificationHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);

        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            NoAddressConnect
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::Equal(message, "NoAddressConnect"))
                {
                    return static_cast<int>(LogTypes::NoAddressConnect);
                }
            }
            return static_cast<int>(LogTypes::Skip);
        };

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                import azlmbr.test

                def on_ping(args):
                    print('NoAddressConnect')

                handler = azlmbr.test.PythonTestSingleAddressNotificationBusHandler()
                handler.connect()
                handler.add_callback('OnPing', on_ping)

                azlmbr.test.PythonTestNotificationHandler_do_ping()

                handler.disconnect()
                azlmbr.test.PythonTestNotificationHandler_do_ping()
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Warning("UnitTest", false, "Failed on with Python exception: %s", e.what());
            FAIL();
        }
        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::NoAddressConnect)]);
    }

    TEST_F(PythonBusProxyTests, NotificationsWithResult)
    {
        PythonTestNotificationHandler pythonTestNotificationHandler;
        pythonTestNotificationHandler.Reflect(m_app.GetBehaviorContext());

        AZ::Entity e;
        Activate(e);
        SimulateEditorBecomingInitialized();

        enum class LogTypes
        {
            Skip = 0,
            WithResult
        };

        m_testSink.m_evaluateMessage = [](const char* window, const char* message) -> int
        {
            if (AzFramework::StringFunc::Equal(window, "python"))
            {
                if (AzFramework::StringFunc::StartsWith(message, "WithResult"))
                {
                    return aznumeric_cast<int>(LogTypes::WithResult);
                }
            }
            return aznumeric_cast<int>(LogTypes::Skip);
        };

        try
        {
            pybind11::exec(R"(
                import azlmbr.bus
                import azlmbr.test

                def on_add_fish(args):
                    value = args[0] + 'fish'
                    return value

                handler = azlmbr.test.PythonTestSingleAddressNotificationBusHandler()
                handler.connect()
                handler.add_callback('OnAddFish', on_add_fish)

                babblefish = azlmbr.test.PythonTestNotificationHandler_do_add_fish('babble')
                if (babblefish == 'babblefish'):
                    print('WithResult_babblefish')

                handler.disconnect()
            )");
        }
        catch ([[maybe_unused]] const std::exception& e)
        {
            AZ_Error("UnitTest", false, "Failed on with Python exception: %s", e.what());
        }
        e.Deactivate();

        EXPECT_EQ(1, m_testSink.m_evaluationMap[static_cast<int>(LogTypes::WithResult)]);
    }}
