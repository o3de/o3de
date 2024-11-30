/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    using namespace UnitTest;

    AZ_CVAR(bool, testBool, false, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(char, testChar, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int8_t, testInt8, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int16_t, testInt16, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int32_t, testInt32, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(int64_t, testInt64, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint8_t, testUInt8, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint16_t, testUInt16, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint32_t, testUInt32, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(uint64_t, testUInt64, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(float, testFloat, 0, nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(double, testDouble, 0, nullptr, ConsoleFunctorFlags::Null, "");

    AZ_CVAR(AZ::CVarFixedString, testString, "default", nullptr, ConsoleFunctorFlags::Null, "");

    AZ_CVAR(AZ::Vector2, testVec2, AZ::Vector2(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Vector3, testVec3, AZ::Vector3(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Vector4, testVec4, AZ::Vector4(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Quaternion, testQuat, AZ::Quaternion(0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Color, testColorNormalized, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Color, testColorNormalizedMixed, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), nullptr, ConsoleFunctorFlags::Null, "");
    AZ_CVAR(AZ::Color, testColorRgba, AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), nullptr, ConsoleFunctorFlags::Null, "");

    // Creates an enum class with values that aren't consecutive(0, 5, 6)
    AZ_ENUM_CLASS(ConsoleTestEnum,
        Option1,
        (Option2, 5),
        Option3);
    AZ_CVAR(ConsoleTestEnum, testEnum, ConsoleTestEnum::Option1, nullptr, ConsoleFunctorFlags::Null,
        "Supports setting the ConsoleTestEnum via the string or integer argument."
        " Option1/0 sets the CVar to the Option1 value"
        ", Option2/5 sets the CVar to the Option2 value"
        ", Option3/6 sets the CVar to the Option3 value");

    class ConsoleTests
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_console = AZStd::make_unique<AZ::Console>();
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
        }

        void TearDown() override
        {
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console = nullptr;
        }

        void TestClassFunc(const AZ::ConsoleCommandContainer& someStrings)
        {
            m_classFuncArgs = someStrings.size();
        }

        AZ_CONSOLEFUNC(ConsoleTests, TestClassFunc, AZ::ConsoleFunctorFlags::Null, "");
        size_t m_classFuncArgs = 0;
        inline static size_t s_consoleFreeFuncArgs = 0;

        AZStd::unique_ptr<AZ::Console> m_console;

        template <typename _CVAR_TYPE, typename _TYPE>
        void TestCVarHelper(_CVAR_TYPE& cvarInstance, const char* cVarName, const char* setCommand, const char* failCommand, _TYPE testInit, _TYPE initialValue, _TYPE setValue)
        {
            AZ::IConsole* console = m_console.get();

            _TYPE getCVarTest = testInit;

            ConsoleFunctorBase* foundCommand = console->FindCommand(cVarName);
            ASSERT_NE(nullptr, foundCommand); // Find command works and returns a non-null result
            EXPECT_STREQ(cVarName, foundCommand->GetName()); // Find command actually returned the correct command

            EXPECT_EQ(GetValueResult::Success, console->GetCvarValue(cVarName, getCVarTest)); // Console finds and retrieves cvar value
            EXPECT_EQ(initialValue, getCVarTest); // Retrieved cvar value

            console->PerformCommand(setCommand);

            EXPECT_EQ(setValue, _TYPE(cvarInstance)); // Set works for type

            EXPECT_EQ(GetValueResult::Success, console->GetCvarValue(cVarName, getCVarTest)); // Console finds and retrieves cvar value
            EXPECT_EQ(setValue, getCVarTest); // Retrieved cvar value

            if (failCommand != nullptr)
            {
                console->PerformCommand(failCommand);
                EXPECT_EQ(setValue, _TYPE(cvarInstance)); // Failed command did not affect cvar state
            }
        }
    };

    void TestFreeFunc(const AZ::ConsoleCommandContainer& someStrings)
    {
        ConsoleTests::s_consoleFreeFuncArgs = someStrings.size();
    }

    AZ_CONSOLEFREEFUNC(TestFreeFunc, AZ::ConsoleFunctorFlags::Null, "");

    TEST_F(ConsoleTests, CVar_GetSetTest_Bool)
    {
        testBool = false; // Reset testBool to false for scenarios where gtest_repeat is invoked
        TestCVarHelper(testBool, "testBool", "testBool true", "testBool asdf", false, false, true);
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Char)
    {
        testChar = {};
        TestCVarHelper(testChar, "testChar", "testChar 1", nullptr, char(100), char(0), '1'); // Char interprets the input as ascii, so if you print it, you get back a '1'
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int8)
    {
        testInt8 = {};
        TestCVarHelper(testInt8, "testInt8", "testInt8 1", "testInt8 asdf", int8_t(100), int8_t(0), int8_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int16)
    {
        testInt16 = {};
        TestCVarHelper(testInt16, "testInt16", "testInt16 1", "testInt16 asdf", int16_t(100), int16_t(0), int16_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int32)
    {
        testInt32 = {};
        TestCVarHelper(testInt32, "testInt32", "testInt32 1", "testInt32 asdf", int32_t(100), int32_t(0), int32_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Int64)
    {
        testInt64 = {};
        TestCVarHelper(testInt64, "testInt64", "testInt64 1", "testInt64 asdf", int64_t(100), int64_t(0), int64_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt8)
    {
        testUInt8 = {};
        TestCVarHelper(testUInt8, "testUInt8", "testUInt8 1", "testUInt8 asdf", uint8_t(100), uint8_t(0), uint8_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt16)
    {
        testUInt16 = {};
        TestCVarHelper(testUInt16, "testUInt16", "testUInt16 1", "testUInt16 asdf", uint16_t(100), uint16_t(0), uint16_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt32)
    {
        testUInt32 = {};
        TestCVarHelper(testUInt32, "testUInt32", "testUInt32 1", "testUInt32 asdf", uint32_t(100), uint32_t(0), uint32_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_UInt64)
    {
        testUInt64 = {};
        TestCVarHelper(testUInt64, "testUInt64", "testUInt64 1", "testUInt64 asdf", uint64_t(100), uint64_t(0), uint64_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Float)
    {
        testFloat = {};
        TestCVarHelper(testFloat, "testFloat", "testFloat 1", "testFloat asdf", float(100), float(0), float(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Double)
    {
        testDouble = {};
        TestCVarHelper(testDouble, "testDouble", "testDouble 1", "testDouble asdf", double(100), double(0), double(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_String)
    {
        testString = "default";
        // There is really no failure condition for string, since even an empty string simply causes the console to echo the current cvar value
        TestCVarHelper(testString, "testString", "testString notdefault", nullptr, AZ::CVarFixedString("garbage"), AZ::CVarFixedString("default"), AZ::CVarFixedString("notdefault"));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Vector2)
    {
        testVec2 = AZ::Vector2{ 0.0f, 0.0f };
        TestCVarHelper(testVec2, "testVec2", "testVec2 1 1", "testVec2 asdf", AZ::Vector2(100, 100), AZ::Vector2(0, 0), AZ::Vector2(1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Vector3)
    {
        testVec3 = AZ::Vector3{ 0.0f, 0.0f, 0.0f };
        TestCVarHelper(testVec3, "testVec3", "testVec3 1 1 1", "testVec3 asdf", AZ::Vector3(100, 100, 100), AZ::Vector3(0, 0, 0), AZ::Vector3(1, 1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Vector4)
    {
        testVec4 = AZ::Vector4{ 0.0f, 0.0f, 0.0f, 0.0f };
        TestCVarHelper(testVec4, "testVec4", "testVec4 1 1 1 1", "testVec4 asdf", AZ::Vector4(100, 100, 100, 100), AZ::Vector4(0, 0, 0, 0), AZ::Vector4(1, 1, 1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Quaternion)
    {
        testQuat = AZ::Quaternion{ 0.0f, 0.0f, 0.0f, 0.0f };
        TestCVarHelper(testQuat, "testQuat", "testQuat 1 1 1 1", "testQuat asdf", AZ::Quaternion(100, 100, 100, 100), AZ::Quaternion(0, 0, 0, 0), AZ::Quaternion(1, 1, 1, 1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Color_Normalized)
    {
        testColorNormalized = AZ::Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        TestCVarHelper(
            testColorNormalized, "testColorNormalized", "testColorNormalized 1.0 1.0 1.0 1.0", "testColorNormalized asdf",
            AZ::Color(0.5f, 0.5f, 0.5f, 0.5f), AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Color_Normalized_Mixed)
    {
        testColorNormalizedMixed = AZ::Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        TestCVarHelper(
            testColorNormalizedMixed, "testColorNormalizedMixed", "testColorNormalizedMixed 1.0 0 1.0 1", "testColorNormalizedMixed asdf",
            AZ::Color(0.5f, 0.5f, 0.5f, 0.5f), AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), AZ::Color(1.0f, 0.0f, 1.0f, 1.0f));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_Color_Rgba)
    {
        testColorRgba = AZ::Color{ 0.0f, 0.0f, 0.0f, 0.0f };
        TestCVarHelper(
            testColorRgba, "testColorRgba", "testColorRgba 255 255 255 255", "testColorRgba asdf",
            AZ::Color(0.5f, 0.5f, 0.5f, 0.5f), AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_EnumType_SupportsNumericValue)
    {
        testEnum = ConsoleTestEnum::Option1;
        TestCVarHelper(
            testEnum, "testEnum", "testEnum 0", "testEnum RandomOption",
            static_cast<ConsoleTestEnum>(2), ConsoleTestEnum::Option1, ConsoleTestEnum::Option1);

        testEnum = ConsoleTestEnum::Option1;
        TestCVarHelper(
            testEnum, "testEnum", "testEnum 3", "testEnum RandomOption",
            static_cast<ConsoleTestEnum>(2), ConsoleTestEnum::Option1, static_cast<ConsoleTestEnum>(3));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_EnumType_SupportsEnumOptionString)
    {
        testEnum = ConsoleTestEnum::Option1;
        TestCVarHelper(
            testEnum, "testEnum", "testEnum Option2", "testEnum RandomOption",
            static_cast<ConsoleTestEnum>(2), ConsoleTestEnum::Option1, ConsoleTestEnum::Option2);

        testEnum = ConsoleTestEnum::Option1;
        TestCVarHelper(
            testEnum, "testEnum", "testEnum Option3", "testEnum RandomOption",
            static_cast<ConsoleTestEnum>(47), ConsoleTestEnum{}, ConsoleTestEnum::Option3);
    }

    TEST_F(ConsoleTests, CVar_ConstructAfterDeferredInit)
    {
        // This cvar only has scope within the function body, it will be added and removed on function entry and exit
        AZ_CVAR_SCOPED(int32_t, testInit, 0, nullptr, ConsoleFunctorFlags::Null, "");
        testInit = {};
        TestCVarHelper(testInit, "testInit", "testInit 1", "testInit asdf", int32_t(100), int32_t(0), int32_t(1));
    }

    TEST_F(ConsoleTests, CVar_GetSetTest_FormatConversion)
    {
        AZ::IConsole* console = m_console.get();

        // This simply tests format conversion
        float testValue = 0.0f;
        console->PerformCommand("testString 100.5f");
        AZ_TEST_ASSERT(console->GetCvarValue("testString", testValue) == GetValueResult::Success); // Console finds and retrieves cvar value
        AZ_TEST_ASSERT(testValue == 100.5f); // Retrieved cvar value

        console->PerformCommand("testString asdf");
        AZ_TEST_ASSERT(static_cast<AZ::CVarFixedString>(testString) == "asdf"); // String changed state
        AZ_TEST_ASSERT(console->GetCvarValue("testString", testValue) != GetValueResult::Success); // Console can't convert an arbitrary string to a float
    }

    TEST_F(ConsoleTests, CVar_Autocomplete)
    {
        AZ::IConsole* console = m_console.get();

        // Empty input
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("");
            AZ_TEST_ASSERT(completeCommand == "");
        }

        // Prefix
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("te");
            AZ_TEST_ASSERT(completeCommand == "test");
        }

        // Prefix
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("testV");
            AZ_TEST_ASSERT(completeCommand == "testVec");
        }

        // Unique
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("testQ");
            AZ_TEST_ASSERT(completeCommand == "testQuat");
        }

        // Complete
        {
            AZStd::string completeCommand = console->AutoCompleteCommand("testVec3");
            AZ_TEST_ASSERT(completeCommand == "testVec3");
        }

        // Duplicate names
        {
            // Register two cvars with the same name
            auto id = AZ::TypeId();
            auto flag = AZ::ConsoleFunctorFlags::Null;
            auto signature = AZ::ConsoleFunctor<void, false>::FunctorSignature();
            AZ::ConsoleFunctor<void, false> cvarOne(*console, "testAutoCompleteDuplication", "", flag, id, signature);
            AZ::ConsoleFunctor<void, false> cvarTwo(*console, "testAutoCompleteDuplication", "", flag, id, signature);

            // Autocomplete given name expecting one match (not two)
            AZStd::vector<AZStd::string> matches;
            AZStd::string completeCommand = console->AutoCompleteCommand("testAutoCompleteD", &matches);
            AZ_TEST_ASSERT(matches.size() == 1 && completeCommand == "testAutoCompleteDuplication");
        }
    }

    TEST_F(ConsoleTests, ConsoleFunctor_FreeFunctorExecutionTest)
    {
        AZ::IConsole* console = m_console.get();
        ASSERT_TRUE(console);

        // Verify that we can successfully execute a free-standing console functor.
        // The test functor puts the number of arguments into s_consoleFreeFuncArgs.
        s_consoleFreeFuncArgs = 0;
        bool result = static_cast<bool>(console->PerformCommand("TestFreeFunc arg1 arg2"));
        EXPECT_TRUE(result);
        EXPECT_EQ(2, s_consoleFreeFuncArgs);
    }

    TEST_F(ConsoleTests, ConsoleFunctor_ClassFunctorExecutionTest)
    {
        AZ::IConsole* console = m_console.get();
        ASSERT_TRUE(console);

        // Verify that we can successfully execute a class instance console functor.
        // The test functor puts the number of arguments into m_classFuncArgs.
        m_classFuncArgs = 0;
        bool result = static_cast<bool>(console->PerformCommand("ConsoleTests.TestClassFunc arg1 arg2"));
        EXPECT_TRUE(result);
        EXPECT_EQ(2, m_classFuncArgs);
    }

    TEST_F(ConsoleTests, ConsoleFunctor_MultiInstanceClassFunctorExecutionTest)
    {
        // Verify that if multiple instances of a class all register the same class method,
        // the method will get called on every instance of the class.

        class Example
        {
        public:
            void TestClassFunc(const AZ::ConsoleCommandContainer& someStrings)
            {
                m_classFuncArgs = someStrings.size();
            }

            AZ_CONSOLEFUNC(Example, TestClassFunc, AZ::ConsoleFunctorFlags::Null, "");
            size_t m_classFuncArgs = 0;
        };

        constexpr int numInstances = 5;
        Example multiInstances[numInstances];

        AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
        ASSERT_TRUE(console);

        bool result = static_cast<bool>(console->PerformCommand("Example.TestClassFunc arg1 arg2"));
        EXPECT_TRUE(result);
        for (auto& instance : multiInstances)
        {
            EXPECT_EQ(2, instance.m_classFuncArgs);
        }
    }
}


namespace ConsoleSettingsRegistryTests
{
    //! ConfigFile MergeUtils Test
    struct ConfigFileParams
    {
        AZStd::string_view m_testConfigFileName;
        AZStd::string_view m_testConfigContents;
    };
    class ConsoleSettingsRegistryFixture
        : public UnitTest::LeakDetectionFixture
        , public ::testing::WithParamInterface<ConfigFileParams>
    {
    public:
        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            // Store off the old global settings registry to restore after each test
            m_oldSettingsRegistry = AZ::SettingsRegistry::Get();
            if (m_oldSettingsRegistry != nullptr)
            {
                AZ::SettingsRegistry::Unregister(m_oldSettingsRegistry);
            }
            AZ::SettingsRegistry::Register(m_registry.get());

            // Create a TestFile in the Test Directory
            auto configFileParams = GetParam();
            AZ::Test::CreateTestFile(m_tempDirectory, configFileParams.m_testConfigFileName, configFileParams.m_testConfigContents);
        }

        void TearDown() override
        {
            // Restore the old global settings registry
            AZ::SettingsRegistry::Unregister(m_registry.get());
            if (m_oldSettingsRegistry != nullptr)
            {
                AZ::SettingsRegistry::Register(m_oldSettingsRegistry);
                m_oldSettingsRegistry = {};
            }
            m_registry.reset();
        }

        void TestClassFunc(const AZ::ConsoleCommandContainer& someStrings)
        {
            m_stringArgCount = someStrings.size();
        }

        AZ_CONSOLEFUNC(ConsoleSettingsRegistryFixture, TestClassFunc, AZ::ConsoleFunctorFlags::Null, "");

    protected:
        size_t m_stringArgCount{};
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_registry;
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;

    private:
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry{};
    };

    static bool s_consoleFreeFunctionInvoked = false;
    static void TestSettingsRegistryFreeFunc(const AZ::ConsoleCommandContainer& someStrings)
    {
        EXPECT_TRUE(someStrings.empty());
        s_consoleFreeFunctionInvoked = true;
    }

    AZ_CONSOLEFREEFUNC(TestSettingsRegistryFreeFunc, AZ::ConsoleFunctorFlags::Null, "");

    TEST_P(ConsoleSettingsRegistryFixture, Console_AbleToLoadSettingsFile_Successfully)
    {
        AZ::Console testConsole(*m_registry);
        testConsole.LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
        AZ::Interface<AZ::IConsole>::Register(&testConsole);
        AZ_CVAR_SCOPED(int32_t, testInit, 0, nullptr, AZ::ConsoleFunctorFlags::Null, "");
        s_consoleFreeFunctionInvoked = false;
        testInit = {};
        AZ::testChar = {};
        AZ::testBool = {};
        AZ::testInt8 = {};
        AZ::testInt16 = {};
        AZ::testInt32 = {};
        AZ::testInt64 = {};
        AZ::testUInt8 = {};
        AZ::testUInt16 = {};
        AZ::testUInt32 = {};
        AZ::testUInt64 = {};
        AZ::testFloat= {};
        AZ::testDouble = {};
        AZ::testString = {};

        auto configFileParams = GetParam();
        auto testFilePath = m_tempDirectory.GetDirectoryAsFixedMaxPath() / configFileParams.m_testConfigFileName;
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(testFilePath.c_str()));
        testConsole.ExecuteConfigFile(testFilePath.Native());
        EXPECT_TRUE(s_consoleFreeFunctionInvoked);
        EXPECT_EQ(3, testInit);
        EXPECT_TRUE(static_cast<bool>(AZ::testBool));
        EXPECT_EQ('Q', AZ::testChar);
        EXPECT_EQ(24, AZ::testInt8);
        EXPECT_EQ(-32, AZ::testInt16);
        EXPECT_EQ(41, AZ::testInt32);
        EXPECT_EQ(-51, AZ::testInt64);
        EXPECT_EQ(3, AZ::testUInt8);
        EXPECT_EQ(5, AZ::testUInt16);
        EXPECT_EQ(6, AZ::testUInt32);
        EXPECT_EQ(0xFFFF'FFFF'FFFF'FFFF, AZ::testUInt64);
        EXPECT_FLOAT_EQ(1.0f, AZ::testFloat);
        EXPECT_DOUBLE_EQ(2, AZ::testDouble);
        EXPECT_STREQ("Stable", static_cast<AZ::CVarFixedString>(AZ::testString).c_str());
        EXPECT_EQ(3, m_stringArgCount);
        AZ::Interface<AZ::IConsole>::Unregister(&testConsole);
    }

    template<typename T>
    using ConsoleDataWrapper = AZ::ConsoleDataWrapper<T, ConsoleThreadSafety<T>>;
    TEST_P(ConsoleSettingsRegistryFixture, Console_RecordsUnregisteredCommands_And_IsAbleToDeferDispatchCommand_Successfully)
    {
        AZ::Console testConsole(*m_registry);
        AZ::Interface<AZ::IConsole>::Register(&testConsole);
        // GetDeferredHead is invoked for the side effect of to set the s_deferredHeadInvoked value to true
        // This allows scoped console variables to be attached immediately
        [[maybe_unused]] auto deferredHead = AZ::ConsoleFunctorBase::GetDeferredHead();


        ConsoleDataWrapper<int32_t> localTestInit{ {}, nullptr, "testInit", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<char> localTestChar{ {}, nullptr, "testChar", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<bool> localTestBool{ {}, nullptr, "testBool", "", AZ::ConsoleFunctorFlags::Null };

        s_consoleFreeFunctionInvoked = false;

        // Invoke the Commands for Scoped CVar variables above
        auto configFileParams = GetParam();
        auto testFilePath = m_tempDirectory.GetDirectoryAsFixedMaxPath() / configFileParams.m_testConfigFileName;
        EXPECT_TRUE(AZ::IO::SystemFile::Exists(testFilePath.c_str()));
        testConsole.ExecuteConfigFile(testFilePath.Native());

        EXPECT_EQ(3, localTestInit);
        EXPECT_TRUE(static_cast<bool>(localTestBool));
        EXPECT_EQ('Q', localTestChar);

        // The following commands from the config files should have been deferred
        ConsoleDataWrapper<int8_t> localTestInt8{ {}, nullptr, "testInt8", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<int16_t> localTestInt16{ {}, nullptr, "testInt16", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<int32_t> localTestInt32{ {}, nullptr, "testInt32", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<int64_t> localTestInt64{ {}, nullptr, "testInt64", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<uint8_t> localTestUInt8{ {}, nullptr, "testUInt8", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<uint16_t> localTestUInt16{ {}, nullptr, "testUInt16", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<uint32_t> localTestUInt32{ {}, nullptr, "testUInt32", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<uint64_t> localTestUInt64{ {}, nullptr, "testUInt64", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<float> localTestFloat{ {}, nullptr, "testFloat", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<double> localTestDouble{ {}, nullptr, "testDouble", "", AZ::ConsoleFunctorFlags::Null };
        ConsoleDataWrapper<AZ::CVarFixedString> localTestString{ {}, nullptr, "testString", "", AZ::ConsoleFunctorFlags::Null };


        // The scoped cvars just above should have all been deferred for execution
        // Each of them should have executed resulting in the expected return value
        EXPECT_TRUE(testConsole.ExecuteDeferredConsoleCommands());

        EXPECT_EQ(24, localTestInt8);
        EXPECT_EQ(-32, localTestInt16);
        EXPECT_EQ(41, localTestInt32);
        EXPECT_EQ(-51, localTestInt64);
        EXPECT_EQ(3, localTestUInt8);
        EXPECT_EQ(5, localTestUInt16);
        EXPECT_EQ(6, localTestUInt32);
        EXPECT_EQ(0xFFFF'FFFF'FFFF'FFFF, localTestUInt64);
        EXPECT_FLOAT_EQ(1.0f, localTestFloat);
        EXPECT_DOUBLE_EQ(2, localTestDouble);
        EXPECT_STREQ("Stable", static_cast<AZ::CVarFixedString>(localTestString).c_str());

        // All of the deferred console commands should have executed at this point
        // Therefore this invocation should return false
        EXPECT_FALSE(testConsole.ExecuteDeferredConsoleCommands());

        AZ::Interface<AZ::IConsole>::Unregister(&testConsole);
    }


    static constexpr AZStd::string_view UserINIStyleContent =
        R"(
            testInit = 3
            testBool true
            testChar Q
            testInt8 24
            testInt16 -32
            testInt32 41
            testInt64 -51
            testUInt8 3
            testUInt16 5
            testUInt32 6
            testUInt64 18446744073709551615
            testFloat 1.0
            testDouble 2
            testString Stable
            ConsoleSettingsRegistryFixture.testClassFunc Foo Bar Baz
            TestSettingsRegistryFreeFunc
        )";

    static constexpr AZStd::string_view UserJsonMergePatchContent =
        R"(
            {
                "Amazon": {
                    "AzCore": {
                        "Runtime": {
                            "ConsoleCommands": {
                                "testInit": 3,
                                "testBool": true,
                                "testChar": "Q",
                                "testInt8": 24,
                                "testInt16": -32,
                                "testInt32": 41,
                                "testInt64": -51,
                                "testUInt8": 3,
                                "testUInt16": 5,
                                "testUInt32": 6,
                                "testUInt64": 18446744073709551615,
                                "testFloat": 1.0,
                                "testDouble": 2,
                                "testString": "Stable",
                                "ConsoleSettingsRegistryFixture.testClassFunc": "Foo Bar Baz",
                                "TestSettingsRegistryFreeFunc": ""
                            }
                        }
                    }
                }
            }
        )";
    static constexpr AZStd::string_view UserJsonPatchContent =
        R"(
            [
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testInit", "value": 3 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testBool", "value": true },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testChar", "value": "Q" },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testInt8", "value": 24 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testInt16", "value": -32 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testInt32", "value": 41 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testInt64", "value": -51 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testUInt8", "value": 3 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testUInt16", "value": 5 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testUInt32", "value": 6 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testUInt64", "value": 18446744073709551615 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testFloat", "value": 1.0 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testDouble", "value": 2 },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/testString", "value": "Stable" },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/ConsoleSettingsRegistryFixture.testClassFunc", "value": "Foo Bar Baz" },
                { "op": "add", "path": "/Amazon/AzCore/Runtime/ConsoleCommands/TestSettingsRegistryFreeFunc", "value": "" }
            ]
        )";

    INSTANTIATE_TEST_CASE_P(
        ExecuteCommandFromSettingsFile,
        ConsoleSettingsRegistryFixture,
        ::testing::Values(
            ConfigFileParams{"user.cfg", UserINIStyleContent},
            ConfigFileParams{"user.setreg", UserJsonMergePatchContent},
            ConfigFileParams{"user.setregpatch", UserJsonPatchContent}
        )
    );
}
