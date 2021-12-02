/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryScriptUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/variant.h>

namespace SettingsRegistryScriptUtilsTests
{
    static constexpr const char* SettingsRegistryScriptClassName = "SettingsRegistryInterface";


    class SettingsRegistryBehaviorContextFixture
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:

        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            AZ::SettingsRegistryScriptUtils::ReflectSettingsRegistryToBehaviorContext(*m_behaviorContext);
        }

        void TearDown() override
        {
            m_behaviorContext.reset();
            m_registry.reset();
        }

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    };

    TEST_F(SettingsRegistryBehaviorContextFixture, CreateSettingsRegistryImpl_BehaviorClassCreate_Succeeds)
    {
        // Create a Settings Registry Interface Proxy Object
        static constexpr const char* SettingsRegistryCreateMethodName = "SettingsRegistry";

        // Lookup SettingsRegistry() method on the BehaviorContext
        const auto createSettingsRegistryIter = m_behaviorContext->m_methods.find(SettingsRegistryCreateMethodName);
        ASSERT_NE(m_behaviorContext->m_methods.end(), createSettingsRegistryIter);
        AZ::BehaviorMethod* settingsRegistryCreate = createSettingsRegistryIter->second;

        ASSERT_NE(nullptr, settingsRegistryCreate);

        using SettingsRegistryScriptProxy = AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy;
        SettingsRegistryScriptProxy settingsRegistryObject;
        EXPECT_TRUE(settingsRegistryCreate->InvokeResult(settingsRegistryObject));
        EXPECT_NE(nullptr, settingsRegistryObject.m_settingsRegistry);
    }

    TEST_F(SettingsRegistryBehaviorContextFixture, GlobalSettingsRegistry_CanBeQueried_Succeeds)
    {
        constexpr const char* GlobalSettingsRegistryPropertyName = "g_SettingsRegistry";
        auto propIt = m_behaviorContext->m_properties.find(GlobalSettingsRegistryPropertyName);
        ASSERT_NE(m_behaviorContext->m_properties.end(), propIt);
        AZ::BehaviorMethod* globalSettingsRegistryGetter = propIt->second->m_getter;
        ASSERT_NE(nullptr, globalSettingsRegistryGetter);

        // Create a Settings Registry Interface Proxy Object
        const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        AZ::BehaviorClass* settingsRegistryClass = classIter->second;
        ASSERT_NE(nullptr, settingsRegistryClass);
        using SettingsRegistryScriptProxy = AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy;
        SettingsRegistryScriptProxy settingsRegistryObject;

        // Check if the Proxy Object wraps a valid Settings Registry
        // No global settings registry should be registered at this point
        // so the invoking the getter on global Settings Registry should succeed, but return nullptr
        EXPECT_TRUE(globalSettingsRegistryGetter->InvokeResult(settingsRegistryObject));
        EXPECT_EQ(nullptr, settingsRegistryObject.m_settingsRegistry);

        // Register the Settings Registry stored on the fixture with the SettingsRegistry Interface<T>
        AZ::SettingsRegistry::Register(m_registry.get());
        EXPECT_TRUE(globalSettingsRegistryGetter->InvokeResult(settingsRegistryObject));
        EXPECT_NE(nullptr, settingsRegistryObject.m_settingsRegistry);
        AZ::SettingsRegistry::Unregister(m_registry.get());
    }

    TEST_F(SettingsRegistryBehaviorContextFixture, MergeJsonString_UsingBehaviorContext_Succeeds)
    {
        constexpr const char* MergeSettingMethodName = "MergeSettings";
        constexpr AZStd::string_view TestJsonString =
            R"({)" "\n"
            R"(    "TestObject" : {)" "\n"
            R"(        "boolValue" : false,)" "\n"
            R"(        "intValue" : 17,)" "\n"
            R"(        "floatValue" : 32.0,)" "\n"
            R"(        "stringValue" : "Hello World")" "\n"
            R"(    },)" "\n"
            R"(    "TestArray" : [)" "\n"
            R"(        {)" "\n"
            R"(            "intIndex" : 3)" "\n"
            R"(        },)" "\n"
            R"(        {)" "\n"
            R"(            "intIndex" : -55)" "\n"
            R"(        })" "\n"
            R"(    ])" "\n"
            R"(})" "\n";

        const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

        ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
        auto foundIt = settingsRegistryInterfaceClass->m_methods.find(MergeSettingMethodName);
        ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

        AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
        bool mergeSettingsResult{};
        // Pass in both the json data string and the patch format argument
        EXPECT_TRUE(foundIt->second->InvokeResult(mergeSettingsResult, &settingsRegistryObject,
            TestJsonString, AZ::SettingsRegistryInterface::Format::JsonMergePatch));
        EXPECT_TRUE(mergeSettingsResult);

        mergeSettingsResult = {};
        // Pass in only the json data string without the patch format argument which should use the default value
        EXPECT_TRUE(foundIt->second->InvokeResult(mergeSettingsResult, &settingsRegistryObject, TestJsonString));
        EXPECT_TRUE(mergeSettingsResult);

        // Query the Settings Registry for the Merged Content
        {
            // Retrieve
            auto settingsRegistry = m_registry.get();
            bool boolValue = true;
            EXPECT_TRUE(settingsRegistry->Get(boolValue, "/TestObject/boolValue"));
            EXPECT_FALSE(boolValue);

            AZ::s64 intValue{};
            EXPECT_TRUE(settingsRegistry->Get(intValue, "/TestObject/intValue"));
            EXPECT_EQ(17, intValue);

            double floatValue{};
            EXPECT_TRUE(settingsRegistry->Get(floatValue, "/TestObject/floatValue"));
            EXPECT_DOUBLE_EQ(32.0, floatValue);

            AZ::SettingsRegistryInterface::FixedValueString stringValue{};
            EXPECT_TRUE(settingsRegistry->Get(stringValue, "/TestObject/stringValue"));
            EXPECT_STREQ("Hello World", stringValue.c_str());

            intValue = {};
            EXPECT_TRUE(settingsRegistry->Get(intValue, "/TestArray/1/intIndex"));
            EXPECT_EQ(-55, intValue);
        }
    }

    TEST_F(SettingsRegistryBehaviorContextFixture, MergeJsonString_WithJsonPatchFormat__Succeeds)
    {
        constexpr const char* MergeSettingMethodName = "MergeSettings";
        constexpr AZStd::string_view TestJsonString =
            R"([)" "\n"
            R"(    { "op": "add", "path": "/TestObject", "value": {} },)" "\n"
            R"(    { "op": "add", "path": "/TestObject/boolValue", "value": false },)" "\n"
            R"(    { "op": "add", "path": "/TestObject/intValue", "value": 17 },)" "\n"
            R"(    { "op": "add", "path": "/TestObject/floatValue", "value": 32.0 },)" "\n"
            R"(    { "op": "add", "path": "/TestObject/stringValue", "value": "Hello World" },)" "\n"
            R"(    { "op": "add", "path": "/TestArray", "value": [] },)" "\n"
            R"(    { "op": "add", "path": "/TestArray/0", "value": { "intIndex": 3  } },)" "\n"
            R"(    { "op": "add", "path": "/TestArray/1", "value": { "intIndex": -55 } })" "\n"
            R"(])" "\n";

        const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

        ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
        auto foundIt = settingsRegistryInterfaceClass->m_methods.find(MergeSettingMethodName);
        ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

        AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
        bool mergeSettingsResult{};
        // Pass in both the json data string and the patch format argument
        EXPECT_TRUE(foundIt->second->InvokeResult(mergeSettingsResult, &settingsRegistryObject,
            TestJsonString, AZ::SettingsRegistryInterface::Format::JsonPatch));
        EXPECT_TRUE(mergeSettingsResult);

        // Query the Settings Registry for the Merged Content
        {
            // Retrieve
            auto settingsRegistry = m_registry.get();
            bool boolValue = true;
            EXPECT_TRUE(settingsRegistry->Get(boolValue, "/TestObject/boolValue"));
            EXPECT_FALSE(boolValue);

            AZ::s64 intValue{};
            EXPECT_TRUE(settingsRegistry->Get(intValue, "/TestObject/intValue"));
            EXPECT_EQ(17, intValue);

            double floatValue{};
            EXPECT_TRUE(settingsRegistry->Get(floatValue, "/TestObject/floatValue"));
            EXPECT_DOUBLE_EQ(32.0, floatValue);

            AZ::SettingsRegistryInterface::FixedValueString stringValue{};
            EXPECT_TRUE(settingsRegistry->Get(stringValue, "/TestObject/stringValue"));
            EXPECT_STREQ("Hello World", stringValue.c_str());

            intValue = {};
            EXPECT_TRUE(settingsRegistry->Get(intValue, "/TestArray/1/intIndex"));
            EXPECT_EQ(-55, intValue);
        }
    }

    TEST_F(SettingsRegistryBehaviorContextFixture, DumpSettings_FromSettingsRegistry_ResultsInExpectedJsonDocument)
    {
        constexpr const char* DumpSettingMethodName = "DumpSettings";
        constexpr AZStd::string_view ExpectedTestObjectResultString =
            R"({)" "\n"
            R"(    "boolValue": false,)" "\n"
            R"(    "intValue": 17,)" "\n"
            R"(    "floatValue": 32.0,)" "\n"
            R"(    "stringValue": "Hello World")" "\n"
            R"(})";
        constexpr AZStd::string_view ExpectedTestArrayResultString =
            R"([)" "\n"
            R"(    {)" "\n"
            R"(        "intIndex": 3)" "\n"
            R"(    },)" "\n"
            R"(    {)" "\n"
            R"(        "intIndex": -55)" "\n"
            R"(    })" "\n"
            R"(])";

        // Populate the settings registry to match the expected json values
        m_registry->Set("/TestObject/boolValue", false);
        m_registry->Set("/TestObject/intValue", aznumeric_cast<AZ::s64>(17));
        m_registry->Set("/TestObject/floatValue", 32.0);
        m_registry->Set("/TestObject/stringValue", "Hello World");
        m_registry->Set("/TestArray/0/intIndex", aznumeric_cast<AZ::s64>(3));
        m_registry->Set("/TestArray/1/intIndex", aznumeric_cast<AZ::s64>(-55));

        const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
        ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
        AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

        ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
        auto foundIt = settingsRegistryInterfaceClass->m_methods.find(DumpSettingMethodName);
        ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

        // Dump the Settings Registry into an AZStd::string
        AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
        AZStd::optional<AZStd::string> settingsDump{};
        // Dump the TestObject first
        EXPECT_TRUE(foundIt->second->InvokeResult(settingsDump, &settingsRegistryObject, AZStd::string_view{"/TestObject" }));
        EXPECT_TRUE(settingsDump);
        EXPECT_STREQ(ExpectedTestObjectResultString.data(), settingsDump->c_str());


        // Dump the TestArray next
        settingsDump = {};
        EXPECT_TRUE(foundIt->second->InvokeResult(settingsDump, &settingsRegistryObject, AZStd::string_view{ "/TestArray" }));
        EXPECT_TRUE(settingsDump);
        EXPECT_STREQ(ExpectedTestArrayResultString.data(), settingsDump->c_str());

    }

    struct SettingsRegistryBehaviorContextParams
    {
        AZStd::string_view m_jsonPointerPath{ "" };
        AZStd::variant<bool, AZ::s64, double, AZStd::string_view> m_expectedValue;
        const char* m_getMethodName{ "" };
        const char* m_setMethodName{ "" };
    };
    class SettingsRegistryBehaviorContextParamFixture
        : public SettingsRegistryBehaviorContextFixture
        , public ::testing::WithParamInterface<SettingsRegistryBehaviorContextParams>
    {
    };

    TEST_P(SettingsRegistryBehaviorContextParamFixture, GetValue_FromSettingsRegistry_ReturnsSuccess)
    {
        auto&& testParam = GetParam();

        // Set the expected value within the SettingsRegistry
        auto ExpectedValueVisitor = [this, jsonPath = testParam.m_jsonPointerPath, methodName = testParam.m_getMethodName](auto&& value)
        {
            using ValueType = AZStd::remove_cvref_t<decltype(value)>;
            // Set value into the Settings Registry instance so that it can be queried from the BehaviorContext
            EXPECT_TRUE(m_registry->Set(jsonPath, value));

            // Find Reflected SettingsRegistryInterface Get* Method
            const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
            ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
            AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

            ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
            auto foundIt = settingsRegistryInterfaceClass->m_methods.find(methodName);
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

            // Invoke Get* method
            // SettingsRegistryInterface::Get() can store the string result in an AZStd::fixed_string/AZStd::string
            // So the AZStd::string_view is mapped to an AZStd::fixed_string for the purpose of calling Get()
            using GetValueType = AZStd::conditional_t<AZStd::is_same_v<AZStd::string_view, ValueType>,
                AZStd::string, ValueType>;
            AZStd::optional<GetValueType> outputValue{};
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
            EXPECT_TRUE(foundIt->second->InvokeResult(outputValue, &settingsRegistryObject, jsonPath));
            EXPECT_TRUE(outputValue);
            EXPECT_EQ(value, *outputValue);
        };

        AZStd::visit(ExpectedValueVisitor, testParam.m_expectedValue);
    }

    TEST_P(SettingsRegistryBehaviorContextParamFixture, GetValue_FromSettingsRegistry_Fails)
    {
        auto&& testParam = GetParam();

        // Set the expected value within the SettingsRegistry
        auto ExpectedValueVisitor = [this, jsonPath = testParam.m_jsonPointerPath, getMethodName = testParam.m_getMethodName](auto&& value)
        {
            using ValueType = AZStd::remove_cvref_t<decltype(value)>;
            // Set value into the Settings Registry instance so that it can be queried from the BehaviorContext
            EXPECT_TRUE(m_registry->Set(jsonPath, value));

            // Find Reflected SettingsRegistryInterface Get* Method
            const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
            ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
            AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

            ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
            auto foundIt = settingsRegistryInterfaceClass->m_methods.find(getMethodName);
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

            // Invoke Get* method
            // SettingsRegistryInterface::Get() can store the string result in an AZStd::fixed_string/AZStd::string
            // So the AZStd::string_view is mapped to an AZStd::fixed_string for the purpose of calling Get()
            using GetValueType = AZStd::conditional_t<AZStd::is_same_v<AZStd::string_view, ValueType>,
                AZStd::string, ValueType>;
            AZStd::optional<GetValueType> outputValue{};
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
            EXPECT_TRUE(foundIt->second->InvokeResult(outputValue, &settingsRegistryObject, AZStd::string_view{ "InvalidJsonPointerPath" }));
            EXPECT_FALSE(outputValue);
        };

        AZStd::visit(ExpectedValueVisitor, testParam.m_expectedValue);
    }

    TEST_P(SettingsRegistryBehaviorContextParamFixture, SetValue_IntoSettingsRegistry_ReturnsSuccess)
    {
        auto&& testParam = GetParam();

        // Set the expected value within the SettingsRegistry
        auto ExpectedValueVisitor = [this, jsonPath = testParam.m_jsonPointerPath, setMethodName = testParam.m_setMethodName](auto&& value)
        {
            using ValueType = AZStd::remove_cvref_t<decltype(value)>;
            // Find Reflected SettingsRegistryInterface Set* Method
            const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
            ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
            AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

            ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
            auto foundIt = settingsRegistryInterfaceClass->m_methods.find(setMethodName);
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

            // Invoke Set* method
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
            bool setResult{};
            EXPECT_TRUE(foundIt->second->InvokeResult(setResult, &settingsRegistryObject, jsonPath, value));
            EXPECT_TRUE(setResult);

            // Check value set through the BehaviorContext against the Settings Registry instance

            // SettingsRegistryInterface::Get() can store the string result in an AZStd::fixed_string/AZStd::string
            // So the AZStd::string_view is mapped to an AZStd::fixed_string for the purpose of calling Get()
            using GetValueType = AZStd::conditional_t<AZStd::is_same_v<AZStd::string_view, ValueType>,
                AZ::SettingsRegistryInterface::FixedValueString, ValueType>;
            GetValueType outputValue{};
            EXPECT_TRUE(m_registry->Get(outputValue, jsonPath));
            EXPECT_EQ(value, outputValue);
        };

        AZStd::visit(ExpectedValueVisitor, testParam.m_expectedValue);
    }

    TEST_P(SettingsRegistryBehaviorContextParamFixture, SetValue_IntoSettingsRegistry_Fails)
    {
        auto&& testParam = GetParam();

        // Set the expected value within the SettingsRegistry
        auto ExpectedValueVisitor = [this, jsonPath = testParam.m_jsonPointerPath, setMethodName = testParam.m_setMethodName](auto&& value)
        {
            using ValueType = AZStd::remove_cvref_t<decltype(value)>;
            // Find Reflected SettingsRegistryInterface Set* Method
            const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
            ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
            AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

            ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
            auto foundIt = settingsRegistryInterfaceClass->m_methods.find(setMethodName);
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

            // Invoke Set* method
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
            bool setResult = true;
            EXPECT_TRUE(foundIt->second->InvokeResult(setResult, &settingsRegistryObject, AZStd::string_view{ "InvalidJsonPointerPath" }, value));
            EXPECT_FALSE(setResult);

            // Check value set through the BehaviorContext against the Settings Registry instance

            // SettingsRegistryInterface::Get() can store the string result in an AZStd::fixed_string/AZStd::string
            // So the AZStd::string_view is mapped to an AZStd::fixed_string for the purpose of calling Get()
            using GetValueType = AZStd::conditional_t<AZStd::is_same_v<AZStd::string_view, ValueType>,
                AZ::SettingsRegistryInterface::FixedValueString, ValueType>;
            GetValueType outputValue{};
            EXPECT_FALSE(m_registry->Get(outputValue, jsonPath));
            EXPECT_NE(value, outputValue);
        };

        AZStd::visit(ExpectedValueVisitor, testParam.m_expectedValue);
    }

    TEST_P(SettingsRegistryBehaviorContextParamFixture, RemoveKey_FromSettingsRegistry_ReturnsSuccess)
    {
        auto&& testParam = GetParam();

        // Set the expected value within the SettingsRegistry
        auto ExpectedValueVisitor = [this, jsonPath = testParam.m_jsonPointerPath](auto&& value)
        {
            constexpr AZStd::string_view removeMethodName{ "RemoveKey" };
            using ValueType = AZStd::remove_cvref_t<decltype(value)>;
            // Set value with the Settings Registry, so it can checked for removal later
            EXPECT_TRUE(m_registry->Set(jsonPath, value));

            // Find Reflected SettingsRegistryInterface RemoveKey Method
            const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
            ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
            AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;

            ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
            auto foundIt = settingsRegistryInterfaceClass->m_methods.find(removeMethodName);
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

            // Invoke RemoveKey method
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
            bool removeResult{};
            EXPECT_TRUE(foundIt->second->InvokeResult(removeResult, &settingsRegistryObject, jsonPath));
            EXPECT_TRUE(removeResult);

            using GetValueType = AZStd::conditional_t<AZStd::is_same_v<AZStd::string_view, ValueType>,
                AZ::SettingsRegistryInterface::FixedValueString, ValueType>;
            GetValueType outputValue{};
            //The value should have been removed from the Settings Registry instance
            EXPECT_FALSE(m_registry->Get(outputValue, jsonPath));
        };

        AZStd::visit(ExpectedValueVisitor, testParam.m_expectedValue);
    }

    TEST_P(SettingsRegistryBehaviorContextParamFixture, RemoveKey_FromSettingsRegistry_Fails)
    {
        auto&& testParam = GetParam();

        // Set the expected value within the SettingsRegistry
        auto ExpectedValueVisitor = [this, jsonPath = testParam.m_jsonPointerPath](auto&& value)
        {
            constexpr AZStd::string_view removeMethodName{ "RemoveKey" };
            using ValueType = AZStd::remove_cvref_t<decltype(value)>;
            // Set value with the Settings Registry, so it can checked for removal later
            EXPECT_TRUE(m_registry->Set(jsonPath, value));

            // Find Reflected SettingsRegistryInterface RemoveKey Method
            const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
            ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
            AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;
            ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
            auto foundIt = settingsRegistryInterfaceClass->m_methods.find(removeMethodName);
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

            // Invoke RemoveKey method
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());
            bool removeResult = true;
            EXPECT_TRUE(foundIt->second->InvokeResult(removeResult, &settingsRegistryObject, AZStd::string_view{ "InvalidJsonPointerPath" }));
            EXPECT_FALSE(removeResult);

            // Check that value was not removed from the Settings Registry instance
            using GetValueType = AZStd::conditional_t<AZStd::is_same_v<AZStd::string_view, ValueType>,
                AZ::SettingsRegistryInterface::FixedValueString, ValueType>;
            GetValueType outputValue{};
            EXPECT_TRUE(m_registry->Get(outputValue, jsonPath));
            EXPECT_EQ(value, outputValue);
        };

        AZStd::visit(ExpectedValueVisitor, testParam.m_expectedValue);
    }

    TEST_P(SettingsRegistryBehaviorContextParamFixture, GetNotifyEvent_AllowsRegistrationOfAzEventHandler_Succeeds)
    {
        auto&& testParam = GetParam();

        bool updateNotifySent{};

        // Set the expected value within the SettingsRegistry
        auto ExpectedValueVisitor = [this, jsonPath = testParam.m_jsonPointerPath, setMethodName = testParam.m_setMethodName,
            &updateNotifySent](auto&& value)
        {
            using ValueType = AZStd::remove_cvref_t<decltype(value)>;
            const auto classIter = m_behaviorContext->m_classes.find(SettingsRegistryScriptClassName);
            ASSERT_NE(m_behaviorContext->m_classes.end(), classIter);
            AZ::BehaviorClass* settingsRegistryInterfaceClass = classIter->second;
            ASSERT_NE(nullptr, settingsRegistryInterfaceClass);
            // Lookup the SettingsRegistry Proxy GetNotifyEvent
            auto foundIt = settingsRegistryInterfaceClass->m_methods.find("GetNotifyEvent");
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);
            // Create local settings registry proxy object
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy settingsRegistryObject(m_registry.get());

            // Register a notification call back
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy::ScriptNotifyEvent::Handler scriptNotifyHandler(
                [&updateNotifySent, jsonPath](AZStd::string_view path)
            {
                if (path == jsonPath)
                {
                    updateNotifySent = true;
                }
            });
            AZ::SettingsRegistryScriptUtils::Internal::SettingsRegistryScriptProxy::ScriptNotifyEvent* scriptNotifyEvent{};
            EXPECT_TRUE(foundIt->second->InvokeResult(scriptNotifyEvent, &settingsRegistryObject));
            ASSERT_NE(nullptr, scriptNotifyEvent);

            // connect the scriptNotifyHandler to the settings registry script proxy event
            scriptNotifyHandler.Connect(*scriptNotifyEvent);

            // Find Reflected SettingsRegistryInterface Set* Method
            foundIt = settingsRegistryInterfaceClass->m_methods.find(setMethodName);
            ASSERT_NE(settingsRegistryInterfaceClass->m_methods.end(), foundIt);

            // Invoke Set* method
            bool setResult{};
            EXPECT_TRUE(foundIt->second->InvokeResult(setResult, &settingsRegistryObject, jsonPath, value));
            EXPECT_TRUE(setResult);

            // Check value set through the BehaviorContext against the Settings Registry instance

            // SettingsRegistryInterface::Get() can store the string result in an AZStd::fixed_string/AZStd::string
            // So the AZStd::string_view is mapped to an AZStd::fixed_string for the purpose of calling Get()
            using GetValueType = AZStd::conditional_t<AZStd::is_same_v<AZStd::string_view, ValueType>,
                AZ::SettingsRegistryInterface::FixedValueString, ValueType>;
            GetValueType outputValue{};
            EXPECT_TRUE(m_registry->Get(outputValue, jsonPath));
            EXPECT_EQ(value, outputValue);
        };

        AZStd::visit(ExpectedValueVisitor, testParam.m_expectedValue);
        EXPECT_TRUE(updateNotifySent);
    }

    INSTANTIATE_TEST_CASE_P(
        SettingsRegistryBehaviorContextGetFunctions,
        SettingsRegistryBehaviorContextParamFixture,
        testing::Values(
            SettingsRegistryBehaviorContextParams{ "/TestObject/boolValue", true, "GetBool", "SetBool" },
            SettingsRegistryBehaviorContextParams{ "/TestObject/intValue", AZ::s64(17), "GetInt", "SetInt" },
            SettingsRegistryBehaviorContextParams{ "/TestObject/floatValue", 32.0, "GetFloat", "SetFloat" },
            SettingsRegistryBehaviorContextParams{ "/TestObject/stringValue", AZStd::string_view{"Hello World"}, "GetString", "SetString" }
        )
    );
}
