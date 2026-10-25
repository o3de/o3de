/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/MockComponentApplication.h>
#include <AzCore/std/parallel/thread.h>
#include <Tests/DOM/DomFixtures.h>

namespace AZ::Dom::Tests
{
    // ----------------------------------------------------------------------------------
    // Fixture: serialization contexts the JSON conversion finds through the component application bus, as in an application
    // ----------------------------------------------------------------------------------

    struct TypeIdCacheTestClass
    {
        AZ_TYPE_INFO(TypeIdCacheTestClass, "{6E1D7B3A-2F64-4C59-9B8E-0A3C5D7F1E24}");
        int m_value = 0;
    };

    class DomTypeIdCacheTests : public DomTestFixture
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

    // ----------------------------------------------------------------------------------
    // The cache scope
    // ----------------------------------------------------------------------------------

    TEST_F(DomTypeIdCacheTests, InsideAScopeTheValueIsTheOneWithoutIt)
    {
        const AZStd::string withoutScope = Converted();
        ASSERT_EQ(withoutScope, "TypeIdCacheTestClass");

        Utils::TypeIdToDomValueCacheScope scope;
        EXPECT_EQ(Converted(), withoutScope);
        EXPECT_EQ(Converted(), withoutScope);
    }

    TEST_F(DomTypeIdCacheTests, InsideAScopeATypeIsConvertedOnceAndOnlyThere)
    {
        {
            Utils::TypeIdToDomValueCacheScope scope;
            EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
            Unreflect();
            EXPECT_EQ(Converted(), "TypeIdCacheTestClass"); // the value of the first conversion
        }
        EXPECT_EQ(Converted(), ""); // no scope: converted again, and the type has no name now
    }

    TEST_F(DomTypeIdCacheTests, ANestedScopeUsesTheOutermostValues)
    {
        Utils::TypeIdToDomValueCacheScope outer;
        EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
        {
            Utils::TypeIdToDomValueCacheScope inner;
            Unreflect();
            EXPECT_EQ(Converted(), "TypeIdCacheTestClass");
        }
        EXPECT_EQ(Converted(), "TypeIdCacheTestClass"); // the inner scope ending leaves the outer one's values
    }

    TEST_F(DomTypeIdCacheTests, AnotherThreadConvertsForItself)
    {
        Utils::TypeIdToDomValueCacheScope scope;
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
} // namespace AZ::Dom::Tests
