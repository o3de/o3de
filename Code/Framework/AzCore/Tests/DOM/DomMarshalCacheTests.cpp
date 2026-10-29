/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/PointerObject.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/MockComponentApplication.h>
#include <AzCore/std/parallel/thread.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    // ----------------------------------------------------------------------------------
    // Fixture: serialization contexts the JSON path finds through the component application bus, as in an application
    // ----------------------------------------------------------------------------------

    struct TypeIdCacheTestClass
    {
        AZ_TYPE_INFO(TypeIdCacheTestClass, "{6E1D7B3A-2F64-4C59-9B8E-0A3C5D7F1E24}");
        int m_value = 0;
    };

    class DomMarshalCacheTests : public DomTestFixture
    {
    public:
        void SetUp() override
        {
            DomTestFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
            m_componentApplication = AZStd::make_unique<::testing::NiceMock<UnitTest::MockComponentApplication>>();
            ON_CALL(*m_componentApplication, GetSerializeContext()).WillByDefault(::testing::Return(m_serializeContext.get()));
            ON_CALL(*m_componentApplication, GetJsonRegistrationContext())
                .WillByDefault(::testing::Return(m_jsonRegistrationContext.get()));
            Reflect();
        }

        void TearDown() override
        {
            m_componentApplication.reset();
            m_jsonRegistrationContext.reset();
            m_serializeContext.reset();
            DomTestFixture::TearDown();
        }

        void Reflect()
        {
            m_serializeContext->Class<TypeIdCacheTestClass>()->Field("value", &TypeIdCacheTestClass::m_value);
        }

        //! After this, converting the type gives no name: only a cached value still has it
        void Unreflect()
        {
            m_serializeContext->EnableRemoveReflection();
            Reflect();
            m_serializeContext->DisableRemoveReflection();
        }

        static AZStd::string Converted()
        {
            return AZStd::string(Utils::TypeIdToDomValue(azrtti_typeid<TypeIdCacheTestClass>()).GetString());
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZStd::unique_ptr<::testing::NiceMock<UnitTest::MockComponentApplication>> m_componentApplication;
    };

    //! The stock JSON serializers registered, PointerJsonSerializer among them, as the JsonSystemComponent registers them
    class DomMarshalCachePointerObjectTests : public DomMarshalCacheTests
    {
    public:
        void SetUp() override
        {
            DomMarshalCacheTests::SetUp();
            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
        }

        void TearDown() override
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();
            DomMarshalCacheTests::TearDown();
        }
    };

    // ----------------------------------------------------------------------------------
    // Type ids
    // ----------------------------------------------------------------------------------

    TEST_F(DomMarshalCacheTests, TypeId_InsideAScopeTheValueIsTheOneWithoutIt)
    {
        const AZStd::string withoutScope = Converted();
        ASSERT_EQ(withoutScope, "TypeIdCacheTestClass");

        Utils::MarshalCacheScope scope;
        EXPECT_EQ(Converted(), withoutScope);
        EXPECT_EQ(Converted(), withoutScope);
    }

    TEST_F(DomMarshalCacheTests, TypeId_InsideAScopeATypeIsConvertedOnceAndOnlyThere)
    {
        {
            Utils::MarshalCacheScope scope;
            EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
            Unreflect();
            EXPECT_EQ(Converted(), "TypeIdCacheTestClass"); // the value of the first conversion
        }
        EXPECT_EQ(Converted(), ""); // no scope: converted again, and the type has no name now
    }

    TEST_F(DomMarshalCacheTests, TypeId_ANestedScopeUsesTheOutermostValues)
    {
        Utils::MarshalCacheScope outer;
        EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
        {
            Utils::MarshalCacheScope inner;
            Unreflect();
            EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
        }
        EXPECT_EQ(Converted(), "TypeIdCacheTestClass"); // the inner scope ending leaves the outer one's values
    }

    TEST_F(DomMarshalCacheTests, TypeId_AnotherThreadConvertsForItself)
    {
        Utils::MarshalCacheScope scope;
        EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
        Unreflect();

        AZStd::string onOtherThread = "not converted";
        AZStd::thread other(
            [&onOtherThread]()
            {
                onOtherThread = Converted();
            });
        other.join();
        EXPECT_EQ(onOtherThread, "");
        EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
    }

    // ----------------------------------------------------------------------------------
    // PointerObjects
    // ----------------------------------------------------------------------------------

    TEST_F(DomMarshalCachePointerObjectTests, PointerObject_BuiltDirectlyItIsTheValueTheJsonPathGives)
    {
        int someValue = 0;
        void* const addresses[] = { nullptr, &someValue, reinterpret_cast<void*>(uintptr_t{ 0x7FFFFFFFFFFFFFFF }),
                                    reinterpret_cast<void*>(uintptr_t{ 0x8000000000000001 }) }; // both sides of INT64_MAX
        const AZ::TypeId typeIds[] = { AZ::TypeId::CreateNull(), azrtti_typeid<int>(), azrtti_typeid<TypeIdCacheTestClass>() };
        for (void* address : addresses)
        {
            for (const AZ::TypeId& typeId : typeIds)
            {
                AZ::PointerObject pointerObject;
                pointerObject.m_address = address;
                pointerObject.m_typeId = typeId;

                const Value throughJson = Utils::ValueFromType(pointerObject);
                ASSERT_TRUE(throughJson.IsObject()); // the stock serializer stored it (no opaque fall-back)
                Value built;
                {
                    Utils::MarshalCacheScope scope;
                    built = Utils::ValueFromType(pointerObject);
                }
                EXPECT_TRUE(Utils::DeepCompareIsEqual(throughJson, built))
                    << reinterpret_cast<uintptr_t>(address) << " " << typeId.ToFixedString().c_str();
            }
        }
    }

    TEST_F(DomMarshalCacheTests, PointerObject_WithoutTheStockSerializerTheScopeLeavesTheJsonPath)
    {
        AZ::PointerObject pointerObject;
        pointerObject.m_typeId = azrtti_typeid<int>();

        const Value withoutScope = Utils::ValueFromType(pointerObject); // no serializer registered: kept opaque
        EXPECT_EQ(withoutScope.GetType(), Type::Opaque);

        Utils::MarshalCacheScope scope;
        EXPECT_EQ(Utils::ValueFromType(pointerObject).GetType(), Type::Opaque);
    }
} // namespace AZ::Dom::Tests
