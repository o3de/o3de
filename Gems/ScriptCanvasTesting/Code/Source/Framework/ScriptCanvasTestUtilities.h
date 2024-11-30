/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>
#include <Editor/Framework/ScriptCanvasReporter.h>
#include <Editor/Framework/ScriptCanvasTraceUtilities.h>
#include <Framework/ScriptCanvasTestNodes.h>

#include <ScriptCanvas/Core/Nodeable.h>
#include <AzFramework/Entity/SliceEntityOwnershipServiceBus.h>
#include "AzFramework/Slice/SliceInstantiationTicket.h"

namespace ScriptCanvasTests
{
    extern const char* k_tempCoreAssetDir;
    extern const char* k_tempCoreAssetName;
    extern const char* k_tempCoreAssetPath;

    const char* GetUnitTestDirPathRelative();

    void ExpectParse(AZStd::string_view graphPath);

    void ExpectParseError(AZStd::string_view graphPath);

    AZStd::string_view GetGraphNameFromPath(AZStd::string_view graphPath);

    void RunUnitTestGraph(AZStd::string_view graphPath);

    void RunUnitTestGraph(AZStd::string_view graphPath, ScriptCanvas::ExecutionMode execution);

    void RunUnitTestGraph(AZStd::string_view graphPath, ScriptCanvas::ExecutionMode execution, const ScriptCanvasEditor::DurationSpec& duration);

    void RunUnitTestGraph(AZStd::string_view graphPath, ScriptCanvas::ExecutionMode execution, AZStd::string_view dependentScriptEvent);

    void RunUnitTestGraph(AZStd::string_view graphPath, const ScriptCanvasEditor::DurationSpec& duration);

    void RunUnitTestGraph(AZStd::string_view graphPath, const ScriptCanvasEditor::RunSpec& runSpec);

    void RunUnitTestGraphMixed(AZStd::string_view graphPath, const ScriptCanvasEditor::DurationSpec& duration);

    void RunUnitTestGraphMixed(AZStd::string_view graphPath);

    void VerifyReporter(const ScriptCanvasEditor::Reporter& reporter);

    template<typename t_NodeType>
    t_NodeType* GetTestNode([[maybe_unused]] const ScriptCanvas::ScriptCanvasId& scriptCanvasId, const AZ::EntityId& nodeID)
    {
        using namespace ScriptCanvas;
        t_NodeType* node{};
        ScriptCanvas::SystemRequestBus::BroadcastResult(node, &ScriptCanvas::SystemRequests::GetNode<t_NodeType>, nodeID);
        EXPECT_TRUE(node != nullptr);
        return node;
    }

