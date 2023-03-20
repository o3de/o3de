/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>

AZ_ENUM_CLASS(GlobalScopeTestEnum, Foo, Bar);
AZ_ENUM_DEFINE_REFLECT_UTILITIES(GlobalScopeTestEnum)

namespace OtherEnumTestNamespace
{
    AZ_ENUM_CLASS(TestEnum, Walk, Run, Fly);
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(TestEnum)
}


namespace UnitTest
{
    AZ_ENUM_CLASS(TestEnum,
                  A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z
    );
    AZ_ENUM_DEFINE_REFLECT_UTILITIES(TestEnum)

    AZ_ENUM(TestEnumUnscoped,
            X,
            (Y, 5)
    );

    AZ_ENUM_WITH_UNDERLYING_TYPE(TestEnum8, uint8_t,
                                 XX,
                                 (YY, 7), 
                                 ZZ
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(TestCEnum16, int16_t,
                                       (X, -10),
                                       Y, 
                                       Z
    );

    class EnumTests : public LeakDetectionFixture
    {
    };

    struct TypeWithEnumMember
    {
        AZ_TYPE_INFO(TypeWithEnumMember, "{1ACB4CCC-8070-4D21-8A52-37FD4391EFF5}");
        TestEnum m_testEnum;
    };

    TEST_F(EnumTests, BasicProperties_AreConstexpr)
    {
        // basic properties verifications:
        static_assert(static_cast<int>(TestEnum::A) == 0);
        static_assert(static_cast<int>(TestEnum::Z) == 25);

        // members array:
        static_assert(TestEnumMembers.size() == 26);
        static_assert(TestEnumMembers[0].m_value == TestEnum::A);
        static_assert(TestEnumMembers[0].m_string == "A");
        static_assert(TestEnumMembers[1].m_value == TestEnum::B);
        static_assert(TestEnumMembers[1].m_string == "B");

        // Unscoped-version tests
        static_assert(Y == 5);

        // 'specific underlying type'-version tests
        static_assert(sizeof(TestEnum8) == sizeof(uint8_t));

        static_assert(XX == 0);
        static_assert(YY == 7);
        static_assert(ZZ == 8);
    }

    TEST_F(EnumTests, EnumerationOverEnum_Succeeds)
    {
        auto EnumerateTestEnum = []() constexpr -> bool
        {
            int count = 0;
            for ([[maybe_unused]] TestEnumEnumeratorValueAndString enumMember : TestEnumMembers)
            {
                ++count;
            }
            return count == 26;
        };
        static_assert(EnumerateTestEnum());
    }

    TEST_F(EnumTests, FromString_WithFoundString_ReturnsEnumValue)
    {
        // optional is not dereferencable in constexpr context because of addressof
        // therefore we can't verify the returned value in static_assert, but we can verify that the function found something.
        static_assert(FromStringToTestEnum("Y").has_value());
        static_assert(TestEnumNamespace::FromString("X").has_value());
        // value verification by runtime:
        EXPECT_TRUE(*FromStringToTestEnum("Y") == TestEnum::Y);
        EXPECT_TRUE(*FromStringToTestEnumUnscoped("Y") == Y);
        // namespace accessed functions versions
        EXPECT_TRUE(*TestEnumNamespace::FromString("Y") == TestEnum::Y);
        EXPECT_TRUE(*TestEnumUnscopedNamespace::FromString("Y") == Y);
        // and on runtime as well
        EXPECT_TRUE(*FromStringToTestEnumUnscoped("X") == X);
    }

    TEST_F(EnumTests, FromString_WithNotFoundString_ReturnsInvalid)
    {
        static_assert(!FromStringToTestEnum("alien"));  // nullopt in boolean context should evaluate to false
        static_assert(!FromStringToTestEnumUnscoped("alien"));
        // namespace accessed functions versions
        static_assert(!TestEnumNamespace::FromString("alien"));
        static_assert(!TestEnumUnscopedNamespace::FromString("alien"));
    }

    TEST_F(EnumTests, ToString_WithValidEnumOption_ReturnsNonEmptyString)
    {
        static_assert(ToString(TestEnum::X) == "X");
        static_assert(ToString(TestEnum::Y) == "Y");

        static_assert(ToString(TestEnum8::XX) == "XX");
        static_assert(ToString(TestEnum8::YY) == "YY");
    }

    TEST_F(EnumTests, ToString_WithInvalidEnumValue_ReturnsEmptyString)
    {
        static_assert(ToString(static_cast<TestEnum>(40)).empty());
        static_assert(ToString(static_cast<TestEnumUnscoped>(15)).empty());
        static_assert(ToString(static_cast<TestEnum8>(20)).empty());
    }

    TEST_F(EnumTests, TraitsBehaviorRegular)
    {
        static_assert(AZ::AzEnumTraits<TestEnum>::Members.size() == 26);
    }

}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(GlobalScopeTestEnum, "{9779918F-E8CE-4B04-A619-FCAA280DCB98}");
    AZ_TYPE_INFO_SPECIALIZE(OtherEnumTestNamespace::TestEnum, "{84CD44D2-7A6F-4271-BA17-3BBFC9C163D0}");
    AZ_TYPE_INFO_SPECIALIZE(UnitTest::TestEnum, "{A826012C-18B9-453E-8A61-7C06503F759D}");
}

