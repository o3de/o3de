/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/PlatformDef.h>

#include <AzCore/JSON/pointer.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <Tests/SerializeContextFixture.h>
#include <Tests/Serialization/Json/JsonSerializationTests.h>
#include <Tests/Serialization/Json/JsonSerializerMock.h>
#include <Tests/Serialization/Json/TestCases.h>
    
namespace JsonSerializationTests
{
    template<typename T>
    class TypedJsonSerializationTests 
        : public JsonSerializationTests
    {
    public:
        using SerializableStruct = T;

        ~TypedJsonSerializationTests() override = default;

        void Reflect(bool fullReflection)
        {
            SerializableStruct::Reflect(m_serializeContext, fullReflection);
            m_fullyReflected = fullReflection;
        }

        bool m_fullyReflected = true;
    };

    TYPED_TEST_CASE(TypedJsonSerializationTests, JsonSerializationTestCases);

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializedDefaultInstance_EmptyJsonReturned)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings->m_keepDefaults = false;

        TypeParam instance;
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(), instance, *this->m_serializationSettings);
        EXPECT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_DocStrEq("{}");
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_EmptyJson_SucceedsAndObjectMatchesDefaults)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_jsonDocument->Parse("{}");

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, *this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, loadResult.GetOutcome());
        
        TypeParam expectedInstance;
        EXPECT_TRUE(loadInstance.Equals(expectedInstance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithoutDefaults_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings->m_keepDefaults = false;

        auto description = TypeParam::GetInstanceWithoutDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, *this->m_serializationSettings);
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        this->Expect_DocStrEq(description.m_json);
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonWithoutDefaults_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithoutDefaults();
        this->m_jsonDocument->Parse(description.m_json);

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, *this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithSomeDefaults_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings->m_keepDefaults = false;

        auto description = TypeParam::GetInstanceWithSomeDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, *this->m_serializationSettings);
        
        bool partialDefaultsSupported = TypeParam::SupportsPartialDefaults;
        EXPECT_EQ(partialDefaultsSupported ? Outcomes::PartialDefaults : Outcomes::DefaultsUsed, result.GetOutcome());
        this->Expect_DocStrEq(description.m_jsonWithStrippedDefaults);
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonWithSomeDefaults_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithSomeDefaults();
        this->m_jsonDocument->Parse(description.m_jsonWithStrippedDefaults);

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, *this->m_deserializationSettings);
        bool validResult =
            loadResult.GetOutcome() == Outcomes::Success ||
            loadResult.GetOutcome() == Outcomes::DefaultsUsed ||
            loadResult.GetOutcome() == Outcomes::PartialDefaults;
        EXPECT_TRUE(validResult);
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithSomeDefaultsKept_StoredSuccessfullyAndJsonMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings->m_keepDefaults = true;

        auto description = TypeParam::GetInstanceWithSomeDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, *this->m_serializationSettings);
        
        EXPECT_EQ(Outcomes::Success, result.GetOutcome());
        this->Expect_DocStrEq(description.m_jsonWithKeptDefaults);
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonWithSomeDefaultsKept_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithSomeDefaults();
        this->m_jsonDocument->Parse(description.m_jsonWithKeptDefaults);

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, *this->m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Store_SerializeWithUnknownType_UnknownTypeReturned)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(false);
        this->m_serializationSettings->m_keepDefaults = true;

        auto description = TypeParam::GetInstanceWithoutDefaults();
        ResultCode result = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, *this->m_serializationSettings);
        EXPECT_EQ(Outcomes::Unknown, result.GetOutcome());
    }

    TYPED_TEST(TypedJsonSerializationTests, LoadAndStore_RoundTrip_StoredObjectCanBeLoadedAgain)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        this->m_serializationSettings->m_keepDefaults = true;
        auto description = TypeParam::GetInstanceWithoutDefaults();

        ResultCode storeResult = AZ::JsonSerialization::Store(*this->m_jsonDocument, this->m_jsonDocument->GetAllocator(),
            *description.m_instance, *this->m_serializationSettings);
        ASSERT_NE(Processing::Halted, storeResult.GetProcessing());

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, *this->m_deserializationSettings);
        ASSERT_NE(Processing::Halted, loadResult.GetProcessing());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    TYPED_TEST(TypedJsonSerializationTests, Load_JsonAdditionalFields_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        this->Reflect(true);
        auto description = TypeParam::GetInstanceWithoutDefaults();
        this->m_jsonDocument->Parse(description.m_json);
        this->InjectAdditionalFields(*this->m_jsonDocument, rapidjson::kStringType, this->m_jsonDocument->GetAllocator());

        TypeParam loadInstance;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadInstance, *this->m_jsonDocument, *this->m_deserializationSettings);
        ASSERT_NE(Processing::Halted, loadResult.GetProcessing());
        EXPECT_TRUE(loadInstance.Equals(*description.m_instance, this->m_fullyReflected));
    }

    // Load

    TEST_F(JsonSerializationTests, Load_PrimitiveAtTheRoot_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        m_jsonDocument->Parse("true");

        bool loadValue = false;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadValue, *m_jsonDocument, *m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_TRUE(loadValue);
    }

    TEST_F(JsonSerializationTests, Load_ArrayAtTheRoot_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        // Since there's no class that will register the container as part of the reflection process
        // explicitly register the container by forcing the creation of generic info specifically
        // for the containers type and template argument.
        auto genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<int>>::GetGenericInfo();
        ASSERT_NE(nullptr, genericInfo);
        genericInfo->Reflect(m_serializeContext.get());

        m_jsonDocument->Parse("[13,42,88]");

        AZStd::vector<int> loadValues;
        ResultCode loadResult = AZ::JsonSerialization::Load(loadValues, *m_jsonDocument, *m_deserializationSettings);
        ASSERT_EQ(Outcomes::Success, loadResult.GetOutcome());
        EXPECT_EQ(loadValues, AZStd::vector<int>({ 13, 42, 88 }));
    }

    TEST_F(JsonSerializationTests, Load_PointerToUnregisteredClass_ReturnsUnknown)
    {
        using namespace AZ::JsonSerializationResult;

        auto scopedReflector = [](AZ::SerializeContext* serializeContext)
        {
            serializeContext->Class<ClassWithPointerToUnregisteredClass>()
                ->Field("ptr", &ClassWithPointerToUnregisteredClass::m_ptr);
        };
        UnitTest::ScopedSerializeContextReflector scopedJsonReflector(*m_serializeContext, { AZStd::move(scopedReflector) });
        m_jsonDocument->Parse(R"(
            {
                "ptr":
                {
                    "$type": "{9E69A37B-22BD-4E3F-9A80-63D902966591}"
                }
            })");
        ASSERT_FALSE(m_jsonDocument->HasParseError());

        ClassWithPointerToUnregisteredClass value;
        ResultCode loadResult = AZ::JsonSerialization::Load(value, *m_jsonDocument, *m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unknown, loadResult.GetOutcome());
    }

    // Tests if a "$type" that's not needed doesn't have side effects because it doesn't give a polymorphic instance.
    TEST_F(JsonSerializationTests, Load_PointerToSameClass_SucceedsAndObjectMatches)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "BaseClass"
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, *m_deserializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, loadResult.GetOutcome());

        ASSERT_NE(nullptr, instance.m_pointer);
        EXPECT_EQ(azrtti_typeid(instance.m_pointer), azrtti_typeid<BaseClass>());
    }

    TEST_F(JsonSerializationTests, Load_InvalidPointerName_FailsToConvert)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "Invalid"
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, *m_deserializationSettings);
        EXPECT_EQ(Outcomes::Unknown, loadResult.GetOutcome());
        EXPECT_EQ(Processing::Halted, loadResult.GetProcessing());
    }

    TEST_F(JsonSerializationTests, Load_LoadToNullPtr_ReturnsCatastrophic)
    {
        using namespace AZ::JsonSerializationResult;

        ResultCode loadResult = AZ::JsonSerialization::Load(nullptr, azrtti_typeid<int>(), *m_jsonDocument, *m_deserializationSettings);
        EXPECT_EQ(Outcomes::Catastrophic, loadResult.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Load_UnrelatedPointerType_FailsToCast)
    {
        using namespace AZ::JsonSerializationResult;

        ComplexNullInheritedPointer::Reflect(m_serializeContext, true);
        SimpleClass::Reflect(m_serializeContext, true);

        m_jsonDocument->Parse(
            R"({
                    "pointer": 
                    {
                        "$type": "SimpleClass"
                    }
                })");

        ComplexNullInheritedPointer instance;
        ResultCode loadResult = AZ::JsonSerialization::Load(instance, *m_jsonDocument, *m_deserializationSettings);
        EXPECT_EQ(Outcomes::TypeMismatch, loadResult.GetOutcome());
        EXPECT_EQ(Processing::Halted, loadResult.GetProcessing());
    }

    // Store

    TEST_F(JsonSerializationTests, Store_PrimitiveAtTheRoot_ReturnsSuccessAndTheValueAtTheRoot)
    {
        using namespace AZ::JsonSerializationResult;

        m_serializationSettings->m_keepDefaults = true;

        bool value = true;
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), value, *m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        Expect_DocStrEq("true");
    }

    TEST_F(JsonSerializationTests, Store_ArrayAtTheRoot_ReturnsSuccessAndTheArrayAtTheRoot)
    {
        using namespace AZ::JsonSerializationResult;

        // Since there's no class that will register the container as part of the reflection process
        // explicitly register the container by forcing the creation of generic info specifically
        // for the containers type and template argument.
        auto genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<int>>::GetGenericInfo();
        ASSERT_NE(nullptr, genericInfo);
        genericInfo->Reflect(m_serializeContext.get());

        m_serializationSettings->m_keepDefaults = true;

        AZStd::vector<int> values = { 13, 42, 88 };
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), values, *m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        Expect_DocStrEq("[13,42,88]");
    }

    TEST_F(JsonSerializationTests, Store_ClassWithoutMembers_ReturnsDefaultsUsed)
    {
        using namespace AZ::JsonSerializationResult;

        m_serializeContext->Class<EmptyClass>()->SerializeWithNoData();

        EmptyClass instance;
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instance, *m_serializationSettings);
        ASSERT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Store_ClassWithoutMembersAndDefaultsKept_ReturnsSuccess)
    {
        using namespace AZ::JsonSerializationResult;

        m_serializeContext->Class<EmptyClass>()->SerializeWithNoData();

        m_serializationSettings->m_keepDefaults = true;

        EmptyClass instance;
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instance, *m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
    }

    TEST_F(JsonSerializationTests, Store_PointerToClassWithSameNames_SuccessWithFullyQualifiedName)
    {
        using namespace AZ::JsonSerializationResult;

        BaseClass::Reflect(m_serializeContext);
        m_serializeContext->Class<A::Inherited, BaseClass>();
        m_serializeContext->Class<B::Inherited, BaseClass>();
        m_serializeContext->Class<ConflictingNameTestClass>()
            ->Field("pointer", &ConflictingNameTestClass::m_pointer);

        m_serializationSettings->m_keepDefaults = true;

        ConflictingNameTestClass instance;
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instance, *m_serializationSettings);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());

        rapidjson::Value* pointerType = rapidjson::Pointer("/pointer/$type").Get(*m_jsonDocument);
        ASSERT_NE(nullptr, pointerType);
        const char* type = pointerType->GetString();
        EXPECT_STRCASEEQ("{E7829F37-C577-4F2B-A85B-6F331548354C} Inherited", type);
    }

    TEST_F(JsonSerializationTests, Load_TemplatedClassWithRegisteredHandler_LoadOnHandlerCalled)
    {
        using namespace AZ::JsonSerializationResult;
        using namespace ::testing;

        m_jsonDocument->Parse(
            R"({
                    "var1": 188, 
                    "var2": 288
                })");

        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        JsonSerializerMock* mock = reinterpret_cast<JsonSerializerMock*>(
            m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<TemplatedClass>()));
        EXPECT_CALL(*mock, Load(_, _, _, _))
            .Times(Exactly(1))
            .WillRepeatedly(Return(Result(m_deserializationSettings->m_reporting, "Test", Tasks::ReadField, Outcomes::Success, "")));

        TemplatedClass<int> instance;
        AZ::JsonSerialization::Load(instance, *m_jsonDocument, *m_deserializationSettings);

        m_serializeContext->EnableRemoveReflection();
        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_serializeContext->DisableRemoveReflection();

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        m_jsonRegistrationContext->DisableRemoveReflection();

    }

    TEST_F(JsonSerializationTests, Store_TemplatedClassWithRegisteredHandler_StoreOnHandlerCalled)
    {
        using namespace AZ::JsonSerializationResult;
        using namespace ::testing;

        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        JsonSerializerMock* mock = reinterpret_cast<JsonSerializerMock*>(
            m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<TemplatedClass>()));
        EXPECT_CALL(*mock, Store(_, _, _, _, _))
            .Times(Exactly(1))
            .WillRepeatedly(Return(Result(m_serializationSettings->m_reporting, "Test", Tasks::WriteValue, Outcomes::Success, "")));

        TemplatedClass<int> instance;
        AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instance, *m_serializationSettings);

        m_serializeContext->EnableRemoveReflection();
        TemplatedClass<int>::Reflect(m_serializeContext, true);
        m_serializeContext->DisableRemoveReflection();

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<TemplatedClass>();
        m_jsonRegistrationContext->DisableRemoveReflection();
    }

    TEST_F(JsonSerializationTests, Store_StoreWithNullPtr_ReturnsCatastrophic)
    {
        using namespace AZ::JsonSerializationResult;

        TemplatedClass<int> instance;
        ResultCode result = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(),
            nullptr, nullptr, azrtti_typeid<int>(), *m_serializationSettings);

        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());
    }
} // namespace JsonSerializationTests
