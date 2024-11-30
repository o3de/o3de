/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace SettingsRegistryTests
{
    class TestClass
    {
    public:
        AZ_TYPE_INFO(TestClass, "{CDD9648A-27CA-4625-9FD9-DD3BB9CB093D}");

        int m_var1 = { 42 };
        double m_var2 = { 42.0 };

        static TestClass Initialize()
        {
            TestClass result;
            result.m_var1 = 88;
            result.m_var2 = 88.0;
            return result;
        }

        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<TestClass>()
                ->Field("Var1", &TestClass::m_var1)
                ->Field("Var2", &TestClass::m_var2);
        }
    };

    struct RegistryEntry
    {
        AZStd::string_view m_path;
        AZStd::string_view m_valueName;
        AZ::SettingsRegistryInterface::Type m_type;
        AZ::SettingsRegistryInterface::VisitAction m_action;
        AZ::SettingsRegistryInterface::VisitResponse m_response;

        RegistryEntry() = default;
        RegistryEntry(AZStd::string_view path, AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type type,
            AZ::SettingsRegistryInterface::VisitAction action,
            AZ::SettingsRegistryInterface::VisitResponse response = AZ::SettingsRegistryInterface::VisitResponse::Continue)
            : m_path(path)
            , m_valueName(valueName)
            , m_type(type)
            , m_action(action)
            , m_response(response)
        {}
    };

    class SettingsRegistryTest
        : public UnitTest::LeakDetectionFixture
    {
    public:
        ~SettingsRegistryTest() override = default;

        void SetUp() override
        {
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_registrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            m_registry->SetContext(m_serializeContext.get());
            m_registry->SetContext(m_registrationContext.get());

            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());
        }

        void TearDown() override
        {
            m_registrationContext->EnableRemoveReflection();
            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());
            m_registrationContext->DisableRemoveReflection();

            m_registry.reset();
            m_registrationContext.reset();
            m_serializeContext.reset();
        }

        void Visit(const AZStd::vector<RegistryEntry>& expected, AZStd::string_view path = "")
        {
            size_t counter = 0;
            auto callback = [&expected, &counter](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs,
                AZ::SettingsRegistryInterface::VisitAction action)
            {
                EXPECT_LT(counter, expected.size());
                if (counter < expected.size())
                {
                    const RegistryEntry& entry = expected[counter];
                    EXPECT_STREQ(entry.m_path.data(), visitArgs.m_jsonKeyPath.data());
                    EXPECT_STREQ(entry.m_valueName.data(), visitArgs.m_fieldName.data());
                    EXPECT_EQ(entry.m_action, action);
                    EXPECT_EQ(entry.m_type, visitArgs.m_type);
                    counter++;
                    return entry.m_response;
                }
                return AZ::SettingsRegistryInterface::VisitResponse::Done;
            };
            EXPECT_TRUE(m_registry->Visit(callback, path));
            EXPECT_EQ(counter, expected.size());
        }

        void MergeNotify(AZStd::string_view path, size_t index, size_t fileIdLength, const char* counterId, const char** fileIds)
        {
            EXPECT_TRUE(path.empty());
            AZ::s64 value = -1;
            bool result = m_registry->Get(value, counterId);
            EXPECT_TRUE(result);
            EXPECT_EQ(index, value);

            size_t i = 0;
            for (; i <= index; ++i)
            {
                bool configValue = false;
                result = m_registry->Get(configValue, fileIds[i]);
                EXPECT_TRUE(result);
                EXPECT_TRUE(configValue);
            }
            for (; i < fileIdLength; ++i)
            {
                AZ::SettingsRegistryInterface::Type type = m_registry->GetType(fileIds[i]);
                EXPECT_EQ(AZ::SettingsRegistryInterface::Type::NoType, type);
            }
        }

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
    };

    template<typename> struct SettingsType {};

    // Json makes a distinction between true and false as different types, so test for both
    // versions.
    struct BoolTrue {};
    struct BoolFalse {};

    template<> struct SettingsType<BoolTrue>
    {
        using DataType = bool;
        using ValueType = bool;
        constexpr static AZ::SettingsRegistryInterface::Type s_type = AZ::SettingsRegistryInterface::Type::Boolean;
        static AZStd::string_view GetStoredJson() { return "true"; }
        static DataType GetStoredValue() { return true; }
        static DataType GetDefaultValue() { return false; }
        static void ExpectEq(ValueType lhs, ValueType rhs) { EXPECT_EQ(lhs, rhs); }
    };

    template<> struct SettingsType<BoolFalse>
    {
        using DataType = bool;
        using ValueType = bool;
        constexpr static AZ::SettingsRegistryInterface::Type s_type = AZ::SettingsRegistryInterface::Type::Boolean;
        static AZStd::string_view GetStoredJson() { return "false"; }
        static DataType GetStoredValue() { return false; }
        static DataType GetDefaultValue() { return true; }
        static void ExpectEq(ValueType lhs, ValueType rhs) { EXPECT_EQ(lhs, rhs); }
    };

    template<> struct SettingsType<AZ::s64>
    {
        using DataType = AZ::s64;
        using ValueType = AZ::s64;
        constexpr static AZ::SettingsRegistryInterface::Type s_type = AZ::SettingsRegistryInterface::Type::Integer;
        static AZStd::string_view GetStoredJson() { return "-88"; }
        static DataType GetStoredValue() { return -88; }
        static DataType GetDefaultValue() { return 42; }
        static void ExpectEq(ValueType lhs, ValueType rhs) { EXPECT_EQ(lhs, rhs); }
    };

    template<> struct SettingsType<double>
    {
        using DataType = double;
        using ValueType = double;
        constexpr static AZ::SettingsRegistryInterface::Type s_type = AZ::SettingsRegistryInterface::Type::FloatingPoint;
        static AZStd::string_view GetStoredJson() { return "88.0"; }
        static DataType GetStoredValue() { return 88.0; }
        static DataType GetDefaultValue() { return 42.0; }
        static void ExpectEq(DataType lhs, DataType rhs) { EXPECT_DOUBLE_EQ(lhs, rhs); }
    };

    template<> struct SettingsType<AZStd::string>
    {
        using DataType = AZStd::string;
        using ValueType = AZStd::string_view;
        constexpr static AZ::SettingsRegistryInterface::Type s_type = AZ::SettingsRegistryInterface::Type::String;
        static AZStd::string_view GetStoredJson() { return R"("World")"; }
        static DataType GetStoredValue() { return "World"; }
        static DataType GetDefaultValue() { return AZStd::string{}; }
        static void ExpectEq(ValueType lhs, ValueType rhs) { EXPECT_STREQ(lhs.data(), rhs.data()); }
    };

    template<> struct SettingsType<AZ::SettingsRegistryInterface::FixedValueString>
    {
        using DataType = AZ::SettingsRegistryInterface::FixedValueString;
        using ValueType = AZStd::string_view;
        constexpr static AZ::SettingsRegistryInterface::Type s_type = AZ::SettingsRegistryInterface::Type::String;
        static AZStd::string_view GetStoredJson() { return R"("World")"; }
        static DataType GetStoredValue() { return "World"; }
        static DataType GetDefaultValue() { return DataType{}; }
        static void ExpectEq(ValueType lhs, ValueType rhs) { EXPECT_STREQ(lhs.data(), rhs.data()); }
    };

    template<typename SettingsType>
    class TypedSettingsRegistryTest
        : public SettingsRegistryTest
    {
    public:
        ~TypedSettingsRegistryTest() override = default;
    };

    using SettingsTypes = ::testing::Types<
        BoolTrue, BoolFalse, AZ::s64, double, AZStd::string, AZ::SettingsRegistryInterface::FixedValueString>;
    TYPED_TEST_CASE(TypedSettingsRegistryTest, SettingsTypes);

    TYPED_TEST(TypedSettingsRegistryTest, GetSet_SetAndGetValue_Success)
    {
        typename SettingsType<TypeParam>::DataType value = SettingsType<TypeParam>::GetStoredValue();
        typename SettingsType<TypeParam>::DataType readValue = SettingsType<TypeParam>::GetDefaultValue();

        AZStd::string_view testPath = "/Test/Path/Value";
        ASSERT_TRUE(this->m_registry->Set(testPath, value));
        ASSERT_TRUE(this->m_registry->Get(readValue, testPath));
        SettingsType<TypeParam>::ExpectEq(value, readValue);
    }

    TYPED_TEST(TypedSettingsRegistryTest, Get_InvalidPath_ReturnsFalse)
    {
        typename SettingsType<TypeParam>::DataType readValue = SettingsType<TypeParam>::GetDefaultValue();
        AZStd::string_view testPath = "#$%^";
        EXPECT_FALSE(this->m_registry->Get(readValue, testPath));
    }

    TYPED_TEST(TypedSettingsRegistryTest, Get_UnknownPath_ReturnsFalse)
    {
        typename SettingsType<TypeParam>::DataType readValue = SettingsType<TypeParam>::GetDefaultValue();
        AZStd::string_view testPath = "/Unknown/Path";
        EXPECT_FALSE(this->m_registry->Get(readValue, testPath));
    }

    TYPED_TEST(TypedSettingsRegistryTest, Get_InvalidType_ReturnsFalse)
    {
        ASSERT_TRUE(this->m_registry->MergeSettings(R"({ "Object": { "Value": 42 } })", AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        typename SettingsType<TypeParam>::DataType readValue = SettingsType<TypeParam>::GetDefaultValue();
        AZStd::string_view testPath = "/Object";
        EXPECT_FALSE(this->m_registry->Get(readValue, testPath));
    }

    TYPED_TEST(TypedSettingsRegistryTest, Set_InvalidPath_ReturnsFalse)
    {
        typename SettingsType<TypeParam>::DataType value = SettingsType<TypeParam>::GetStoredValue();
        AZStd::string_view testPath = "#$%^";
        EXPECT_FALSE(this->m_registry->Set(testPath, value));
    }

    TYPED_TEST(TypedSettingsRegistryTest, Set_NotifiersCalled_ReturnsFalse)
    {
        size_t counter = 0;
        auto callback0 = [&counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_EQ("/Object/Value", notifyEventArgs.m_jsonKeyPath);
            counter++;
        };
        auto callback1 = [&counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            AZ::SettingsRegistryInterface::Type type = SettingsType<TypeParam>::s_type;
            EXPECT_EQ(notifyEventArgs.m_type, type);
            counter++;
        };
        auto testNotifier1 = this->m_registry->RegisterNotifier(callback0);
        auto testNotifier2 = this->m_registry->RegisterNotifier(AZStd::move(callback1));

        typename SettingsType<TypeParam>::DataType value = SettingsType<TypeParam>::GetStoredValue();
        AZStd::string_view testPath = "/Object/Value";
        EXPECT_TRUE(this->m_registry->Set(testPath, value));

        EXPECT_EQ(2, counter);
    }

    TYPED_TEST(TypedSettingsRegistryTest, Set_ConnectedNotifierCalledAndDisconnectedNotifierNotCalled_Success)
    {
        size_t counter1{};
        auto callback0 = [&counter1](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_EQ("/Object/Value", notifyEventArgs.m_jsonKeyPath);
            counter1++;
        };

        size_t counter2{};
        auto callback1 = [&counter2](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_EQ("/Object/Value", notifyEventArgs.m_jsonKeyPath);
            counter2++;
        };;

        auto testNotifier1 = this->m_registry->RegisterNotifier(callback0);
        auto testNotifier2 = this->m_registry->RegisterNotifier(AZStd::move(callback1));

        typename SettingsType<TypeParam>::DataType value = SettingsType<TypeParam>::GetStoredValue();
        AZStd::string_view testPath = "/Object/Value";
        EXPECT_TRUE(this->m_registry->Set(testPath, value));

        EXPECT_EQ(1, counter1);
        EXPECT_EQ(1, counter2);

        // Disconnect the second Notifier
        testNotifier2.Disconnect();
        EXPECT_TRUE(this->m_registry->Set(testPath, SettingsType<TypeParam>::GetDefaultValue()));

        EXPECT_EQ(2, counter1);
        EXPECT_EQ(1, counter2);
    }

    TYPED_TEST(TypedSettingsRegistryTest, GetType_TypeForTheStoredValue_TypeMatchesProvidedType)
    {
        typename SettingsType<TypeParam>::DataType value = SettingsType<TypeParam>::GetStoredValue();
        AZ::SettingsRegistryInterface::Type type = SettingsType<TypeParam>::s_type;

        AZStd::string_view testPath = "/Test/Path/Value";
        ASSERT_TRUE(this->m_registry->Set(testPath, value));
        EXPECT_EQ(type, this->m_registry->GetType(testPath));
    }

    TYPED_TEST(TypedSettingsRegistryTest, VisitWithVisitor_VisitingObject_ValuesInJsonAreVisited)
    {
        AZStd::string_view storedJson = SettingsType<TypeParam>::GetStoredJson();
        AZStd::string json = AZStd::string::format(
            R"({
                "Test":
                {
                    "Object":{ "Type": %.*s }
                }
            })", aznumeric_cast<int>(storedJson.length()), storedJson.data());
        ASSERT_TRUE(this->m_registry->MergeSettings(json.c_str(), AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        struct : public AZ::SettingsRegistryInterface::Visitor
        {
            using AZ::SettingsRegistryInterface::Visitor::Visit;

            using ValueType [[maybe_unused]] = typename SettingsType<TypeParam>::ValueType;
            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, ValueType value) override
            {
                AZ::SettingsRegistryInterface::Type expectedType = SettingsType<TypeParam>::s_type;
                EXPECT_EQ(expectedType, visitArgs.m_type);
                SettingsType<TypeParam>::ExpectEq(SettingsType<TypeParam>::GetStoredValue(), value);
                m_counter++;
            }
            size_t m_counter{ 0 };
        } visitor;

        EXPECT_TRUE(this->m_registry->Visit(visitor, "/Test"));
        EXPECT_EQ(1, visitor.m_counter);
    }

    TYPED_TEST(TypedSettingsRegistryTest, VisitWithVisitor_VisitingArray_ValuesInJsonAreVisited)
    {
        AZStd::string_view storedJson = SettingsType<TypeParam>::GetStoredJson();
        AZStd::string json = AZStd::string::format(
            R"({
                "Test":
                {
                    "Array":[ %.*s ]
                }
            })", aznumeric_cast<int>(storedJson.length()), storedJson.data());
        ASSERT_TRUE(this->m_registry->MergeSettings(json.c_str(), AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        struct : public AZ::SettingsRegistryInterface::Visitor
        {
            using AZ::SettingsRegistryInterface::Visitor::Visit;

            using ValueType [[maybe_unused]] = typename SettingsType<TypeParam>::ValueType;
            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, ValueType value) override
            {
                AZ::SettingsRegistryInterface::Type expectedType = SettingsType<TypeParam>::s_type;
                EXPECT_EQ(expectedType, visitArgs.m_type);
                SettingsType<TypeParam>::ExpectEq(SettingsType<TypeParam>::GetStoredValue(), value);
                m_counter++;
            }
            size_t m_counter{ 0 };
        } visitor;

        EXPECT_TRUE(this->m_registry->Visit(visitor, "/Test"));
        EXPECT_EQ(1, visitor.m_counter);
    }

    TEST_F(SettingsRegistryTest, VisitWithVisitor_VisitingIntegerGreaterThanNumericLimitsOfSigned64Bit_SuppliesCorrectBitPattern)
    {
        // Create a 64-bit value that is greater than what can be represented in a signed int64_t
        constexpr AZ::u64 unsigned64BitValue = aznumeric_cast<AZ::u64>((std::numeric_limits<AZ::s64>::max)()) + 1;
        AZStd::string json = AZStd::string::format(
            R"({
                "Test":
                {
                    "Object":{ "Type": %llu }
                }
            })", unsigned64BitValue);
        ASSERT_TRUE(this->m_registry->MergeSettings(json.c_str(), AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        struct : public AZ::SettingsRegistryInterface::Visitor
        {
            using AZ::SettingsRegistryInterface::Visitor::Visit;
            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZ::s64 value) override
            {
                EXPECT_EQ(AZ::SettingsRegistryInterface::Type::Integer, visitArgs.m_type);
                AZ::u64 testValue = reinterpret_cast<AZ::u64&>(value);
                EXPECT_EQ(expectedValue, testValue);

            }
            const AZ::u64 expectedValue{ unsigned64BitValue };
        } visitor;

        EXPECT_TRUE(this->m_registry->Visit(visitor, "/Test/Object/Type"));
    }

    TEST_F(SettingsRegistryTest, VisitWithVisitor_EndOfPathArguments_MatchesValueNameArgument_ForAllIterations)
    {
        // Validate that every invocation of the Traverse and Visit command supplies a 'path' parameter
        // whose end matches that of the 'valueName' parameter
        AZStd::string json{
            R"({
                "Test":
                {
                    "Object":{ "Type": "TestString" }
                }
            })"
        };
        ASSERT_TRUE(this->m_registry->MergeSettings(json.c_str(), AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        struct : public AZ::SettingsRegistryInterface::Visitor
        {
            AZ::SettingsRegistryInterface::VisitResponse Traverse(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs,
                AZ::SettingsRegistryInterface::VisitAction) override
            {
                EXPECT_TRUE(visitArgs.m_jsonKeyPath.ends_with(visitArgs.m_fieldName));
                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
            }

            using AZ::SettingsRegistryInterface::Visitor::Visit;
            void Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, AZStd::string_view)override
            {
                EXPECT_TRUE(visitArgs.m_jsonKeyPath.ends_with(visitArgs.m_fieldName));
            }
        } visitor;

        EXPECT_TRUE(this->m_registry->Visit(visitor, ""));
        EXPECT_TRUE(this->m_registry->Visit(visitor, "/Test"));
    }

    //
    // Object
    //

    TEST_F(SettingsRegistryTest, GetSetObject_SetAndGetValue_Success)
    {
        TestClass::Reflect(*m_serializeContext);

        AZStd::string_view testPath = "/Test/Path/Value";
        TestClass value;
        value.m_var1 = 88;
        value.m_var2 = 88.0;
        TestClass readValue;
        ASSERT_TRUE(m_registry->SetObject(testPath, &value, azrtti_typeid(value)));
        ASSERT_TRUE(m_registry->GetObject(&readValue, azrtti_typeid(readValue), testPath));
        EXPECT_EQ(value.m_var1, readValue.m_var1);
        EXPECT_DOUBLE_EQ(value.m_var2, readValue.m_var2);

        m_serializeContext->EnableRemoveReflection();
        TestClass::Reflect(*m_serializeContext);
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(SettingsRegistryTest, GetObject_InvalidPath_ReturnsFalse)
    {
        TestClass::Reflect(*m_serializeContext);

        AZStd::string_view testPath = "$%^&";
        TestClass readValue;

        EXPECT_FALSE(m_registry->GetObject(&readValue, azrtti_typeid(readValue), testPath));

        m_serializeContext->EnableRemoveReflection();
        TestClass::Reflect(*m_serializeContext);
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(SettingsRegistryTest, GetObject_UnknownPath_ReturnsFalse)
    {
        TestClass::Reflect(*m_serializeContext);

        AZStd::string_view testPath = "/Unknown/Path";
        TestClass readValue;

        EXPECT_FALSE(m_registry->GetObject(&readValue, azrtti_typeid(readValue), testPath));

        m_serializeContext->EnableRemoveReflection();
        TestClass::Reflect(*m_serializeContext);
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(SettingsRegistryTest, SetObject_InvalidPath_ReturnFalse)
    {
        TestClass::Reflect(*m_serializeContext);

        AZStd::string_view testPath = "$%^&";
        TestClass value;

        EXPECT_FALSE(m_registry->SetObject(testPath, &value, azrtti_typeid(value)));

        m_serializeContext->EnableRemoveReflection();
        TestClass::Reflect(*m_serializeContext);
        m_serializeContext->DisableRemoveReflection();
    }

    TEST_F(SettingsRegistryTest, SetObject_NotifiersCalled_ReturnsFalse)
    {
        size_t counter = 0;
        auto callback0 = [&counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_EQ("/Object/Value", notifyEventArgs.m_jsonKeyPath);
            counter++;
        };
        auto callback1 = [&counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_EQ(notifyEventArgs.m_type, AZ::SettingsRegistryInterface::Type::Object);
            counter++;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback0);
        auto testNotifier2 = m_registry->RegisterNotifier(AZStd::move(callback1));

        TestClass::Reflect(*m_serializeContext);

        AZStd::string_view testPath = "/Object/Value";
        TestClass value;

        EXPECT_TRUE(m_registry->SetObject(testPath, &value, azrtti_typeid(value)));
        EXPECT_EQ(2, counter);

        m_serializeContext->EnableRemoveReflection();
        TestClass::Reflect(*m_serializeContext);
        m_serializeContext->DisableRemoveReflection();
    }

    //
    // Remove
    //
    TYPED_TEST(TypedSettingsRegistryTest, Remove_SetAndRemoveValue_Success)
    {
        typename SettingsType<TypeParam>::DataType value = SettingsType<TypeParam>::GetStoredValue();
        typename SettingsType<TypeParam>::DataType newValue = SettingsType<TypeParam>::GetDefaultValue();

        AZStd::string_view testPath = "/Test/Path/Value";
        EXPECT_TRUE(this->m_registry->Set(testPath, value));
        EXPECT_TRUE(this->m_registry->Get(newValue, testPath));
        SettingsType<TypeParam>::ExpectEq(value, newValue);
        EXPECT_TRUE(this->m_registry->Remove(testPath));

        typename SettingsType<TypeParam>::DataType notFoundValue{};
        EXPECT_FALSE(this->m_registry->Get(notFoundValue, testPath));
    }

    TYPED_TEST(TypedSettingsRegistryTest, Remove_InvalidPath_ReturnsFalse)
    {
        AZStd::string_view testPath = "#$%^";
        EXPECT_FALSE(this->m_registry->Remove(testPath));
        typename SettingsType<TypeParam>::DataType notFoundValue{};
        EXPECT_FALSE(this->m_registry->Get(notFoundValue, testPath));
    }

    TYPED_TEST(TypedSettingsRegistryTest, Remove_UnknownPath_ReturnsFalse)
    {
        AZStd::string_view testPath = "/Unknown/Path";
        EXPECT_FALSE(this->m_registry->Remove(testPath));
        typename SettingsType<TypeParam>::DataType notFoundValue{};
        EXPECT_FALSE(this->m_registry->Get(notFoundValue, testPath));
    }

    //
    // Specializations::Append
    //

    TEST_F(SettingsRegistryTest, SpecializationsAppend_TooManySpecializations_ReportsErrorAndReturnsFalse)
    {
        AZ::SettingsRegistryInterface::Specializations specializations;
        for (size_t i = 0; i < AZ::SettingsRegistryInterface::Specializations::MaxCount; ++i)
        {
            specializations.Append("test");
        }

        AZ_TEST_START_TRACE_SUPPRESSION;
        bool result = specializations.Append("TooFar");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(result);
    }

    //
    // GetType
    // Note: The typed tests already test several GetType versions.
    //

    TEST_F(SettingsRegistryTest, GetType_TypeForObject_TypeMatchesProvidedType)
    {
        ASSERT_TRUE(m_registry->MergeSettings(R"({ "Object": { "Value": 42 } })", AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        AZStd::string_view testPath = "/Object";
        EXPECT_EQ(AZ::SettingsRegistryImpl::Type::Object, m_registry->GetType(testPath));
    }

    TEST_F(SettingsRegistryTest, GetType_TypeForArray_TypeMatchesProvidedType)
    {
        ASSERT_TRUE(m_registry->MergeSettings(R"({ "Array": [ 42 ] })", AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        AZStd::string_view testPath = "/Array";
        EXPECT_EQ(AZ::SettingsRegistryImpl::Type::Array, m_registry->GetType(testPath));
    }

    TEST_F(SettingsRegistryTest, GetType_TypeForNull_TypeMatchesProvidedType)
    {
        ASSERT_TRUE(m_registry->MergeSettings(
            R"( [
                    { "op": "add", "path": "/Object", "value": {"Value": null} }
                ])",
            AZ::SettingsRegistryInterface::Format::JsonPatch));
        AZStd::string_view testPath = "/Object/Value";
        EXPECT_EQ(AZ::SettingsRegistryImpl::Type::Null, m_registry->GetType(testPath));
    }

    TEST_F(SettingsRegistryTest, GetType_InvalidPath_ReturnNone)
    {
        AZ::SettingsRegistryInterface::Type type = m_registry->GetType("#$%");
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::NoType, type);
    }

    TEST_F(SettingsRegistryTest, GetType_UnknownPath_ReturnNone)
    {
        AZ::SettingsRegistryInterface::Type type = m_registry->GetType("/Unknown/Path");
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::NoType, type);
    }

    //
    // Visit
    //

    TEST_F(SettingsRegistryTest, VisitWithVisitor_InvalidPath_ReturnsFalse)
    {
        struct : public AZ::SettingsRegistryInterface::Visitor
        {
        } visitor;

        EXPECT_FALSE(m_registry->Visit(visitor, "#$%"));
    }

    TEST_F(SettingsRegistryTest, VisitWithVisitor_UnknownPath_ReturnsFalse)
    {
        struct : public AZ::SettingsRegistryInterface::Visitor
        {
        } visitor;

        EXPECT_FALSE(m_registry->Visit(visitor, "/Unknown/Path"));
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_FullIteration_AllFieldsAreReported)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"( [
                    { "op": "add", "path": "/Test", "value": { "Object": {} } },

                    { "op": "add", "path": "/Test/Object/NullType", "value": null },
                    { "op": "add", "path": "/Test/Object/TrueType", "value": true },
                    { "op": "add", "path": "/Test/Object/FalseType", "value": false },
                    { "op": "add", "path": "/Test/Object/IntType", "value": -42 },
                    { "op": "add", "path": "/Test/Object/UIntType", "value": 42 },
                    { "op": "add", "path": "/Test/Object/DoubleType", "value": 42.0 },
                    { "op": "add", "path": "/Test/Object/StringType", "value": "Hello world" },

                    { "op": "add", "path": "/Test/Array", "value": [ null, true, false, -42, 42, 42.0, "Hello world" ] }
                ])", SRI::Format::JsonPatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::Begin),

            RegistryEntry("/Test/Object", "Object", SRI::Type::Object, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Object/NullType", "NullType", SRI::Type::Null, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object/TrueType", "TrueType", SRI::Type::Boolean, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object/FalseType", "FalseType", SRI::Type::Boolean, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object/IntType", "IntType", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object/UIntType", "UIntType", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object/DoubleType", "DoubleType", SRI::Type::FloatingPoint, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object/StringType", "StringType", SRI::Type::String, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object", "Object", SRI::Type::Object, SRI::VisitAction::End),

            RegistryEntry("/Test/Array", "Array", SRI::Type::Array, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Array/0", "0", SRI::Type::Null, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array/1", "1", SRI::Type::Boolean, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array/2", "2", SRI::Type::Boolean, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array/3", "3", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array/4", "4", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array/5", "5", SRI::Type::FloatingPoint, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array/6", "6", SRI::Type::String, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array", "Array", SRI::Type::Array, SRI::VisitAction::End),

            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::End)
        };

        Visit(expected, "/Test");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_IterateWithOffsetInObject_AllFieldsInSubSectionAreListed)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Layer1":
                {
                    "Layer2":
                    {
                        "Layer3":
                        {
                            "StringType": "Hello World"
                        }
                    }
                }
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Layer1/Layer2", "Layer2", SRI::Type::Object, SRI::VisitAction::Begin),
            RegistryEntry("/Layer1/Layer2/Layer3", "Layer3", SRI::Type::Object, SRI::VisitAction::Begin),
            RegistryEntry("/Layer1/Layer2/Layer3/StringType", "StringType", SRI::Type::String, SRI::VisitAction::Value),
            RegistryEntry("/Layer1/Layer2/Layer3", "Layer3", SRI::Type::Object, SRI::VisitAction::End),
            RegistryEntry("/Layer1/Layer2", "Layer2", SRI::Type::Object, SRI::VisitAction::End)
        };

        Visit(expected, "/Layer1/Layer2");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_IterateWithOffsetInArray_AllFieldsInSubSectionAreListed)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Object":
                [
                    [
                        [
                            "Hello World"
                        ]
                    ]
                ]
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Object/0/0", "0", SRI::Type::Array, SRI::VisitAction::Begin),
            RegistryEntry("/Object/0/0/0", "0", SRI::Type::String, SRI::VisitAction::Value),
            RegistryEntry("/Object/0/0", "0", SRI::Type::Array, SRI::VisitAction::End)
        };

        Visit(expected, "/Object/0/0");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_SkipObject_FirstAndThirdObjectOnly)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Test":
                {
                    "Object0": { "Field" : 142 },
                    "Object1": { "Field" : 242 },
                    "Object2": { "Field" : 342 }
                }
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::Begin),

            RegistryEntry("/Test/Object0", "Object0", SRI::Type::Object, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Object0/Field", "Field", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object0", "Object0", SRI::Type::Object, SRI::VisitAction::End),

            RegistryEntry("/Test/Object1", "Object1", SRI::Type::Object, SRI::VisitAction::Begin, SRI::VisitResponse::Skip),

            RegistryEntry("/Test/Object2", "Object2", SRI::Type::Object, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Object2/Field", "Field", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object2", "Object2", SRI::Type::Object, SRI::VisitAction::End),

            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::End)
        };

        Visit(expected, "/Test");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_SkipArray_FirstAndThirdObjectOnly)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Test":
                {
                    "Array0": [ 142, 188 ],
                    "Array1": [ 242, 288 ],
                    "Array2": [ 342, 388 ]
                }
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::Begin),

            RegistryEntry("/Test/Array0", "Array0", SRI::Type::Array, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Array0/0", "0", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array0/1", "1", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array0", "Array0", SRI::Type::Array, SRI::VisitAction::End),

            RegistryEntry("/Test/Array1", "Array1", SRI::Type::Array, SRI::VisitAction::Begin, SRI::VisitResponse::Skip),

            RegistryEntry("/Test/Array2", "Array2", SRI::Type::Array, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Array2/0", "0", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array2/1", "1", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array2", "Array2", SRI::Type::Array, SRI::VisitAction::End),

            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::End)
        };

        Visit(expected, "/Test");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_StopMidwayThroughObject_FirstObjectOnly)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Test":
                {
                    "Object0": { "Field" : 142 },
                    "Object1": { "Field" : 242 },
                    "Object2": { "Field" : 342 }
                }
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::Begin),

            RegistryEntry("/Test/Object0", "Object0", SRI::Type::Object, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Object0/Field", "Field", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object0", "Object0", SRI::Type::Object, SRI::VisitAction::End),

            RegistryEntry("/Test/Object1", "Object1", SRI::Type::Object, SRI::VisitAction::Begin, SRI::VisitResponse::Done)
        };

        Visit(expected, "/Test");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_StopOnObjectEnd_FirstObjectOnly)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Test":
                {
                    "Object0": { "Field" : 142 },
                    "Object1": { "Field" : 242 },
                    "Object2": { "Field" : 342 }
                }
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::Begin),

            RegistryEntry("/Test/Object0", "Object0", SRI::Type::Object, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Object0/Field", "Field", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Object0", "Object0", SRI::Type::Object, SRI::VisitAction::End, SRI::VisitResponse::Done)
        };

        Visit(expected, "/Test");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_StopMidwayThroughArray_FirstObjectOnly)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Test":
                {
                    "Array0": [ 142, 188 ],
                    "Array1": [ 242, 288 ],
                    "Array2": [ 342, 388 ]
                }
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::Begin),

            RegistryEntry("/Test/Array0", "Array0", SRI::Type::Array, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Array0/0", "0", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array0/1", "1", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array0", "Array0", SRI::Type::Array, SRI::VisitAction::End),

            RegistryEntry("/Test/Array1", "Array1", SRI::Type::Array, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Array1/0", "0", SRI::Type::Integer, SRI::VisitAction::Value, SRI::VisitResponse::Done),
        };

        Visit(expected, "/Test");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_StopOnArrayEnd_FirstObjectOnly)
    {
        using SRI = AZ::SettingsRegistryInterface;

        ASSERT_TRUE(m_registry->MergeSettings(
            R"({
                "Test":
                {
                    "Array0": [ 142, 188 ],
                    "Array1": [ 242, 288 ],
                    "Array2": [ 342, 388 ]
                }
            })", SRI::Format::JsonMergePatch));

        AZStd::vector<RegistryEntry> expected =
        {
            RegistryEntry("/Test", "Test", SRI::Type::Object, SRI::VisitAction::Begin),

            RegistryEntry("/Test/Array0", "Array0", SRI::Type::Array, SRI::VisitAction::Begin),
            RegistryEntry("/Test/Array0/0", "0", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array0/1", "1", SRI::Type::Integer, SRI::VisitAction::Value),
            RegistryEntry("/Test/Array0", "Array0", SRI::Type::Array, SRI::VisitAction::End, SRI::VisitResponse::Done)
        };

        Visit(expected, "/Test");
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_InvalidPath_ReturnsFalse)
    {
        auto callback = [](const AZ::SettingsRegistryInterface::VisitArgs&,
            AZ::SettingsRegistryInterface::VisitAction)
        {
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        EXPECT_FALSE(m_registry->Visit(callback, "#$%"));
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_UnknownPath_ReturnsFalse)
    {
        auto callback = [](const AZ::SettingsRegistryInterface::VisitArgs&,
            AZ::SettingsRegistryInterface::VisitAction)
        {
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        EXPECT_FALSE(m_registry->Visit(callback, "/Unknown/Path"));
    }

    TEST_F(SettingsRegistryTest, VisitWithCallback_WithEmptyStringViewPath_DoesNotCrash)
    {
        auto callback = [](const AZ::SettingsRegistryInterface::VisitArgs&,
            AZ::SettingsRegistryInterface::VisitAction)
        {
            return AZ::SettingsRegistryInterface::VisitResponse::Continue;
        };

        EXPECT_TRUE(m_registry->Visit(callback, {}));
    }

    //
    // MergeCommandLineArgument
    //

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SetStringArgument_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=Value", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SetStringWithSpaceArgument_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=Value of test", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value of test", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SetIntegerArgument_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=42", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::Integer, m_registry->GetType("/Test"));
        AZ::s64 value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_EQ(42, value);
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SetFloatingPointArgument_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=42.0", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::FloatingPoint, m_registry->GetType("/Test"));
        double value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_EQ(42.0, value);
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SetTrueArgument_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=true", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::Boolean, m_registry->GetType("/Test"));
        bool value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_TRUE(value);
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SetFalseArgument_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=false", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::Boolean, m_registry->GetType("/Test"));
        bool value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_FALSE(value);
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyWithPath_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("/Parent/Test=Value", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Parent/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Parent/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyAndPath_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=Value", "/Parent", {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Parent/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Parent/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyWithDividerAndPath_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("/Test=Value", "/Parent", {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Parent/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Parent/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyAndPathWithDivider_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=Value", "/Parent/", {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Parent/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Parent/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyWithDividerAndPathWithDivider_ValueAddedToRegistry)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("/Test=Value", "/Parent/", {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Parent/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Parent/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SpacesBeforeKey_SpacesAreRemoved)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("   Test=Value", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SpacesAfterKey_SpacesAreRemoved)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test   =Value", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SpacesBeforeAndAfterKey_SpacesAreRemoved)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("   Test   =Value", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SpacesInKey_SpacesAreNotRemoved)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("   Test Key  =Value", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test Key"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test Key"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SpacesBeforeValue_SpacesAreRemoved)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=    Value", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SpacesAfterValue_SpacesAreRemoved)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=Value     ", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_SpacesBeforeAndAfterValue_SpacesAreRemoved)
    {
        ASSERT_TRUE(m_registry->MergeCommandLineArgument("Test=     Value     ", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZStd::string value;
        ASSERT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_STREQ("Value", value.c_str());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyIsTooLong_ReturnsFalse)
    {
        constexpr int LongKeySize = 1024;
        AZStd::string argument = AZStd::string::format("Te%*cst=Value", LongKeySize, ' ');
        EXPECT_FALSE(m_registry->MergeCommandLineArgument(argument, {}, {}));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyIsTooLongWithDivider_ReturnsFalse)
    {
        constexpr int LongKeySize = 1024;
        AZStd::string argument = AZStd::string::format("/Te%*cst=Value", LongKeySize, ' ');
        EXPECT_FALSE(m_registry->MergeCommandLineArgument(argument, "/Path", {}));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_ValueIsTooLong_ReturnsFalse)
    {
        constexpr int LongValueSize = 1024;
        AZStd::string argument = AZStd::string::format("Test=Val%*cue", LongValueSize, ' ');
        EXPECT_FALSE(m_registry->MergeCommandLineArgument(argument, {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::NoType, m_registry->GetType("/Test"));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_MissingValue_ReturnsEmptyString)
    {
        EXPECT_TRUE(m_registry->MergeCommandLineArgument("Test=", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Test"));
        AZ::SettingsRegistryInterface::FixedValueString value;
        EXPECT_TRUE(m_registry->Get(value, "/Test"));
        EXPECT_TRUE(value.empty());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_MissingKey_ReturnsFalse)
    {
        EXPECT_FALSE(m_registry->MergeCommandLineArgument("=Value", {}, {}));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_MissingKeyAndValue_ReturnsFalse)
    {
        EXPECT_FALSE(m_registry->MergeCommandLineArgument("=", {}, {}));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_EmptyString_ReturnsFalse)
    {
        EXPECT_FALSE(m_registry->MergeCommandLineArgument("", {}, {}));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyIsSpaces_ReturnsFalse)
    {
        EXPECT_FALSE(m_registry->MergeCommandLineArgument("    =Value", {}, {}));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_ValueIsSpaces_ReturnsEmptyString)
    {
        EXPECT_TRUE(m_registry->MergeCommandLineArgument("Key=    ", {}, {}));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Key"));
        AZ::SettingsRegistryInterface::FixedValueString value;
        EXPECT_TRUE(m_registry->Get(value, "/Key"));
        EXPECT_TRUE(value.empty());
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_KeyAndValueAreSpaces_ReturnsFalse)
    {
        EXPECT_FALSE(m_registry->MergeCommandLineArgument("    =    ", {}, {}));
    }

    TEST_F(SettingsRegistryTest, MergeCommandLineArgument_OnlySpaces_ReturnsFalse)
    {
        EXPECT_FALSE(m_registry->MergeCommandLineArgument("    ", {}, {}));
    }

    //
    // MergeSettings
    //
    TEST_F(SettingsRegistryTest, MergeSettings_MergeJsonWithAnchorKey_StoresSettingsUnderneathKey)
    {
        constexpr AZStd::string_view anchorKey = "/Anchor/Root/0";
        constexpr auto mergeFormat = AZ::SettingsRegistryInterface::Format::JsonMergePatch;
        EXPECT_TRUE(m_registry->MergeSettings(R"({ "Test": "1" })", mergeFormat, anchorKey));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::Array, m_registry->GetType("/Anchor/Root"));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::Object, m_registry->GetType("/Anchor/Root/0"));
        EXPECT_EQ(AZ::SettingsRegistryInterface::Type::String, m_registry->GetType("/Anchor/Root/0/Test"));
    }

    TEST_F(SettingsRegistryTest, MergeSettings_NotifierSignals_AtAnchorKeyAndStoresMergeType)
    {
        AZStd::string_view anchorKey = "/Anchor/Root";
        bool callbackInvoked{};
        auto callback = [anchorKey, &callbackInvoked](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_EQ(anchorKey, notifyEventArgs.m_jsonKeyPath);
            EXPECT_EQ(AZ::SettingsRegistryInterface::Type::Array, notifyEventArgs.m_type);
            callbackInvoked = true;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);
        constexpr auto mergeFormat = AZ::SettingsRegistryInterface::Format::JsonMergePatch;
        EXPECT_TRUE(m_registry->MergeSettings(R"([ "Test" ])", mergeFormat, anchorKey));
        EXPECT_TRUE(callbackInvoked);
    }

    //
    // MergeSettingsFile
    //

    TEST_F(SettingsRegistryTest, MergeSettingsFile_MergeTestFile_PatchAppliedAndReported)
    {
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", R"({ "Test": 1 })");

        auto callback = [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_TRUE(notifyEventArgs.m_jsonKeyPath.empty());
            AZ::s64 value = -1;
            bool result = m_registry->Get(value, "/Test");
            EXPECT_TRUE(result);
            EXPECT_EQ(1, value);
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);
        auto outcome = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {}, nullptr);
        ASSERT_TRUE(outcome);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_MergeTestFileWithRootKey_PatchAppliedAndReported)
    {
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", R"({ "Test": 1 })");

        auto callback = [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            EXPECT_EQ("/Path", notifyEventArgs.m_jsonKeyPath);
            AZ::s64 value = -1;
            bool result = m_registry->Get(value, "/Path/Test");
            EXPECT_TRUE(result);
            EXPECT_EQ(1, value);
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);
        auto outcome = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "/Path", nullptr);
        ASSERT_TRUE(outcome);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_MergeTestFileWithBuffer_PatchAppliedAndReported)
    {
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", R"({ "Test": 1 })");

        auto result = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
        EXPECT_TRUE(result);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_EmptyPath_ReturnsFalse)
    {
        auto result = m_registry->MergeSettingsFile("", AZ::SettingsRegistryInterface::Format::JsonMergePatch, {}, nullptr);
        EXPECT_FALSE(result);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_PathAsSubString_ReturnsTrue)
    {
        auto path = m_tempDirectory.GetDirectoryAsFixedMaxPath()
            / AZ::SettingsRegistryInterface::RegistryFolder / "test.setreg1234";
        AZ::Test::CreateTestFile(
            m_tempDirectory, AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "test.setreg", R"({ "Test": 1 })");

        AZStd::string_view subPath(path.c_str(), path.Native().size() - 4);
        auto result = m_registry->MergeSettingsFile(subPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {}, nullptr);
        EXPECT_TRUE(result);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_PathAsSubStringThatsTooLong_ReturnsFalse)
    {
        constexpr AZStd::fixed_string<AZ::IO::MaxPathLength + 1> path(AZ::IO::MaxPathLength + 1, '1');
        const AZStd::string_view subPath(path);

        auto result = m_registry->MergeSettingsFile(subPath, AZ::SettingsRegistryInterface::Format::JsonMergePatch, {}, nullptr);
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.GetMessages().empty());
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_InvalidPath_ReturnsFalse)
    {
        auto result = m_registry->MergeSettingsFile("InvalidPath", AZ::SettingsRegistryInterface::Format::JsonMergePatch, {}, nullptr);
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.GetMessages().empty());
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_InvalidRootKey_ReturnsFalse)
    {
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", R"({ "Test": 1 })");

        auto result = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "$", nullptr);
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.GetMessages().empty());
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_ParseError_ReturnsFalse)
    {
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", "{ Test: 1 }");

        auto result = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {}, nullptr);
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.GetMessages().empty());
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_NonObjectRoot_EmptyAnchorKey_JsonMergePatch_ReturnsFalse)
    {
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", R"("BooleanValue": false)");

        auto result = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
        EXPECT_FALSE(result);
        EXPECT_FALSE(result.GetMessages().empty());
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_NonObjectRoot_NonEmptyAnchorKey_JsonMergePatch_ReturnsTrue)
    {
        // Because the root key isn't empty the setting registry will not be be completely overridden and therefore
        // it is safe to merge the .setreg file with a boolean element at the root
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", R"("BooleanValue": false)");

        auto result = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, "/Test");
        EXPECT_TRUE(result);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_EmptyObjectRoot_EmptyAnchorKey_JsonMergePatch_PatchAppliedAndReported)
    {
        // In this scenario the object root is empty. The Merge should complete successfully
        // but leave the settings registry unchanged.
        auto path = AZ::Test::CreateTestFile(m_tempDirectory, "test.setreg", R"({})");

        // Initialize the Settings Registry with an Object at /TestObject/IntValue
        m_registry->MergeSettings(R"({ "TestObject": { "IntValue": 7 } })", AZ::SettingsRegistryInterface::Format::JsonMergePatch);

        // Merging a file with a empty JSON Object should be effectively a no-op.
        // There are some changes in the settings registry to record the merge history for introspection purposes.
        auto outcome = m_registry->MergeSettingsFile(path->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, {});
        EXPECT_TRUE(outcome);

        AZ::s64 intValue{};
        EXPECT_TRUE(m_registry->Get(intValue, "/TestObject/IntValue"));
        EXPECT_EQ(7, intValue);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_JsonWithImport_MergesImportedFiles_InOrder)
    {
        auto filePath = AZ::Test::CreateTestFile(m_tempDirectory, "Memory.phone.setreg", R"({ "pre_field": {"first": 7 },
            "$import": "Memory.os.setreg", "non_override": 7, "post_field": {"2nd": "twenty-one"} })");
        AZ::Test::CreateTestFile(m_tempDirectory, "Memory.os.setreg", R"({ "$import": { "filename": "Memory.mobile.setreg" },
            "non_override2": "Hello World", "override_imported_settings": "override" })");
        AZ::Test::CreateTestFile(m_tempDirectory, "Memory.mobile.setreg", R"({ "Memory": 1, "override_imported_settings": "initial",
            "pre_field": {"first": 14, "second": "fourteen"}, "post_field": [2] })");

        // anchor the settings to scalability
        constexpr AZStd::string_view anchorKey = "/Test/Scalability";
        auto result = m_registry->MergeSettingsFile(filePath->Native(), AZ::SettingsRegistryInterface::Format::JsonMergePatch, anchorKey);
        ASSERT_TRUE(result);

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
        AZ::s64 intValue{};
        EXPECT_TRUE(m_registry->Get(intValue, FixedValueString(anchorKey) + "/non_override"));
        EXPECT_EQ(7, intValue);

        AZStd::string stringValue;
        EXPECT_TRUE(m_registry->Get(stringValue, FixedValueString(anchorKey) + "/non_override2"));
        EXPECT_EQ("Hello World", stringValue);

        // Check the overridden imported settigns
        stringValue.clear();
        EXPECT_TRUE(m_registry->Get(stringValue, FixedValueString(anchorKey) + "/override_imported_settings"));
        // As the Memory.os.setreg imports the Memory.mobile.setreg all of its settings
        // should be merged afterwards and therefore overrides them
        EXPECT_EQ("override", stringValue);

        intValue = {};
        EXPECT_TRUE(m_registry->Get(intValue, FixedValueString(anchorKey) + "/Memory"));
        EXPECT_EQ(1, intValue);

        // Check that the "pre_field" is object "first" field is overridden
        // but the "post_field" overrides the import files value
        // The "Memory.mobile.setreg" would merge over the "Memory.phone.setreg"
        // because its import values appear afterwards
        intValue = {};
        EXPECT_TRUE(m_registry->Get(intValue, FixedValueString(anchorKey) + "/pre_field/first"));
        EXPECT_EQ(14, intValue);

        stringValue.clear();
        EXPECT_TRUE(m_registry->Get(stringValue, FixedValueString(anchorKey) + "/pre_field/second"));
        EXPECT_EQ("fourteen", stringValue);

        stringValue.clear();
        // The "Memory.phone.setreg" "post_field" should retain its value
        // as that field appears after the import
        EXPECT_TRUE(m_registry->Get(stringValue, FixedValueString(anchorKey) + "/post_field/2nd"));
        EXPECT_EQ("twenty-one", stringValue);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFile_JsonWithImport_MultipleImports_MergesBasedOnOrder)
    {
        AZ::Test::CreateTestFile(m_tempDirectory, "number.setreg", R"({ "object_field": { "1": 23,
            "2": 34 }})");
        AZ::Test::CreateTestFile(m_tempDirectory, "string.setreg", R"({ "object_field": { "1": "Hello",
           "3": "World" }})");

        // anchor the settings underneath the Aggregate key
        constexpr AZStd::string_view anchorKey = "/Test/Aggregate";
        {
            // Test by importing the files in order of number.setreg followed by
            // string.setreg. In this scenario, the "object_field/1" from string.setreg
            // should be the value in the Settings Registry
            auto numberStringFilePath = AZ::Test::CreateTestFile(m_tempDirectory, "aggregate.setreg", R"({
                "$import": "number.setreg",
                "$import": "string.setreg" })");
            auto result = m_registry->MergeSettingsFile(numberStringFilePath->Native(),
                AZ::SettingsRegistryInterface::Format::JsonMergePatch, anchorKey);
            ASSERT_TRUE(result);

            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            AZStd::string stringValue;
            EXPECT_TRUE(m_registry->Get(stringValue, FixedValueString(anchorKey) + "/object_field/1"));
            EXPECT_EQ("Hello", stringValue);

            AZ::s64 intValue{};
            EXPECT_TRUE(m_registry->Get(intValue, FixedValueString(anchorKey) + "/object_field/2"));
            EXPECT_EQ(34, intValue);

            stringValue.clear();
            EXPECT_TRUE(m_registry->Get(stringValue, FixedValueString(anchorKey) + "/object_field/3"));
            EXPECT_EQ("World", stringValue);
        }

        {
            // Test by importing the files in order of string.setreg followed by
            // number.setreg. In this scenario, the "object_field/1" from number.setreg
            // should be the value in the Settings Registry
            auto stringNumberFilePath = AZ::Test::CreateTestFile(m_tempDirectory, "aggregate.setreg", R"({
                "$import": "string.setreg",
                "$import": "number.setreg" })");

            auto result = m_registry->MergeSettingsFile(stringNumberFilePath->Native(),
                AZ::SettingsRegistryInterface::Format::JsonMergePatch, anchorKey);
            ASSERT_TRUE(result);

            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;
            AZ::s64 intValue{};
            EXPECT_TRUE(m_registry->Get(intValue, FixedValueString(anchorKey) + "/object_field/1"));
            EXPECT_EQ(23, intValue);

            intValue = {};
            EXPECT_TRUE(m_registry->Get(intValue, FixedValueString(anchorKey) + "/object_field/2"));
            EXPECT_EQ(34, intValue);

            AZStd::string stringValue;
            EXPECT_TRUE(m_registry->Get(stringValue, FixedValueString(anchorKey) + "/object_field/3"));
            EXPECT_EQ("World", stringValue);
        }
    }

    //
    // MergeSettingsFolder
    //

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_MergeTestFiles_FilesAppliedInSpecializationOrder)
    {
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.setreg",
            R"({ "Memory": 0, "MemoryRoot": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.test.setreg",
            R"({ "Memory": 3, "MemoryEditorTest": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.setreg",
            R"({ "Memory": 1, "MemoryEditor": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.test.setreg",
            R"({ "Memory": 2, "MemoryTest": true })");

        size_t counter = 0;
        auto callback = [this, &counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            const char* fileIds[] =
            {
                "/MemoryRoot",
                "/MemoryEditor",
                "/MemoryTest",
                "/MemoryEditorTest"
            };

            MergeNotify(notifyEventArgs.m_jsonKeyPath, counter, AZ_ARRAY_SIZE(fileIds), "/Memory", fileIds);
            counter++;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);

        auto result = m_registry->MergeSettingsFolder((m_tempDirectory.GetDirectoryAsFixedMaxPath() / AZ::SettingsRegistryInterface::RegistryFolder).Native(), {"editor", "test"}, {});
        EXPECT_TRUE(result);
        EXPECT_EQ(4, counter);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_WithPlatformFiles_FilesAppliedInSpecializationOrder)
    {
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.setreg",
            R"({ "Memory": 0, "MemoryRoot": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.test.setreg",
            R"({ "Memory": 5, "MemoryEditorTest": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.setreg",
            R"({ "Memory": 2, "MemoryEditor": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.test.setreg",
            R"({ "Memory": 4, "MemoryTest": true })");

        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Platform/Special/Memory.setreg",
            R"({ "Memory": 1, "MemorySpecial": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Platform/Special/Memory.editor.setreg",
            R"({ "Memory": 3, "MemoryEditorSpecial": true })");

        size_t counter = 0;
        auto callback = [this, &counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            const char* fileIds[] =
            {
                "/MemoryRoot",
                "/MemorySpecial",
                "/MemoryEditor",
                "/MemoryEditorSpecial",
                "/MemoryTest",
                "/MemoryEditorTest"
            };

            MergeNotify(notifyEventArgs.m_jsonKeyPath, counter, AZ_ARRAY_SIZE(fileIds), "/Memory", fileIds);
            counter++;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);

        auto result = m_registry->MergeSettingsFolder((m_tempDirectory.GetDirectoryAsFixedMaxPath() / AZ::SettingsRegistryInterface::RegistryFolder).Native(), { "editor", "test" }, "Special");
        EXPECT_TRUE(result);
        EXPECT_EQ(6, counter);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_DifferentlyNamedFiles_FilesAppliedInAlphabeticAndSpecializationOrder)
    {
        AZ::Test::CreateTestFile(
            m_tempDirectory, AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "c.setreg", R"({ "Id": 2, "c": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "a.editor.test.setreg",
            R"({ "Id": 0, "a": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "b.editor.setreg",
            R"({ "Id": 1, "b": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "c.test.setreg",
            R"({ "Id": 3, "cTest": true })");

        size_t counter = 0;
        auto callback = [this, &counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            const char* fileIds[] =
            {
                "/a",
                "/b",
                "/c",
                "/cTest"
            };

            MergeNotify(notifyEventArgs.m_jsonKeyPath, counter, AZ_ARRAY_SIZE(fileIds), "/Id", fileIds);
            counter++;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);

        auto result = m_registry->MergeSettingsFolder((m_tempDirectory.GetDirectoryAsFixedMaxPath() / AZ::SettingsRegistryInterface::RegistryFolder).Native(), { "editor", "test" }, {});
        EXPECT_TRUE(result);
        EXPECT_EQ(4, counter);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_JsonPatchFiles_FilesAppliedInAlphabeticAndSpecializationOrder)
    {
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.setreg",
            R"({ "Memory": 0, "MemoryRoot": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.test.setreg",
            R"({ "Memory": 3, "MemoryEditorTest": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.setreg",
            R"({ "Memory": 1, "MemoryEditor": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory, AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.test.setregpatch", R"(
            [
                { "op": "replace", "path": "/Memory", "value": 2 },
                { "op": "add", "path": "/MemoryTest", "value": true }
            ])");

        size_t counter = 0;
        auto callback = [this, &counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            const char* fileIds[] =
            {
                "/MemoryRoot",
                "/MemoryEditor",
                "/MemoryTest",
                "/MemoryEditorTest"
            };

            MergeNotify(notifyEventArgs.m_jsonKeyPath, counter, AZ_ARRAY_SIZE(fileIds), "/Memory", fileIds);
            counter++;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);

        auto result = m_registry->MergeSettingsFolder((m_tempDirectory.GetDirectoryAsFixedMaxPath() / AZ::SettingsRegistryInterface::RegistryFolder).Native(), { "editor", "test" }, {});
        EXPECT_TRUE(result);
        EXPECT_EQ(4, counter);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_UnsupportedFilesAreIgnored_OneFileAppliedAndSuccessReturned)
    {
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.setreg",
            R"({ "Memory": 0, "MemoryRoot": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.wrong",
            R"({ "Memory": 1, "Wrong": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Platform/Special/Memory.wrong",
            R"({ "Memory": 2, "SpecialWrong": true })");

        size_t counter = 0;
        auto callback = [this, &counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            const char* fileIds[] =
            {
                "/MemoryRoot"
            };

            MergeNotify(notifyEventArgs.m_jsonKeyPath, counter, AZ_ARRAY_SIZE(fileIds), "/Memory", fileIds);
            counter++;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);

        auto result = m_registry->MergeSettingsFolder((m_tempDirectory.GetDirectoryAsFixedMaxPath() / AZ::SettingsRegistryInterface::RegistryFolder).Native(), { "editor", "test" }, "Special");
        EXPECT_TRUE(result);
        EXPECT_EQ(1, counter);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_WithScratchBuffer_FilesAppliedInSpecializationOrder)
    {
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.setreg",
            R"({ "Memory": 0, "MemoryRoot": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.test.setreg",
            R"({ "Memory": 3, "MemoryEditorTest": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.setreg",
            R"({ "Memory": 1, "MemoryEditor": true })");
        AZ::Test::CreateTestFile(
            m_tempDirectory,
            AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.test.setreg",
            R"({ "Memory": 2, "MemoryTest": true })");

        size_t counter = 0;
        auto callback = [this, &counter](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
        {
            const char* fileIds[] =
            {
                "/MemoryRoot",
                "/MemoryEditor",
                "/MemoryTest",
                "/MemoryEditorTest"
            };

            MergeNotify(notifyEventArgs.m_jsonKeyPath, counter, AZ_ARRAY_SIZE(fileIds), "/Memory", fileIds);
            counter++;
        };
        auto testNotifier1 = m_registry->RegisterNotifier(callback);

        AZStd::vector<char> buffer;

        auto result = m_registry->MergeSettingsFolder((m_tempDirectory.GetDirectoryAsFixedMaxPath() / AZ::SettingsRegistryInterface::RegistryFolder).Native(), { "editor", "test" }, {}, "", &buffer);
        EXPECT_TRUE(result);
        EXPECT_EQ(4, counter);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_EmptyFolder_ReportsSuccessButNothingAdded)
    {
        auto result = m_registry->MergeSettingsFolder(m_tempDirectory.GetDirectoryAsFixedMaxPath().Native(), { "editor", "test" }, {});
        EXPECT_TRUE(result);
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_PathTooLong_ReportsErrorAndReturnsFalse)
    {
        constexpr AZStd::fixed_string<AZ::IO::MaxPathLength + 1> path(AZ::IO::MaxPathLength + 1, 'a');

        auto result = m_registry->MergeSettingsFolder(path, { "editor", "test" }, {});
        EXPECT_FALSE(result);
        // The message structure should contain the error message
        EXPECT_FALSE(result.GetMessages().empty());
    }

    TEST_F(SettingsRegistryTest, MergeSettingsFolder_ConflictingSpecializations_ReportsErrorAndReturnsFalse)
    {
        AZ::Test::CreateTestFile(
            m_tempDirectory, AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.test.editor.setreg", "{}");
        AZ::Test::CreateTestFile(
            m_tempDirectory, AZ::IO::FixedMaxPath(AZ::SettingsRegistryInterface::RegistryFolder) / "Memory.editor.test.setreg", "{}");

        auto result = m_registry->MergeSettingsFolder((m_tempDirectory.GetDirectoryAsFixedMaxPath() / AZ::SettingsRegistryInterface::RegistryFolder).Native(), { "editor", "test" }, {});
        EXPECT_FALSE(result);
        // The message structure should contain the error message
        EXPECT_FALSE(result.GetMessages().empty());
    }
} // namespace SettingsRegistryTests