namespace UnitTest
{
    TEST_F(EnumTests, ReflectToSerializeContext)
    {
        AZ::SerializeContext context;

        GlobalScopeTestEnumReflect(context);
        TestEnumReflect(context);
        OtherEnumTestNamespace::TestEnumReflect(context);

        // There should be 1 Attributes::EnumUnderlyingType and N Attributes::EnumValueKey

        auto enumClassData = context.FindClassData(azrtti_typeid<GlobalScopeTestEnum>());
        EXPECT_NE(nullptr, enumClassData);
        EXPECT_EQ(3, enumClassData->m_attributes.size());

        enumClassData = context.FindClassData(azrtti_typeid<TestEnum>());
        EXPECT_NE(nullptr, enumClassData);
        EXPECT_EQ(27, enumClassData->m_attributes.size());

        enumClassData = context.FindClassData(azrtti_typeid<OtherEnumTestNamespace::TestEnum>());
        EXPECT_NE(nullptr, enumClassData);
        EXPECT_EQ(4, enumClassData->m_attributes.size());
    }

    TEST_F(EnumTests, ReflectToBehaviorContext)
    {
        AZ::BehaviorContext context;

        GlobalScopeTestEnumReflect(context);
        TestEnumReflect(context);
        OtherEnumTestNamespace::TestEnumReflect(context, "OtherTestEnum");

        EXPECT_NE(context.m_properties.end(), context.m_properties.find("GlobalScopeTestEnum_Foo"));
        EXPECT_NE(context.m_properties.end(), context.m_properties.find("GlobalScopeTestEnum_Bar"));

        EXPECT_NE(context.m_properties.end(), context.m_properties.find("TestEnum_A"));
        EXPECT_NE(context.m_properties.end(), context.m_properties.find("TestEnum_Z"));

        EXPECT_NE(context.m_properties.end(), context.m_properties.find("OtherTestEnum_Walk"));
        EXPECT_NE(context.m_properties.end(), context.m_properties.find("OtherTestEnum_Run"));
        EXPECT_NE(context.m_properties.end(), context.m_properties.find("OtherTestEnum_Fly"));
    }

    TEST_F(EnumTests, BehaviorContextReflect_Contains_BehaviorClassOfEnum)
    {
        AZ::BehaviorContext context;

        TestEnumReflect(context);

        // Validate that the enum type is reflected as a BehaviorClass on the BehaviorContext
        auto enumClassIter = context.m_classes.find("UnitTest::TestEnum");
        ASSERT_NE(context.m_classes.end(), enumClassIter);

        // Verify that the enum type BehaviorClass contains properties for the enum options
        AZ::BehaviorClass* enumClass = enumClassIter->second;
        auto VerifyEnumOptionReflectionOnEnumType = [&properties = enumClass->m_properties]
        (TestEnum expectedValue, AZStd::string_view enumOptionName)
        {
            auto propertyIt = properties.find(enumOptionName);
            ASSERT_NE(properties.end(), propertyIt);
            TestEnum testValue{};
            EXPECT_TRUE(propertyIt->second->m_getter->InvokeResult(testValue));
            EXPECT_EQ(expectedValue, testValue);
        };
        AZ::AzEnumTraits<TestEnum>::Visit(VerifyEnumOptionReflectionOnEnumType);

        // Verify that the enum options are also available as global constants on the BehaviorContext
        auto VerifyEnumOptionReflectionAsGlobalConstant = [&properties = context.m_properties]
        (TestEnum expectedValue, AZStd::string_view enumOptionName)
        {
            // Compose the combinded global enum option name
            auto qualifiedOptionName = AZStd::fixed_string<256>::format("TestEnum_%.*s",
                AZ_STRING_ARG(enumOptionName));
            auto propertyIt = properties.find(AZStd::string_view(qualifiedOptionName));
            ASSERT_NE(properties.end(), propertyIt);
            TestEnum testValue{};
            EXPECT_TRUE(propertyIt->second->m_getter->InvokeResult(testValue));
            EXPECT_EQ(expectedValue, testValue);
        };
        AZ::AzEnumTraits<TestEnum>::Visit(VerifyEnumOptionReflectionAsGlobalConstant);
    }

    TEST_F(EnumTests, ScriptContext_BindTo_BehaviorContext_DoesNotAssert_WithEnumType_ReflectedAsProperty)
    {
        AZ::BehaviorContext context;

        TestEnumReflect(context);

        context.Class<TypeWithEnumMember>()
            ->Property("testenum", BehaviorValueProperty(&TypeWithEnumMember::m_testEnum))
            ;

        AZ::ScriptContext scriptContext;
        scriptContext.BindTo(&context);
    }
}
