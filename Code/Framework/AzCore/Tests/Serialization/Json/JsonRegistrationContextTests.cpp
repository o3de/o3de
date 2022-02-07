/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/JsonSerializerMock.h>

namespace JsonSerializationTests
{
    // Helper class which mocks virtual methods we do not handle,
    // and takes care of the common unreflect behavior for the json registration context.
    template <typename SerializerClass>
    class JsonSerializerTemplatedMock
        : public JsonSerializerMock
    {
    public:
        AZ_CLASS_ALLOCATOR(JsonSerializerTemplatedMock, AZ::SystemAllocator, 0);
        
        ~JsonSerializerTemplatedMock() override = default;

        static void Unreflect(AZ::JsonRegistrationContext* context)
        {
            context->EnableRemoveReflection();
            SerializerClass::Reflect(context);
            context->DisableRemoveReflection();
        }
    };

    class SerializerWithNoType
        : public JsonSerializerTemplatedMock<SerializerWithNoType>
    {
    public:
        AZ_RTTI(SerializerWithNoType, "{81D33B75-FD74-4FCD-AA6A-E1832EFCC530}", BaseJsonSerializer);
        ~SerializerWithNoType() override = default;

        static void Reflect(AZ::JsonRegistrationContext* context)
        {
            context->Serializer<SerializerWithNoType>();
        }
    };

    // Registers a single uuid
    class SerializerWithOneType
        : public JsonSerializerTemplatedMock<SerializerWithOneType>
    {
    public:
        AZ_RTTI(SerializerWithOneType, "{916C781A-DAF2-45F1-8386-2B47D0B7F8B5}", BaseJsonSerializer);
        ~SerializerWithOneType() override = default;

        static void Reflect(AZ::JsonRegistrationContext* context)
        {
            context->Serializer<SerializerWithOneType>()
                ->HandlesType<bool>();
        }
    };

    // Registers two different uuids
    class SerializerWithTwoDifferentTypes
        : public JsonSerializerTemplatedMock<SerializerWithTwoDifferentTypes>
    {
    public:
        AZ_RTTI(SerializerWithTwoDifferentTypes, "{3A15D3FC-2029-49DB-A1A3-04AAB74EC404}", BaseJsonSerializer);
        ~SerializerWithTwoDifferentTypes() override = default;

        static void Reflect(AZ::JsonRegistrationContext* context)
        {
            context->Serializer<SerializerWithTwoDifferentTypes>()
                ->HandlesType<bool>()
                ->HandlesType<char>();
        }
    };

    class SerializerWithOneDuplicatedType
        : public JsonSerializerTemplatedMock<SerializerWithOneDuplicatedType>
    {
    public:
        AZ_RTTI(SerializerWithOneDuplicatedType, "{630E65C7-EA98-4611-AC9D-3367093AD275}", BaseJsonSerializer);
        ~SerializerWithOneDuplicatedType() override = default;

        static void Reflect(AZ::JsonRegistrationContext* context)
        {
            context->Serializer<SerializerWithOneDuplicatedType>()
                ->HandlesType<bool>();
        }
    };

    class SerializerWithOneDuplicatedTypeWithOverride
        : public JsonSerializerTemplatedMock<SerializerWithOneDuplicatedTypeWithOverride>
    {
    public:
        AZ_RTTI(SerializerWithOneDuplicatedTypeWithOverride, "{4218D591-E578-499B-B578-ACA70C9944AB}", BaseJsonSerializer);
        ~SerializerWithOneDuplicatedTypeWithOverride() override = default;

        static void Reflect(AZ::JsonRegistrationContext* context)
        {
            context->Serializer<SerializerWithOneDuplicatedTypeWithOverride>()
                ->HandlesType<bool>(true);
        }
    };

    // Attempts to register the same type twice
    class SerializerWithTwoSameTypes
        : public JsonSerializerTemplatedMock<SerializerWithTwoSameTypes>
    {
    public:
        AZ_RTTI(SerializerWithTwoSameTypes, "{6364A67C-7F11-46FC-A462-A765E3870BC5}", BaseJsonSerializer);
        ~SerializerWithTwoSameTypes() override = default;

        static void Reflect(AZ::JsonRegistrationContext* context)
        {
            context->Serializer<SerializerWithTwoSameTypes>()
                ->HandlesType<bool>()
                ->HandlesType<bool>();
        }
    };