    template<typename t_NodeType>
    t_NodeType* CreateTestNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, AZ::EntityId& entityOut)
    {
        using namespace ScriptCanvas;

        AZ::Entity* entity{ aznew AZ::Entity };
        entity->Init();
        entityOut = entity->GetId();
        EXPECT_TRUE(entityOut.IsValid());
        ScriptCanvas::SystemRequestBus::Broadcast(&ScriptCanvas::SystemRequests::CreateNodeOnEntity, entityOut, scriptCanvasId, azrtti_typeid<t_NodeType>());
        return GetTestNode<t_NodeType>(scriptCanvasId, entityOut);
    }

    template<typename t_Value>
    ScriptCanvas::VariableId CreateVariable(ScriptCanvas::ScriptCanvasId scriptCanvasId, const t_Value& value, AZStd::string_view variableName)
    {
        using namespace ScriptCanvas;
        AZ::Outcome<VariableId, AZStd::string> addVariableOutcome = AZ::Failure(AZStd::string());
        GraphVariableManagerRequestBus::EventResult(addVariableOutcome, scriptCanvasId, &GraphVariableManagerRequests::AddVariable, variableName, Datum(value), false);
        if (!addVariableOutcome)
        {
            AZ_Warning("Script Canvas Test", false, "%s", addVariableOutcome.GetError().data());
            return {};
        }

        return addVariableOutcome.TakeValue();
    }

    AZ::EntityId CreateClassFunctionNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId, AZStd::string_view className, AZStd::string_view methodName);
    AZStd::string SlotDescriptorToString(ScriptCanvas::SlotDescriptor type);
    void DumpSlots(const ScriptCanvas::Node& node);
    bool Connect(ScriptCanvas::Graph& graph, const AZ::EntityId& fromNodeID, const char* fromSlotName, const AZ::EntityId& toNodeID, const char* toSlotName, bool dumpSlotsOnFailure = true);

    class UnitTestEvents
        : public AZ::EBusTraits
    {
    public:
        virtual void Failed(AZStd::string description) = 0;
        virtual AZStd::string Result(AZStd::string description) = 0;
        virtual void SideEffect(AZStd::string description) = 0;
        virtual void Succeeded(AZStd::string description) = 0;
    };

    using UnitTestEventsBus = AZ::EBus<UnitTestEvents>;

    class UnitTestEventsHandler
        : public UnitTestEventsBus::Handler
    {
    public:
        ~UnitTestEventsHandler()
        {
            Clear();
        }

        void Clear()
        {
            for (auto iter : m_failures)
            {
                ADD_FAILURE() << iter.first.c_str();
            }

            for (auto iter : m_successes)
            {
                GTEST_SUCCEED();
            }

            m_failures.clear();
            m_results.clear();
            m_successes.clear();
            m_results.clear();
        }

        void Failed(AZStd::string description) override
        {
            auto iter = m_failures.find(description);
            if (iter != m_failures.end())
            {
                ++iter->second;
            }
            else
            {
                m_failures.insert(AZStd::make_pair(description, 1));
            }
        }

        int FailureCount(AZStd::string_view description) const
        {
            auto iter = m_failures.find(description);
            return iter != m_failures.end() ? iter->second : 0;
        }

        bool IsFailed() const
        {
            return !m_failures.empty();
        }

        bool IsSucceeded() const
        {
            return !m_successes.empty();
        }

        void SideEffect(AZStd::string description) override
        {
            auto iter = m_sideEffects.find(description);
            if (iter != m_sideEffects.end())
            {
                ++iter->second;
            }
            else
            {
                m_sideEffects.insert(AZStd::make_pair(description, 1));
            }
        }


        int SideEffectCount() const
        {
            int count(0);

            for (auto iter : m_sideEffects)
            {
                count += iter.second;
            }

            return count;
        }

        int SideEffectCount(AZStd::string_view description) const
        {
            auto iter = m_sideEffects.find(description);
            return iter != m_sideEffects.end() ? iter->second : 0;
        }

        void Succeeded(AZStd::string description) override
        {
            auto iter = m_successes.find(description);

            if (iter != m_successes.end())
            {
                ++iter->second;
            }
            else
            {
                m_successes.insert(AZStd::make_pair(description, 1));
            }
        }

        int SuccessCount(AZStd::string_view description) const
        {
            auto iter = m_successes.find(description);
            return iter != m_successes.end() ? iter->second : 0;
        }

        AZStd::string Result(AZStd::string description) override
        {
            auto iter = m_results.find(description);

            if (iter != m_results.end())
            {
                ++iter->second;
            }
            else
            {
                m_results.insert(AZStd::make_pair(description, 1));
            }

            return description;
        }

        int ResultCount(AZStd::string_view description) const
        {
            auto iter = m_results.find(description);
            return iter != m_results.end() ? iter->second : 0;
        }

    private:
        AZStd::unordered_map<AZStd::string, int> m_failures;
        AZStd::unordered_map<AZStd::string, int> m_sideEffects;
        AZStd::unordered_map<AZStd::string, int> m_successes;
        AZStd::unordered_map<AZStd::string, int> m_results;
    };

    struct TestBehaviorContextProperties
    {
        AZ_TYPE_INFO(TestBehaviorContextProperties, "{13ECCA93-9A62-4A46-846C-ABFEEA215C48}");

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<TestBehaviorContextProperties>()
                    ->Version(0)
                    ;
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<TestBehaviorContextProperties>("TestBehaviorContextProperties")
                    ->Property("boolean", BehaviorValueGetter(&TestBehaviorContextProperties::m_booleanProp), BehaviorValueSetter(&TestBehaviorContextProperties::m_booleanProp))
                    ->Property("number", BehaviorValueGetter(&TestBehaviorContextProperties::m_numberProp), BehaviorValueSetter(&TestBehaviorContextProperties::m_numberProp))
                    ->Property("string", BehaviorValueGetter(&TestBehaviorContextProperties::m_stringProp), BehaviorValueSetter(&TestBehaviorContextProperties::m_stringProp))
                    ->Property("getterOnlyNumber", BehaviorValueGetter(&TestBehaviorContextProperties::m_getterOnlyProp), nullptr)
                    ->Property("setterOnlyNumber", nullptr, BehaviorValueSetter(&TestBehaviorContextProperties::m_setterOnlyProp))
                    ;
            }
        }

        ScriptCanvas::Data::BooleanType m_booleanProp = false;
        ScriptCanvas::Data::NumberType m_numberProp = 0.0f;
        ScriptCanvas::Data::StringType m_stringProp;
        ScriptCanvas::Data::NumberType m_getterOnlyProp = 0.0f;
        ScriptCanvas::Data::NumberType m_setterOnlyProp = 0.0f;
    };

    class TestBehaviorContextObject
    {
    public:
        AZ_TYPE_INFO(TestBehaviorContextObject, "{52DA0875-8EF6-4D39-9CF6-9F83BAA59A1D}");

        static TestBehaviorContextObject MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs);

        static const TestBehaviorContextObject* MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs);

        static const TestBehaviorContextObject& MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs);

        static int MaxReturnByValueInteger(int lhs, int rhs);

        static const int* MaxReturnByPointerInteger(const int* lhs, const int* rhs);

        static const int& MaxReturnByReferenceInteger(const int& lhs, const int& rhs);

        static void Reflect(AZ::ReflectContext* reflectContext);

        static AZ::u32 GetCreatedCount() { return s_createdCount; }
        static AZ::u32 GetDestroyedCount() { return s_destroyedCount; }
        static void ResetCounts() { s_createdCount = s_destroyedCount = 0; }

        TestBehaviorContextObject()
        {
            ++s_createdCount;
        }

        TestBehaviorContextObject(const TestBehaviorContextObject& src)
            : m_value(src.m_value)
        {
            ++s_createdCount;
        }

        TestBehaviorContextObject(TestBehaviorContextObject&& src)
            : m_value(src.m_value)
        {
            ++s_createdCount;
        }

        TestBehaviorContextObject(int value)
            : m_value(value)
        {
            ++s_createdCount;
        }

        ~TestBehaviorContextObject()
        {
            [[maybe_unused]] AZ::u32 destroyedCount = ++s_destroyedCount;
            AZ_Assert(destroyedCount <= s_createdCount, "ScriptCanvas execution error");
        }

        TestBehaviorContextObject& operator=(const TestBehaviorContextObject& source)
        {
            m_value = source.m_value;
            return *this;
        }

        // make a to string

        int GetValue() const { return m_value; }
        void SetValue(int value) { m_value = value; }

        bool IsNormalized() const { return m_isNormalized; }
        void Normalize() { m_isNormalized = true; }
        void Denormalize() { m_isNormalized = false; }

        bool operator>(const TestBehaviorContextObject& other) const
        {
            return m_value > other.m_value;
        }

        bool operator>=(const TestBehaviorContextObject& other) const
        {
            return m_value >= other.m_value;
        }

        bool operator==(const TestBehaviorContextObject& other) const
        {
            return m_value == other.m_value;
        }

        bool operator!=(const TestBehaviorContextObject& other) const
        {
            return m_value != other.m_value;
        }

        bool operator<=(const TestBehaviorContextObject& other) const
        {
            return m_value <= other.m_value;
        }

        bool operator<(const TestBehaviorContextObject& other) const
        {
            return m_value < other.m_value;
        }

    private:
        int m_value = -1;
        bool m_isNormalized = false;
        static AZ::u32 s_createdCount;
        static AZ::u32 s_destroyedCount;
    };

    TestBehaviorContextObject MaxReturnByValue(TestBehaviorContextObject lhs, TestBehaviorContextObject rhs);
    const TestBehaviorContextObject* MaxReturnByPointer(const TestBehaviorContextObject* lhs, const TestBehaviorContextObject* rhs);
    const TestBehaviorContextObject& MaxReturnByReference(const TestBehaviorContextObject& lhs, const TestBehaviorContextObject& rhs);

    class EventTest
        : public AZ::ComponentBus
    {
    public:
        virtual void Event() = 0;
        virtual int Number(int number) = 0;
        virtual AZStd::tuple<int, int> Numbers(int number) = 0;
        virtual AZStd::string String(AZStd::string_view string) = 0;
        virtual TestBehaviorContextObject Object(const TestBehaviorContextObject& vector) = 0;
    };

    using EventTestBus = AZ::EBus<EventTest>;

    class EventTestHandler
        : public EventTestBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER
        (EventTestHandler
            , "{29E7F7EE-0867-467A-8389-68B07C184109}"
            , AZ::SystemAllocator
            , Event
            , Number
            , Numbers
            , String
            , Object);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EventTestHandler>()
                    ->Version(0)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<EventTestBus>("EventTestHandler")
                    ->Handler<EventTestHandler>()
                    ;
            }
        }

        void Event() override
        {
            Call(FN_Event);
        }

        int Number(int input) override
        {
            int output;
            CallResult(output, FN_Number, input);
            return output;
        }

        AZStd::tuple<int, int> Numbers(int number) override
        {
            AZStd::tuple<int, int> result;
            CallResult(result, FN_Numbers, number);
            return result;
        }

        AZStd::string String(AZStd::string_view input) override
        {
            AZStd::string output;
            CallResult(output, FN_String, input);
            return output;
        }

        TestBehaviorContextObject Object(const TestBehaviorContextObject& input) override
        {
            TestBehaviorContextObject output;
            CallResult(output, FN_Object, input);
            return output;
        }
    }; // EventTestHandler

    class ConvertibleToString
    {
    public:
        AZ_TYPE_INFO(ConvertibleToString, "{DBF947E7-4097-4C5D-AF0D-E2DB311E8958}");

        static AZStd::string ConstCharPtrToString(const char* inputStr)
        {
            return AZStd::string_view(inputStr);
        }

        static AZStd::string StringViewToString(AZStd::string_view inputView)
        {
            return inputView;
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ConvertibleToString>()
                    ->Version(0)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<ConvertibleToString>("ConvertibleToString")
                    ->Method("ConstCharPtrToString", &ConvertibleToString::ConstCharPtrToString)
                    ->Method("StringViewToString", &ConvertibleToString::StringViewToString)
                    ;
            }
        }
    };

    class UnitTestEntityContext
        : public AzFramework::EntityIdContextQueryBus::MultiHandler
        , public AzFramework::EntityContextRequestBus::Handler
        , public AzFramework::SliceEntityOwnershipServiceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(UnitTestEntityContext, AZ::SystemAllocator);

        UnitTestEntityContext() { AzFramework::EntityContextRequestBus::Handler::BusConnect(GetOwningContextId()); }
        //// EntityIdContextQueryBus::MultiHandler
        // Returns a generate Uuid which represents the UnitTest EntityContext
        AzFramework::EntityContextId GetOwningContextId() override { return AzFramework::EntityContextId("{36591268-5CE9-4BC1-8277-0AB9AD528447}"); }
        ////

        //// AzFramework::SliceEntityOwnershipServiceRequests
        const AzFramework::RootSliceAsset& GetRootAsset() const override {
            static AzFramework::RootSliceAsset invalid;
            return invalid;
        }
        AZ::SliceComponent* GetRootSlice() override { return {}; }
        void SetIsDynamic(bool) override {}
        void CancelSliceInstantiation(const AzFramework::SliceInstantiationTicket&) override {}
        AzFramework::SliceInstantiationTicket GenerateSliceInstantiationTicket() override { return AzFramework::SliceInstantiationTicket(); }
        bool HandleRootEntityReloadedFromStream(AZ::Entity*, bool, [[maybe_unused]] AZ::SliceComponent::EntityIdToEntityIdMap* idRemapTable = nullptr) override
        {
            return false;
        }
        const AZ::SliceComponent::EntityIdToEntityIdMap& GetLoadedEntityIdMap() override { return m_unitTestEntityIdMap; }
        AZ::SliceComponent::SliceInstanceAddress CloneSliceInstance(AZ::SliceComponent::SliceInstanceAddress,
            AZ::SliceComponent::EntityIdToEntityIdMap&) override
        {
            return{};
        }
        AzFramework::SliceInstantiationTicket InstantiateSlice(const AZ::Data::Asset<AZ::Data::AssetData>&,
            [[maybe_unused]] const AZ::IdUtils::Remapper<AZ::EntityId>::IdMapper& customIdMapper = nullptr,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilter = nullptr) override
        {
            return AzFramework::SliceInstantiationTicket();
        }


        AZ::Data::AssetId CurrentlyInstantiatingSlice() override {
            return AZ::Data::AssetId();
        };

        AZ::Entity* CreateEntity(const char* name) override;

        void AddEntity(AZ::Entity* entity) override;

        void ActivateEntity(AZ::EntityId entityId) override;
        void DeactivateEntity(AZ::EntityId entityId) override;
        bool DestroyEntity(AZ::Entity* entity) override;
        bool DestroyEntityById(AZ::EntityId entityId) override;
        AZ::Entity* CloneEntity(const AZ::Entity& sourceEntity) override;
        void ResetContext() override;
        AZ::EntityId FindLoadedEntityIdMapping(const AZ::EntityId& staticId) const override;
        ////

        void AddEntity(AZ::EntityId entityId);
        void RemoveEntity(AZ::EntityId entityId);
        bool IsOwnedByThisContext(AZ::EntityId entityId) { return m_unitTestEntityIdMap.count(entityId) != 0; }

        AZ::SliceComponent::EntityIdToEntityIdMap m_unitTestEntityIdMap;
    };

    template<typename TBusIdType>
    class TemplateEventTest
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = TBusIdType;

        virtual void GenericEvent() = 0;
        virtual AZStd::string UpdateNameEvent(AZStd::string_view) = 0;
        virtual AZ::Vector3 VectorCreatedEvent(const AZ::Vector3&) = 0;
    };

    template<typename TBusIdType>
    using TemplateEventTestBus = AZ::EBus<TemplateEventTest<TBusIdType>>;

    template<typename TBusIdType>
    class TemplateEventTestHandler
        : public TemplateEventTestBus<TBusIdType>::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER_TEMPLATE(TemplateEventTestHandler, (TemplateEventTestHandler, "{AADA0906-C216-4D98-BD51-76BB0C2F7FAF}", TBusIdType), AZ::SystemAllocator
            , GenericEvent
            , UpdateNameEvent
            , VectorCreatedEvent
        );

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                const AZStd::string ebusName(AZ::AzTypeInfo<TemplateEventTestHandler>::Name());
                behaviorContext->EBus<TemplateEventTestBus<TBusIdType>>(ebusName.data())
                    ->template Handler<TemplateEventTestHandler>()
                    ->Event("Update Name", &TemplateEventTestBus<TBusIdType>::Events::UpdateNameEvent)
                    ;
            }
        }

        void GenericEvent() override
        {
            Call(FN_GenericEvent);
        }

        AZStd::string UpdateNameEvent(AZStd::string_view name) override
        {
            AZStd::string result;
            CallResult(result, FN_UpdateNameEvent, name);
            return result;
        }

        AZ::Vector3 VectorCreatedEvent(const AZ::Vector3& newVector) override
        {
            AZ::Vector3 result;
            CallResult(result, FN_VectorCreatedEvent, newVector);
            return result;
        }
    };


    class ScriptUnitTestEventHandler
        : public UnitTestEventsBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER
        (ScriptUnitTestEventHandler
            , "{3FC89358-837D-43D5-8C4F-61B672A751B2}"
            , AZ::SystemAllocator
            , Failed
            , Result
            , SideEffect
            , Succeeded);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptUnitTestEventHandler>()
                    ->Version(0)
                    ;
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<UnitTestEventsBus>("UnitTestEventsBus")
                    ->Handler<ScriptUnitTestEventHandler>()
                    ->Event("Failed", &UnitTestEventsBus::Events::Failed)
                    ->Event("Result", &UnitTestEventsBus::Events::Result)
                    ->Event("SideEffect", &UnitTestEventsBus::Events::SideEffect)
                    ->Event("Succeeded", &UnitTestEventsBus::Events::Succeeded)
                    ;
            }
        }

        void Failed(AZStd::string description) override
        {
            Call(FN_Failed, description);
        }

        AZStd::string Result(AZStd::string description) override
        {
            AZStd::string output;
            CallResult(output, FN_Result, description);
            return output;
        }

        void SideEffect(AZStd::string description) override
        {
            Call(FN_SideEffect, description);
        }

        void Succeeded(AZStd::string description) override
        {
            Call(FN_Succeeded, description);
        }
    }; // ScriptUnitTestEventHandler

    class ScriptUnitTestNodeNotificationHandler
        : private ScriptCanvas::NodeNotificationsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptUnitTestNodeNotificationHandler, AZ::SystemAllocator);

        ScriptUnitTestNodeNotificationHandler(AZ::EntityId nodeId)
        {
            ScriptCanvas::NodeNotificationsBus::Handler::BusConnect(nodeId);
        }

        ~ScriptUnitTestNodeNotificationHandler()
        {
            ScriptCanvas::NodeNotificationsBus::Handler::BusDisconnect();
        }

        AZ::u32 GetSlotsAdded() const
        {
            return m_slotsAdded;
        }

        AZ::u32 GetSlotsRemoved() const
        {
            return m_slotsRemoved;
        }

    private:
        void OnSlotAdded([[maybe_unused]] const ScriptCanvas::SlotId& slotId) override
        {
            ++m_slotsAdded;
        }

        void OnSlotRemoved([[maybe_unused]] const ScriptCanvas::SlotId& slotId) override
        {
            ++m_slotsRemoved;
        }

        AZ::u32 m_slotsAdded = 0;
        AZ::u32 m_slotsRemoved = 0;
    };
    
    class TestNodeableObject
        : public ScriptCanvas::Nodeable
    {
    public:

        AZ_RTTI(TestNodeableObject, "{5FA6967F-AB4D-4077-91C9-1C2CE36733AF}", Nodeable);
        AZ_CLASS_ALLOCATOR(Nodeable, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<TestNodeableObject, Nodeable>();

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<TestNodeableObject>("TestNodeableObject", "")
                        ;
                }
            }

            // reflect API for the node
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<TestNodeableObject>("TestNodeableObject")
                    ->Method("Branch", &TestNodeableObject::Branch)
                    ;
            }
        }

        void Branch(bool condition)
        {
            if (condition)
            {
                ExecutionOut(AZ_CRC_CE("BranchTrue"), true, AZStd::string("called the true version!"));
            }
            else
            {
                ExecutionOut(AZ_CRC_CE("BranchFalse"), false, AZStd::string("called the false version!"), AZ::Vector3(1, 2, 3));
            }
        }

        bool IsActive() const override
        {
            return false;
        }
    };

    // Test class that's used to test inheritance with Script Canvas slots
    class TestBaseClass
    {
    public:
        AZ_RTTI(TestBaseClass, "{C1F80F17-BE3C-49B5-862D-3C41F04208E0}");

        TestBaseClass() = default;
        virtual ~TestBaseClass() = default;

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<TestBaseClass>();

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<TestBaseClass>("TestBaseClass", "");
                }
            }

            // reflect API for the node
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<TestBaseClass>("TestBaseClass");
            }
        }
    };

    // Test class that's used to test inheritance with Script Canvas slots
    class TestSubClass : public TestBaseClass
    {
    public:
        AZ_RTTI(TestSubClass, "{2ED5B680-F097-4044-B1C8-0977A0E3F027}", TestBaseClass);

        TestSubClass() = default;
        ~TestSubClass() override = default;

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<TestSubClass, TestBaseClass>();

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<TestSubClass>("TestSubClass", "");
                }
            }

            // reflect API for the node
            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                behaviorContext->Class<TestSubClass>("TestSubClass");
            }
        }
    };

} // namespace ScriptCanvasTests
