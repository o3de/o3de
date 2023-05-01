/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/JSON/document.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Settings/ConfigurableStack.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace UnitTest
{
    struct ConfigInt
    {
        AZ_TYPE_INFO(UnitTest::ConfigInt, "{1FAF6E55-7FA4-4FFA-8C41-34F422B8E8AB}");
        AZ_CLASS_ALLOCATOR(ConfigInt, AZ::SystemAllocator);

        int m_value;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto sc = azrtti_cast<AZ::SerializeContext*>(context))
            {
                sc->Class<ConfigInt>()->Field("Value", &ConfigInt::m_value);
            }
        }
    };

    struct ConfigurableStackTests : public LeakDetectionFixture
    {
        void Reflect(AZ::ReflectContext* context)
        {
            if (auto sc = azrtti_cast<AZ::SerializeContext*>(context))
            {
                AZ::JsonSystemComponent::Reflect(sc);
                ConfigInt::Reflect(sc);
                sc->RegisterGenericType<AZ::ConfigurableStack<ConfigInt>>();  
            }
            else if (auto jrc = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                AZ::JsonSystemComponent::Reflect(jrc);
            }
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

            Reflect(m_serializeContext.get());
            Reflect(m_jsonRegistrationContext.get());
            
            m_deserializationSettings.m_registrationContext = m_jsonRegistrationContext.get();
            m_deserializationSettings.m_serializeContext = m_serializeContext.get();
        }

        void TearDown()
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            Reflect(m_jsonRegistrationContext.get());
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_serializeContext->EnableRemoveReflection();
            Reflect(m_serializeContext.get());
            m_serializeContext->DisableRemoveReflection();

            LeakDetectionFixture::TearDown();
        }

        void ObjectTest(AZStd::string_view jsonText)
        {
            AZ::ConfigurableStack<ConfigInt> stack;

            rapidjson::Document document;
            document.Parse(jsonText.data(), jsonText.length());
            ASSERT_FALSE(document.HasParseError());
            AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(stack, document, m_deserializationSettings);
            ASSERT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
            ASSERT_EQ(4, stack.size());

            int numberCounter = 0;
            int valueCounter = 42;
            for (auto& [name, value] : stack)
            {
                EXPECT_STREQ(AZStd::string::format("Value%i", numberCounter).c_str(), name.c_str());
                EXPECT_EQ(valueCounter, value->m_value);
                numberCounter++;
                valueCounter++;
            }
        }

        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_jsonRegistrationContext;
        AZ::JsonDeserializerSettings m_deserializationSettings;
    };

    TEST_F(ConfigurableStackTests, DeserializeArray)
    {
        AZ::ConfigurableStack<ConfigInt> stack;

        rapidjson::Document document;
        document.Parse(
            R"([
                    { "Value": 42 },
                    { "Value": 43 },
                    { "Value": 44 },
                    { "Value": 45 }
                ])");
        ASSERT_FALSE(document.HasParseError());
        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(stack, document, m_deserializationSettings);
        ASSERT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        ASSERT_EQ(4, stack.size());

        int numberCounter = 0;
        int valueCounter = 42;
        for (auto& [name, value] : stack)
        {
            EXPECT_STREQ(AZStd::to_string(numberCounter).c_str(), name.c_str());
            EXPECT_EQ(valueCounter, value->m_value);
            numberCounter++;
            valueCounter++;
        }
    }

    TEST_F(ConfigurableStackTests, DeserializeObject)
    {
        ObjectTest(
            R"({
                    "Value0": { "Value": 42 },
                    "Value1": { "Value": 43 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 }
                })");
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithLateBefore)
    {
        ObjectTest(
            R"({
                    "Value0": { "Value": 42 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 },
                    "Value1":
                    {
                        "$stack_before": "Value2",
                        "Value": 43
                    }
                })");
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithEarlyBefore)
    {
        ObjectTest(
            R"({
                    "Value1":
                    {
                        "$stack_before": "Value2",
                        "Value": 43
                    },
                    "Value0": { "Value": 42 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 }
                })");
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithLateAfter)
    {
        ObjectTest(
            R"({
                    "Value0": { "Value": 42 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 },
                    "Value1":
                    {
                        "$stack_after": "Value0",
                        "Value": 43
                    }
                })");
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithEarlyAfter)
    {
        ObjectTest(
            R"({
                    "Value1":
                    {
                        "$stack_after": "Value0",
                        "Value": 43
                    },
                    "Value0": { "Value": 42 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 }
                })");
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithBeforeFirst)
    {
        ObjectTest(
            R"({
                    "Value1": { "Value": 43 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 },
                    "Value0":
                    {
                        "$stack_before": "Value1",
                        "Value": 42
                    }
                    
                })");
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithInsertAfterLast)
    {
        ObjectTest(
            R"({
                    "Value3":
                    {
                        "$stack_after": "Value2",
                        "Value": 45
                    },
                    "Value0": { "Value": 42 },
                    "Value1": { "Value": 43 },
                    "Value2": { "Value": 44 }
                })");
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithInvalidTarget)
    {
        AZ::ConfigurableStack<ConfigInt> stack;

        rapidjson::Document document;
        document.Parse(
            R"({
                    "Value1":
                    {
                        "$stack_after": "airplane",
                        "Value": 43
                    },
                    "Value0": { "Value": 42 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 }
                })");
        ASSERT_FALSE(document.HasParseError());
        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(stack, document, m_deserializationSettings);
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialSkip, result.GetOutcome());
        EXPECT_EQ(3, stack.size());
    }

    TEST_F(ConfigurableStackTests, DeserializeObjectWithInvalidTargetType)
    {
        AZ::ConfigurableStack<ConfigInt> stack;

        rapidjson::Document document;
        document.Parse(
            R"({
                    "Value1":
                    {
                        "$stack_after": 42,
                        "Value": 43
                    },
                    "Value0": { "Value": 42 },
                    "Value2": { "Value": 44 },
                    "Value3": { "Value": 45 }
                })");
        ASSERT_FALSE(document.HasParseError());
        AZ::JsonSerializationResult::ResultCode result = AZ::JsonSerialization::Load(stack, document, m_deserializationSettings);
        EXPECT_EQ(AZ::JsonSerializationResult::Processing::Completed, result.GetProcessing());
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::PartialSkip, result.GetOutcome());
        EXPECT_EQ(3, stack.size());
    }
} // namespace UnitTest