    template<typename, typename> struct Template2Typename {};
    template<size_t, size_t> struct Template2Int {};
    template<typename, size_t> struct Template1TypenameInt {};
    template<typename, typename, size_t> struct Template2TypenameInt {};
    template<typename, typename, typename, size_t> struct Template3TypenameInt {};
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(JsonSerializationTests::Template2Typename,    "{1409AAD3-C34E-4B81-BB7B-E968C44EBD2D}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_TYPENAME);
    AZ_TYPE_INFO_TEMPLATE(JsonSerializationTests::Template2Int,         "{91E223F5-4FFF-456B-B0C0-A4646AFACD67}", AZ_TYPE_INFO_AUTO, AZ_TYPE_INFO_AUTO);
    AZ_TYPE_INFO_TEMPLATE(JsonSerializationTests::Template1TypenameInt, "{8D607A55-3BBB-4C29-AA58-7714F49FC8C0}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_AUTO);
    AZ_TYPE_INFO_TEMPLATE(JsonSerializationTests::Template2TypenameInt, "{637DA26D-249D-421A-819F-012237CD416F}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_AUTO);
    AZ_TYPE_INFO_TEMPLATE(JsonSerializationTests::Template3TypenameInt, "{818683B1-0F48-4F0B-B57C-169CF0ED850F}", AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_TYPENAME, AZ_TYPE_INFO_AUTO);
}

namespace JsonSerializationTests
{
    class JsonRegistrationContextTests
        : public UnitTest::AllocatorsTestFixture
    {
    public:
        ~JsonRegistrationContextTests() override = default;

        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();

            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
        }

        void TearDown() override
        {
            m_jsonRegistrationContext.reset();

            UnitTest::AllocatorsTestFixture::TearDown();
        }

        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
    };

    TEST_F(JsonRegistrationContextTests, SerializerWithNoHandledTypes_Succeeds)
    {
        SerializerWithNoType::Reflect(m_jsonRegistrationContext.get());
        EXPECT_EQ(0, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        SerializerWithNoType::Unreflect(m_jsonRegistrationContext.get());
    }

    TEST_F(JsonRegistrationContextTests, RegisterAndUnregisterValidType_Succeeds)
    {
        SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());

        EXPECT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()));
        EXPECT_EQ(1, m_jsonRegistrationContext->GetRegisteredSerializers().size());

        SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());

