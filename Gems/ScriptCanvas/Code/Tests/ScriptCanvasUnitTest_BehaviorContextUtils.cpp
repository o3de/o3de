/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Utils/BehaviorContextUtils.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    namespace BehaviorContextUtilsUnitTestStructures
    {
        class TestHandler
            : public AZ::ComponentApplicationBus::Handler
        {
        public:
            AZ::BehaviorContext* m_behaviorContext;

            // ComponentApplicationBus
            AZ::ComponentApplication* GetApplication() override { return nullptr; }
            void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}
            void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}
            bool AddEntity(AZ::Entity*) override { return true; }
            bool RemoveEntity(AZ::Entity*) override { return true; }
            bool DeleteEntity(const AZ::EntityId&) override { return true; }
            AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
            AZ::SerializeContext* GetSerializeContext() override { return nullptr; }
            AZ::BehaviorContext*  GetBehaviorContext() override { return m_behaviorContext; }
            AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
            const char* GetExecutableFolder() const override { return nullptr; }
            const char* GetBinFolder() const { return nullptr; }
            const char* GetAppRoot() override { return nullptr; }
            AZ::Debug::DrillerManager* GetDrillerManager() override { return nullptr; }
            void EnumerateEntities(const EntityCallback& /*callback*/) override {}

            void Init(AZ::BehaviorContext* behaviorContext)
            {
                m_behaviorContext = behaviorContext;
            }

            void Activate()
            {
                AZ::ComponentApplicationBus::Handler::BusConnect();
            }

            void Deactivate()
            {
                AZ::ComponentApplicationBus::Handler::BusDisconnect();
            }
        };

        class TestClass
        {
        public:
            AZ_TYPE_INFO(TestClass, "{A69035EF-F79F-4B1F-A192-5AB173C3B1F8}");
        };

        class TestRequest : public AZ::EBusTraits
        {
        public:
            void TestMethod1() {}
            void TestMethod2() {}
        };
        using TestEBus = AZ::EBus<TestRequest>;
    };

    class ScriptCanvasBehaviorContextUtilsUnitTestFixture
        : public ScriptCanvasUnitTestFixture
    {
    protected:
        AZ::BehaviorContext* m_behaviorContext;
        BehaviorContextUtilsUnitTestStructures::TestHandler* m_testHandler;

        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            m_behaviorContext = aznew AZ::BehaviorContext();
            m_testHandler = new BehaviorContextUtilsUnitTestStructures::TestHandler();
            m_testHandler->Init(m_behaviorContext);
            m_testHandler->Activate();
        };

        void TearDown() override
        {
            m_testHandler->Deactivate();
            delete m_testHandler;

            delete m_behaviorContext;

            ScriptCanvasUnitTestFixture::TearDown();
        };
    };

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindFree_ReturnFalse_BehaviorContextNotFound)
    {
        m_testHandler->m_behaviorContext = nullptr;
        const AZ::BehaviorMethod* dummyPtr{};
        auto actualResult = BehaviorContextUtils::FindFree(dummyPtr, "DummyMethodName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindFree_ReturnFalse_NoMatchingMethodFound)
    {
        const AZ::BehaviorMethod* dummyPtr{};
        auto actualResult = BehaviorContextUtils::FindFree(dummyPtr, "DummyMethodName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindFree_ReturnTrue_MatchingMethodFound)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        const AZ::BehaviorMethod* methodPtr{};
        auto actualResult = BehaviorContextUtils::FindFree(methodPtr, "TestMethod");
        EXPECT_TRUE(actualResult);
        EXPECT_EQ(methodPtr, m_behaviorContext->m_methods["TestMethod"]);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEBus_ReturnFalse_BehaviorContextNotFound)
    {
        m_testHandler->m_behaviorContext = nullptr;
        const AZ::BehaviorEBus* dummyPtr{};
        auto actualResult = BehaviorContextUtils::FindEBus(dummyPtr, "DummyEBusName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEBus_ReturnFalse_NoMatchingEBusFound)
    {
        const AZ::BehaviorEBus* dummyPtr{};
        auto actualResult = BehaviorContextUtils::FindEBus(dummyPtr, "DummyEBusName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEBus_ReturnFalse_MatchingEBusFound)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus");
        const AZ::BehaviorEBus* ebusPtr{};
        auto actualResult = BehaviorContextUtils::FindEBus(ebusPtr, "TestEBus");
        EXPECT_TRUE(actualResult);
        EXPECT_EQ(ebusPtr, m_behaviorContext->m_ebuses["TestEBus"]);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEvent_ReturnFalse_BehaviorContextNotFound)
    {
        m_testHandler->m_behaviorContext = nullptr;
        const AZ::BehaviorMethod* dummyPtr{};
        auto actualResult = BehaviorContextUtils::FindEvent(dummyPtr, "DummyEBusName", "DummyEventName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEvent_ReturnFalse_NoMatchingEBusFound)
    {
        const AZ::BehaviorMethod* dummyPtr{};
        auto actualResult = BehaviorContextUtils::FindEvent(dummyPtr, "DummyEBusName", "DummyEventName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEvent_ReturnFalse_NoMatchingEventFound)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod1", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod1);
        const AZ::BehaviorMethod* dummyPtr{};
        auto actualResult = BehaviorContextUtils::FindEvent(dummyPtr, "TestEBus", "DummyEventName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEvent_ReturnTrue_MatchingEventFound)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod1", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod1);
        const AZ::BehaviorMethod* methodPtr{};
        auto actualResult = BehaviorContextUtils::FindEvent(methodPtr, "TestEBus", "TestMethod1");
        EXPECT_TRUE(actualResult);
        EXPECT_EQ(methodPtr, m_behaviorContext->m_ebuses["TestEBus"]->m_events["TestMethod1"].m_broadcast);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindEvent_GetBroadcastEventType_MatchingEventFound)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod1", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod1);
        const AZ::BehaviorMethod* methodPtr{};
        EventType eventType;
        auto actualResult = BehaviorContextUtils::FindEvent(methodPtr, "TestEBus", "TestMethod1", &eventType);
        EXPECT_EQ(eventType, EventType::Broadcast);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindClass_ReturnFalse_BehaviorContextNotFound)
    {
        m_testHandler->m_behaviorContext = nullptr;
        const AZ::BehaviorClass* dummyClassPtr{};
        const AZ::BehaviorMethod* dummyMethodPtr{};
        auto actualResult = BehaviorContextUtils::FindClass(dummyMethodPtr, dummyClassPtr, "DummyClassName", "DummyMethodName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindClass_ReturnFalse_NoMatchingClassFound)
    {
        const AZ::BehaviorClass* dummyClassPtr{};
        const AZ::BehaviorMethod* dummyMethodPtr{};
        auto actualResult = BehaviorContextUtils::FindClass(dummyMethodPtr, dummyClassPtr, "DummyClassName", "DummyMethodName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindClass_ReturnFalse_NoMatchingClassMethodFound)
    {
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass")
            ->Method("TestClassMethod", []() {});
        const AZ::BehaviorClass* dummyClassPtr{};
        const AZ::BehaviorMethod* dummyMethodPtr{};
        auto actualResult = BehaviorContextUtils::FindClass(dummyMethodPtr, dummyClassPtr, "TestClass", "DummyMethodName");
        EXPECT_FALSE(actualResult);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindClass_ReturnTrue_MatchingClassMethodFound)
    {
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass")
            ->Method("TestClassMethod", []() {});
        const AZ::BehaviorClass* classPtr{};
        const AZ::BehaviorMethod* methodPtr{};
        auto actualResult = BehaviorContextUtils::FindClass(methodPtr, classPtr, "TestClass", "TestClassMethod");
        EXPECT_TRUE(actualResult);
        EXPECT_EQ(classPtr, m_behaviorContext->m_classes["TestClass"]);
        EXPECT_EQ(methodPtr, m_behaviorContext->m_classes["TestClass"]->m_methods["TestClassMethod"]);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, FindClass_GetPrettyClassName_MatchingClassMethodFound)
    {
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass")
            ->Method("TestClassMethod", []() {});
        const AZ::BehaviorClass* classPtr{};
        const AZ::BehaviorMethod* methodPtr{};
        AZStd::string prettyClassName;
        auto actualResult = BehaviorContextUtils::FindClass(methodPtr, classPtr, "TestClass", "TestClassMethod", &prettyClassName);
        EXPECT_EQ(prettyClassName, "TestClass");
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEBusAddressPolicy_GetSingleAddressPolicy_EBusTypeIdIsNull)
    {
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::Uuid::CreateNull();
        auto actualAddressPolicy = BehaviorContextUtils::GetEBusAddressPolicy(testEbus);
        EXPECT_EQ(actualAddressPolicy, AZ::EBusAddressPolicy::Single);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEBusAddressPolicy_GetSingleAddressPolicy_EBusTypeIdIsVoid)
    {
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<void>::Uuid();
        auto actualAddressPolicy = BehaviorContextUtils::GetEBusAddressPolicy(testEbus);
        EXPECT_EQ(actualAddressPolicy, AZ::EBusAddressPolicy::Single);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEBusAddressPolicy_GetByIdAddressPolicy_EBusTypeIdIsStringType)
    {
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<AZStd::string>::Uuid();
        auto actualAddressPolicy = BehaviorContextUtils::GetEBusAddressPolicy(testEbus);
        EXPECT_EQ(actualAddressPolicy, AZ::EBusAddressPolicy::ById);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventMethod_GetQueueEventMethod_EBusHasQueueFunctionAndAddressTypeIsById)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<AZStd::string>::Uuid();
        testEbus.m_queueFunction = m_behaviorContext->m_methods["TestMethod"];
        AZ::BehaviorEBusEventSender testEventSender;
        testEventSender.m_queueEvent = m_behaviorContext->m_methods["TestMethod"];
        auto actualMethod = BehaviorContextUtils::GetEventMethod(testEbus, testEventSender);
        EXPECT_EQ(actualMethod, m_behaviorContext->m_methods["TestMethod"]);

        testEbus.m_queueFunction = nullptr;
        testEventSender.m_queueEvent = nullptr;
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventMethod_GetQueueBroadcastMethod_EBusHasQueueFunctionAndAddressTypeIsSingle)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<void>::Uuid();
        testEbus.m_queueFunction = m_behaviorContext->m_methods["TestMethod"];
        AZ::BehaviorEBusEventSender testEventSender;
        testEventSender.m_queueBroadcast = m_behaviorContext->m_methods["TestMethod"];
        auto actualMethod = BehaviorContextUtils::GetEventMethod(testEbus, testEventSender);
        EXPECT_EQ(actualMethod, m_behaviorContext->m_methods["TestMethod"]);

        testEbus.m_queueFunction = nullptr;
        testEventSender.m_queueBroadcast = nullptr;
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventMethod_GetEventMethod_EBusHasNoQueueFunctionAndAddressTypeIsById)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<AZStd::string>::Uuid();
        AZ::BehaviorEBusEventSender testEventSender;
        testEventSender.m_event = m_behaviorContext->m_methods["TestMethod"];
        auto actualMethod = BehaviorContextUtils::GetEventMethod(testEbus, testEventSender);
        EXPECT_EQ(actualMethod, m_behaviorContext->m_methods["TestMethod"]);

        testEventSender.m_event = nullptr;
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventMethod_GetBroadcastMethod_EBusHasNoQueueFunctionAndAddressTypeIsSingle)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<void>::Uuid();
        AZ::BehaviorEBusEventSender testEventSender;
        testEventSender.m_broadcast = m_behaviorContext->m_methods["TestMethod"];
        auto actualMethod = BehaviorContextUtils::GetEventMethod(testEbus, testEventSender);
        EXPECT_EQ(actualMethod, m_behaviorContext->m_methods["TestMethod"]);

        testEventSender.m_broadcast = nullptr;
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventType_GetEventQueueType_EBusHasQueueFunctionAndAddressTypeIsById)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<AZStd::string>::Uuid();
        testEbus.m_queueFunction = m_behaviorContext->m_methods["TestMethod"];
        auto actualEventType = BehaviorContextUtils::GetEventType(testEbus);
        EXPECT_EQ(actualEventType, EventType::EventQueue);

        testEbus.m_queueFunction = nullptr;
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventType_GetBroadcastQueueType_EBusHasQueueFunctionAndAddressTypeIsSingle)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<void>::Uuid();
        testEbus.m_queueFunction = m_behaviorContext->m_methods["TestMethod"];
        auto actualEventType = BehaviorContextUtils::GetEventType(testEbus);
        EXPECT_EQ(actualEventType, EventType::BroadcastQueue);

        testEbus.m_queueFunction = nullptr;
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventType_GetEventType_EBusHasNoQueueFunctionAndAddressTypeIsById)
    {
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<AZStd::string>::Uuid();
        auto actualEventType = BehaviorContextUtils::GetEventType(testEbus);
        EXPECT_EQ(actualEventType, EventType::Event);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GetEventType_GetBroadcastType_EBusHasNoQueueFunctionAndAddressTypeIsSingle)
    {
        AZ::BehaviorEBus testEbus;
        testEbus.m_idParam.m_typeId = AZ::AzTypeInfo<void>::Uuid();
        auto actualEventType = BehaviorContextUtils::GetEventType(testEbus);
        EXPECT_EQ(actualEventType, EventType::Broadcast);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GenerateFingerprintForBehaviorContext_ReturnZero_BehaviorContextIsInvalid)
    {
        m_testHandler->m_behaviorContext = nullptr;
        size_t actualHash = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        EXPECT_EQ(actualHash, 0);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GenerateFingerprintForBehaviorContext_ReturnSameHash_SameBehaviorContext)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        size_t actualHash1 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        size_t actualHash2 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GenerateFingerprintForBehaviorContext_ReturnSameHash_BehaviorContextsAreEmpty)
    {
        size_t actualHash1 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        size_t actualHash2 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GenerateFingerprintForBehaviorContext_ReturnDifferentHash_DifferentBehaviorContexts)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        size_t actualHash1 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        delete m_behaviorContext->m_methods["TestMethod"];
        m_behaviorContext->m_methods.clear();
        m_behaviorContext->Method("TestMethod", [](bool) {});
        size_t actualHash2 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GenerateFingerprintForBehaviorContext_ReturnDifferentHash_OneBehaviorContextIsEmpty)
    {
        size_t actualHash1 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        m_behaviorContext->Method("TestMethod", []() {});
        size_t actualHash2 = BehaviorContextUtils::GenerateFingerprintForBehaviorContext();
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GenerateFingerprintForMethod_ReturnSameHash_SameMethod)
    {
        m_behaviorContext->Method("TestMethod", []() {});
        size_t actualHash1 = BehaviorContextUtils::GenerateFingerprintForMethod(MethodType::Free, "", "TestMethod");
        size_t actualHash2 = BehaviorContextUtils::GenerateFingerprintForMethod(MethodType::Free, "", "TestMethod");
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, GenerateFingerprintForMethod_ReturnDifferentHash_DifferentMethod)
    {
        m_behaviorContext->Method("TestMethod1", []() {});
        m_behaviorContext->Method("TestMethod2", [](bool) {});
        size_t actualHash1 = BehaviorContextUtils::GenerateFingerprintForMethod(MethodType::Free, "", "TestMethod1");
        size_t actualHash2 = BehaviorContextUtils::GenerateFingerprintForMethod(MethodType::Free, "", "TestMethod2");
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEBuses_ReturnZero_EbuseMapIsInvalid)
    {
        size_t actualHash = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash, nullptr);
        EXPECT_EQ(actualHash, 0);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEBuses_ReturnSameHash_SameEBusMaps)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod1);
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash1, &(m_behaviorContext->m_ebuses));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash2, &(m_behaviorContext->m_ebuses));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }


    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEBuses_ReturnSameHash_EBusMapsAreEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash1, &(m_behaviorContext->m_ebuses));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash2, &(m_behaviorContext->m_ebuses));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEBuses_ReturnDifferentHash_EBusMapsHaveDifferentEBusName)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus1");
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash1, &(m_behaviorContext->m_ebuses));
        delete m_behaviorContext->m_ebuses["TestEBus1"];
        m_behaviorContext->m_ebuses.clear();
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus2");
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash2, &(m_behaviorContext->m_ebuses));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEBuses_ReturnDifferentHash_EBusMapsHaveDifferentEBusDefinition)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod1", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod1);
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash1, &(m_behaviorContext->m_ebuses));
        delete m_behaviorContext->m_ebuses["TestEBus"];
        m_behaviorContext->m_ebuses.clear();
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod2", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod2);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash2, &(m_behaviorContext->m_ebuses));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEBuses_ReturnDifferentHash_OneEBusMapIsEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash1, &(m_behaviorContext->m_ebuses));
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus");
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineEBuses(actualHash2, &(m_behaviorContext->m_ebuses));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEvents_ReturnZero_EbusIsNull)
    {
        size_t actualHash = 0;
        BehaviorContextUtils::HashCombineEvents(actualHash, nullptr);
        EXPECT_EQ(actualHash, 0);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEvents_ReturnSameHash_SameEBus)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod1);
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineEvents(actualHash1, m_behaviorContext->m_ebuses["TestEBus"]);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineEvents(actualHash2, m_behaviorContext->m_ebuses["TestEBus"]);
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineEvents_ReturnDifferentHash_EBusHasDifferentEventDefinition)
    {
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod1", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod1);
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineEvents(actualHash1, m_behaviorContext->m_ebuses["TestEBus"]);
        delete m_behaviorContext->m_ebuses["TestEBus"];
        m_behaviorContext->m_ebuses.clear();
        m_behaviorContext
            ->EBus<BehaviorContextUtilsUnitTestStructures::TestEBus>("TestEBus")
            ->Event("TestMethod2", &BehaviorContextUtilsUnitTestStructures::TestEBus::Events::TestMethod2);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineEvents(actualHash2, m_behaviorContext->m_ebuses["TestEBus"]);
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineClasses_ReturnZero_ClassMapIsInvalid)
    {
        size_t actualHash = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash, nullptr);
        EXPECT_EQ(actualHash, 0);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineClasses_ReturnSameHash_SameClassMaps)
    {
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass")
            ->Property("TestClassProperty", []() { return true; }, [](bool) {})
            ->Method("TestClassMethod", []() {});
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash1, &(m_behaviorContext->m_classes));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash2, &(m_behaviorContext->m_classes));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineClasses_ReturnSameHash_ClassMapsAreEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash1, &(m_behaviorContext->m_classes));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash2, &(m_behaviorContext->m_classes));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineClasses_ReturnDifferentHash_ClassMapsHaveDifferentClassName)
    {
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass1")
            ->Property("TestClassProperty", []() { return true; }, [](bool) {})
            ->Method("TestClassMethod", []() {});
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash1, &(m_behaviorContext->m_classes));
        delete m_behaviorContext->m_classes["TestClass1"];
        m_behaviorContext->m_classes.clear();
        m_behaviorContext->m_typeToClassMap.clear();
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass2")
            ->Property("TestClassProperty", []() { return true; }, [](bool) {})
            ->Method("TestClassMethod", []() {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash2, &(m_behaviorContext->m_classes));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineClasses_ReturnDifferentHash_ClassMapsHaveDifferentClassDefinition)
    {
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass")
            ->Property("TestClassProperty", []() { return true; }, [](bool) {})
            ->Method("TestClassMethod", []() {});
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash1, &(m_behaviorContext->m_classes));
        delete m_behaviorContext->m_classes["TestClass"];
        m_behaviorContext->m_classes.clear();
        m_behaviorContext->m_typeToClassMap.clear();
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass")
            ->Method("TestClassMethod", []() {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash2, &(m_behaviorContext->m_classes));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineClasses_ReturnDifferentHash_OneClassMapIsEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash1, &(m_behaviorContext->m_classes));
        m_behaviorContext
            ->Class<BehaviorContextUtilsUnitTestStructures::TestClass>("TestClass")
            ->Property("TestClassProperty", []() { return true; }, [](bool) {})
            ->Method("TestClassMethod", []() {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineClasses(actualHash2, &(m_behaviorContext->m_classes));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineProperties_ReturnZero_PropertyMapIsInvalid)
    {
        size_t actualHash = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash, nullptr);
        EXPECT_EQ(actualHash, 0);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineProperties_ReturnSameHash_SamePropertyMaps)
    {
        m_behaviorContext->Property("TestProperty", []() { return true; }, [](bool) {});

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash1, &(m_behaviorContext->m_properties));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash2, &(m_behaviorContext->m_properties));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineProperties_ReturnSameHash_PropertyMapsAreEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash1, &(m_behaviorContext->m_properties));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash2, &(m_behaviorContext->m_properties));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineProperties_ReturnDifferentHash_PropertyMapsHaveDifferentPropertyName)
    {
        m_behaviorContext->Property("TestProperty1", []() { return true; }, [](bool) {});
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash1, &(m_behaviorContext->m_properties));

        delete m_behaviorContext->m_properties["TestProperty1"];
        m_behaviorContext->m_properties.clear();

        m_behaviorContext->Property("TestProperty2", []() { return true; }, [](bool) {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash2, &(m_behaviorContext->m_properties));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineProperties_ReturnDifferentHash_PropertyMapsHaveDifferentGetterSetter)
    {
        m_behaviorContext->Property("TestProperty", []() { return true; }, [](bool) {});
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash1, &(m_behaviorContext->m_properties));

        delete m_behaviorContext->m_properties["TestProperty"];
        m_behaviorContext->m_properties.clear();

        m_behaviorContext->Property("TestProperty", []() { return 1; }, [](int) {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash2, &(m_behaviorContext->m_properties));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineProperties_ReturnDifferentHash_OnePropertyMapIsEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash1, &(m_behaviorContext->m_properties));
        m_behaviorContext->Property("TestProperty", []() { return true; }, [](bool) {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineProperties(actualHash2, &(m_behaviorContext->m_properties));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethods_ReturnZero_MethodMapIsInvalid)
    {
        size_t actualHash = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash, nullptr);
        EXPECT_EQ(actualHash, 0);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethods_ReturnSameHash_SameMethodMaps)
    {
        m_behaviorContext->Method("TestMethod", []() {});

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash2, &(m_behaviorContext->m_methods));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethods_ReturnSameHash_MethodMapsAreEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash2, &(m_behaviorContext->m_methods));
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethods_ReturnDifferentHash_MethodMapsHaveDifferentMethodName)
    {
        m_behaviorContext->Method("TestMethod1", [](bool) {});
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));

        delete m_behaviorContext->m_methods["TestMethod1"];
        m_behaviorContext->m_methods.clear();

        m_behaviorContext->Method("TestMethod2", [](bool) {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethods_ReturnDifferentHash_MethodMapsHaveDifferentMethodSignature)
    {
        m_behaviorContext->Method("TestMethod", [](bool) {});
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));

        delete m_behaviorContext->m_methods["TestMethod"];
        m_behaviorContext->m_methods.clear();

        m_behaviorContext->Method("TestMethod", [](bool, bool) {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethods_ReturnDifferentHash_OneMethodMapIsEmpty)
    {
        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));
        m_behaviorContext->Method("TestMethod", [](bool) {});
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethods(actualHash1, &(m_behaviorContext->m_methods));
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethodSignature_ReturnZero_MethodIsInvalid)
    {
        size_t actualHash = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash, nullptr);
        EXPECT_EQ(actualHash, 0);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethodSignature_ReturnSameHash_SameMethod)
    {
        m_behaviorContext->Method("TestMethod", [](bool) {});

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash1, m_behaviorContext->m_methods["TestMethod"]);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash2, m_behaviorContext->m_methods["TestMethod"]);
        EXPECT_TRUE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethodSignature_ReturnDifferentHash_MethodsHaveDifferentName)
    {
        m_behaviorContext->Method("TestMethod1", [](bool) {});
        m_behaviorContext->Method("TestMethod2", [](bool) {});

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash1, m_behaviorContext->m_methods["TestMethod1"]);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash2, m_behaviorContext->m_methods["TestMethod2"]);
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethodSignature_ReturnDifferentHash_MethodsHaveDifferentArguments)
    {
        m_behaviorContext->Method("TestMethod1", [](bool) {});
        m_behaviorContext->Method("TestMethod2", [](int) {});

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash1, m_behaviorContext->m_methods["TestMethod1"]);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash2, m_behaviorContext->m_methods["TestMethod2"]);
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethodSignature_ReturnDifferentHash_MethodsHaveDifferentArgumentNumber)
    {
        m_behaviorContext->Method("TestMethod1", [](bool) {});
        m_behaviorContext->Method("TestMethod2", [](bool, bool) {});

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash1, m_behaviorContext->m_methods["TestMethod1"]);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash2, m_behaviorContext->m_methods["TestMethod2"]);
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethodSignature_ReturnDifferentHash_MethodsHaveDifferentResult)
    {
        m_behaviorContext->Method("TestMethod1", []() { return true; });
        m_behaviorContext->Method("TestMethod2", []() {});

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash1, m_behaviorContext->m_methods["TestMethod1"]);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash2, m_behaviorContext->m_methods["TestMethod2"]);
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, HashCombineMethodSignature_ReturnDifferentHash_MethodsHaveDifferentResultType)
    {
        m_behaviorContext->Method("TestMethod1", []() { return true; });
        m_behaviorContext->Method("TestMethod2", []() { return 1; });

        size_t actualHash1 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash1, m_behaviorContext->m_methods["TestMethod1"]);
        size_t actualHash2 = 0;
        BehaviorContextUtils::HashCombineMethodSignature(actualHash2, m_behaviorContext->m_methods["TestMethod2"]);
        EXPECT_FALSE(actualHash1 == actualHash2);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, IsSameDataType_ReturnFalse_ParameterIsNull)
    {
        bool result = BehaviorContextUtils::IsSameDataType(nullptr, Data::Type::Boolean());
        EXPECT_FALSE(result);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, IsSameDataType_ReturnFalse_ParameterHasDifferentDataType)
    {
        m_behaviorContext->Method("TestMethod", []() { return 1; });
        bool result = BehaviorContextUtils::IsSameDataType(m_behaviorContext->m_methods["TestMethod"]->GetResult(), Data::Type::Boolean());
        EXPECT_FALSE(result);
    }

    TEST_F(ScriptCanvasBehaviorContextUtilsUnitTestFixture, IsSameDataType_ReturnTrue_ParameterHasSameDataType)
    {
        m_behaviorContext->Method("TestMethod", []() { return 1; });
        bool result = BehaviorContextUtils::IsSameDataType(m_behaviorContext->m_methods["TestMethod"]->GetResult(), Data::Type::Number());
        EXPECT_TRUE(result);
    }
}
