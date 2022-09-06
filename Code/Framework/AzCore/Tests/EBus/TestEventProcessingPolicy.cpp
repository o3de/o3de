/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/IEventScheduler.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/LoggerSystemComponent.h>
#include <AzCore/Time/TimeSystem.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    class EventProcessingPolicyTests
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            AZ::NameDictionary::Create();
        }

        void TearDown() override
        {
            AZ::NameDictionary::Destroy();
            TeardownAllocator();
        }

    };

    struct ITestEventProcessingReporter
    {
        AZ_RTTI(ITestEventProcessingReporter, "{3F4EBD7C-F2F9-4379-8DC1-5115113FD81A}");

        virtual void Report(const char* item) = 0;
    };

    struct TestEventProcessingPolicy
    {
        template<class Interface>
        static void ReportIface(Interface&& iface)
        {
            if (auto* reporter = AZ::Interface<ITestEventProcessingReporter>::Get(); reporter)
            {
                reporter->Report(iface->RTTI_GetTypeName());
            }
        }

        template<class Results, class Function, class Interface, class... InputArgs>
        static void CallResult(Results& results, Function&& func, Interface&& iface, InputArgs&&... args)
        {
            ReportIface(iface);
            results = AZStd::invoke(AZStd::forward<Function>(func), AZStd::forward<Interface>(iface), AZStd::forward<InputArgs>(args)...);
        }

        template<class Function, class Interface, class... InputArgs>
        static void Call(Function&& func, Interface&& iface, InputArgs&&... args)
        {
            ReportIface(iface);
            AZStd::invoke(AZStd::forward<Function>(func), AZStd::forward<Interface>(iface), AZStd::forward<InputArgs>(args)...);
        }
    };

    struct ITestHookEvents
    {
        AZ_RTTI(ITestHookEvents, "{349C00E8-65C7-4FF6-B505-E047238618CC}");

        virtual void Contribute(AZStd::vector<AZStd::string>& stack) const = 0;
    };

    struct TestHookEventTraits
        : public AZ::EBusTraits
    {
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using EventProcessingPolicy = TestEventProcessingPolicy;
    };

    using TestHookEventBus = AZ::EBus<ITestHookEvents, TestHookEventTraits>;

    struct TestOneEventHook
        : public TestHookEventBus::Handler
    {
        AZ_RTTI(TestOneEventHook, "{BC6F4FCD-F035-4527-9262-7582ACE33EDF}", TestHookEventBus::Handler);

        TestOneEventHook()
        {
            TestHookEventBus::Handler::BusConnect();
        }

        ~TestOneEventHook()
        {
            TestHookEventBus::Handler::BusDisconnect();
        }

        void Contribute(AZStd::vector<AZStd::string>& stack) const override
        {
            stack.push_back("TestOneEventHook");
        }

        const char* Type()
        {
            return "TestOneEventHook";
        }
    };

    struct TestTwoEventHook
        : public TestHookEventBus::Handler
    {
        AZ_RTTI(TestTwoEventHook, "{E6C52A54-56DA-4BB7-BEA5-B1CF07640A31}", TestHookEventBus::Handler);

        TestTwoEventHook()
        {
            TestHookEventBus::Handler::BusConnect();
        }

        ~TestTwoEventHook()
        {
            TestHookEventBus::Handler::BusDisconnect();
        }

        void Contribute(AZStd::vector<AZStd::string>& stack) const override
        {
            stack.push_back("TestTwoEventHook");
        }

        const char* Type()
        {
            return "TestTwoEventHook";
        }
    };

    struct TestReporter final
        : public ITestEventProcessingReporter
    {
        TestReporter()
        {
            AZ::Interface<ITestEventProcessingReporter>::Register(this);
        }

        ~TestReporter()
        {
            AZ::Interface<ITestEventProcessingReporter>::Unregister(this);
        }

        void Report(const char* item) override
        {
            m_reports.push_back(item);
        }

        AZStd::vector<AZStd::string> m_reports;
    };

    TEST_F(EventProcessingPolicyTests, SimplePolicy_Works)
    {
        TestOneEventHook testOneEventHook;
        TestTwoEventHook testTwoEventHook;
        TestReporter testReporter;

        AZStd::vector<AZStd::string> data;
        TestHookEventBus::Broadcast(&TestHookEventBus::Events::Contribute, data);

        EXPECT_EQ(data.size(), 2);
        EXPECT_EQ(testReporter.m_reports.size(), 2);
    }
}