        EXPECT_EQ(0, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        EXPECT_EQ(m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()), nullptr);
    }

    TEST_F(JsonRegistrationContextTests, RegisterForTemplateTypenameVarArg_Succeeds)
    {
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template2Typename>();
        ASSERT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<Template2Typename>()));
        
        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template2Typename>(); 
        m_jsonRegistrationContext->DisableRemoveReflection();
    }

    TEST_F(JsonRegistrationContextTests, RegisterForTemplateAutoVarArg_Succeeds)
    {
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template2Int>();
        ASSERT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<Template2Int>()));

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template2Int>();
        m_jsonRegistrationContext->DisableRemoveReflection();
    }

    TEST_F(JsonRegistrationContextTests, RegisterForTemplate1TypenameInt_Succeeds)
    {
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template1TypenameInt>();
        ASSERT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<Template1TypenameInt>()));

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template1TypenameInt>();
        m_jsonRegistrationContext->DisableRemoveReflection();
    }

    TEST_F(JsonRegistrationContextTests, RegisterForTemplate2TypenameInt_Succeeds)
    {
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template2TypenameInt>();
        ASSERT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<Template2TypenameInt>()));

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template2TypenameInt>();
        m_jsonRegistrationContext->DisableRemoveReflection();
    }

    TEST_F(JsonRegistrationContextTests, RegisterForTemplate3TypenameInt_Succeeds)
    {
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template3TypenameInt>();
        ASSERT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<Template3TypenameInt>()));

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonRegistrationContext->Serializer<JsonSerializerMock>()->HandlesType<Template3TypenameInt>();
        m_jsonRegistrationContext->DisableRemoveReflection();
    }

    TEST_F(JsonRegistrationContextTests, MultipleMatchedRegisterAndUnregister_Succeeds)
    {
        SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());

        EXPECT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()));
        EXPECT_EQ(1, m_jsonRegistrationContext->GetRegisteredSerializers().size());

        SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());

        EXPECT_EQ(0, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        EXPECT_EQ(m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()), nullptr);

        SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());

        EXPECT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()));
        EXPECT_EQ(1, m_jsonRegistrationContext->GetRegisteredSerializers().size());

        SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());

        EXPECT_EQ(0, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        EXPECT_EQ(m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()), nullptr);
    }

    TEST_F(JsonRegistrationContextTests, RegisterSameSerializerForDifferentTypes_Succeeds)
    {
        SerializerWithTwoDifferentTypes::Reflect(m_jsonRegistrationContext.get());

        EXPECT_EQ(2, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        EXPECT_NE(m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()), nullptr);
        EXPECT_NE(m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<char>()), nullptr);

        SerializerWithTwoDifferentTypes::Unreflect(m_jsonRegistrationContext.get());

        EXPECT_EQ(0, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        EXPECT_EQ(m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()), nullptr);
        EXPECT_EQ(m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<char>()), nullptr);
    }

    TEST_F(JsonRegistrationContextTests, RegisterSameUuidWithMultipleSerializers_Fails)
    {
        EXPECT_NE(AZ::AzTypeInfo<SerializerWithOneDuplicatedType>::Uuid(), AZ::AzTypeInfo<SerializerWithOneType>::Uuid());

        SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());
        AZ_TEST_START_ASSERTTEST;
        SerializerWithOneDuplicatedType::Reflect(m_jsonRegistrationContext.get());
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        AZ::BaseJsonSerializer* mockSerializer = m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>());
        ASSERT_NE(mockSerializer, nullptr);
        EXPECT_EQ(AZ::AzTypeInfo<SerializerWithOneType>::Uuid(), mockSerializer->RTTI_GetType());

        SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());
        SerializerWithOneDuplicatedType::Unreflect(m_jsonRegistrationContext.get());
    }

    TEST_F(JsonRegistrationContextTests, OverwriteRegisterSameUuidWithMultipleSerializers_Succeeds)
    {
        EXPECT_NE(AZ::AzTypeInfo<SerializerWithOneDuplicatedTypeWithOverride>::Uuid(), AZ::AzTypeInfo<SerializerWithOneType>::Uuid());

        SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());
        SerializerWithOneDuplicatedTypeWithOverride::Reflect(m_jsonRegistrationContext.get());
        
        EXPECT_EQ(1, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        AZ::BaseJsonSerializer* mockSerializer = m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>());
        ASSERT_NE(mockSerializer, nullptr);
        EXPECT_EQ(AZ::AzTypeInfo<SerializerWithOneDuplicatedTypeWithOverride>::Uuid(), mockSerializer->RTTI_GetType());

        SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());
        SerializerWithOneDuplicatedTypeWithOverride::Unreflect(m_jsonRegistrationContext.get());
    }

    TEST_F(JsonRegistrationContextTests, RegisterSameUuidWithSameSerializers_Fails)
    {
        AZ_TEST_START_ASSERTTEST;
        SerializerWithTwoSameTypes::Reflect(m_jsonRegistrationContext.get());
        AZ_TEST_STOP_ASSERTTEST(1);

        EXPECT_EQ(1, m_jsonRegistrationContext->GetRegisteredSerializers().size());
        EXPECT_NE(nullptr, m_jsonRegistrationContext->GetSerializerForType(azrtti_typeid<bool>()));

        SerializerWithTwoSameTypes::Unreflect(m_jsonRegistrationContext.get());
    }

    TEST_F(JsonRegistrationContextTests, DoubleRegisterSerializer_Asserts)
    {
        SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());

        AZ_TEST_START_ASSERTTEST;
        SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());
        AZ_TEST_STOP_ASSERTTEST(2); // one for the double reflect and one for the unique_ptr going out of scope before unregistering serializers

        SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());
    }

#if GTEST_HAS_DEATH_TEST
    using JsonSerializationDeathTests = JsonRegistrationContextTests;
    TEST_F(JsonSerializationDeathTests, DoubleUnregisterSerializer_Asserts)
    {
        ASSERT_DEATH({
            SerializerWithOneType::Reflect(m_jsonRegistrationContext.get());
            SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());
            SerializerWithOneType::Unreflect(m_jsonRegistrationContext.get());
            }, ".*"
        );
    }
#endif // GTEST_HAS_DEATH_TEST

} //namespace JsonSerializationTests
