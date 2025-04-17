/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/JSON/writer.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/std/iterator.h>

namespace JsonSerializationTests
{
    inline void BaseJsonSerializerFixture::SetUp()
    {
        LeakDetectionFixture::SetUp();

        auto reportCallback = [](AZStd::string_view /*message*/, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view /*target*/)->
            AZ::JsonSerializationResult::ResultCode
        {
            return result;
        };

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();

        AddSystemComponentDescriptors(m_systemComponents);
        for (AZ::ComponentDescriptor* component : m_systemComponents)
        {
            component->Reflect(m_serializeContext.get());
            component->Reflect(m_jsonRegistrationContext.get());
        }
        RegisterAdditional(m_serializeContext);
        RegisterAdditional(m_jsonRegistrationContext);

        m_jsonDocument = AZStd::make_unique<rapidjson::Document>();

        m_serializationSettings = AZStd::make_unique<AZ::JsonSerializerSettings>();
        m_serializationSettings->m_serializeContext = m_serializeContext.get();
        m_serializationSettings->m_registrationContext = m_jsonRegistrationContext.get();
        m_serializationSettings->m_reporting = reportCallback;

        m_deserializationSettings = AZStd::make_unique<AZ::JsonDeserializerSettings>();
        m_deserializationSettings->m_serializeContext = m_serializeContext.get();
        m_deserializationSettings->m_registrationContext = m_jsonRegistrationContext.get();
        m_deserializationSettings->m_reporting = reportCallback;

        m_jsonSerializationContext = AZStd::make_unique<AZ::JsonSerializerContext>(*m_serializationSettings, m_jsonDocument->GetAllocator());
        m_jsonDeserializationContext = AZStd::make_unique<AZ::JsonDeserializerContext>(*m_deserializationSettings);
    }

    inline void BaseJsonSerializerFixture::TearDown()
    {
        m_jsonDeserializationContext.reset();
        m_jsonSerializationContext.reset();
        m_deserializationSettings.reset();
        m_serializationSettings.reset();

        m_jsonDocument.reset();

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_serializeContext->EnableRemoveReflection();
        RegisterAdditional(m_jsonRegistrationContext);
        RegisterAdditional(m_serializeContext);
        for (AZ::ComponentDescriptor* descriptor : m_systemComponents)
        {
            descriptor->Reflect(m_serializeContext.get());
            descriptor->Reflect(m_jsonRegistrationContext.get());
            delete descriptor;
        }
        m_serializeContext->DisableRemoveReflection();
        m_jsonRegistrationContext->DisableRemoveReflection();
        m_systemComponents.clear();
        m_systemComponents.shrink_to_fit();

        m_jsonRegistrationContext.reset();
        m_serializeContext.reset();

        LeakDetectionFixture::TearDown();
    }

    inline void BaseJsonSerializerFixture::AddSystemComponentDescriptors(ComponentContainer& systemComponents)
    {
        systemComponents.push_back(AZ::JsonSystemComponent::CreateDescriptor());
    }

    inline void BaseJsonSerializerFixture::RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& /*serializeContext*/)
    {
    }

    inline void BaseJsonSerializerFixture::RegisterAdditional(AZStd::unique_ptr<AZ::JsonRegistrationContext>& /*serializeContext*/)
    {
    }

    inline rapidjson::Value BaseJsonSerializerFixture::CreateExplicitDefault()
    {
        return rapidjson::Value(rapidjson::kObjectType);
    }

    inline void BaseJsonSerializerFixture::Expect_ExplicitDefault(rapidjson::Value& value)
    {
        ASSERT_TRUE(value.IsObject());
        EXPECT_EQ(0, value.MemberCount());
    }

    inline void BaseJsonSerializerFixture::Expect_DocStrEq(AZStd::string_view testString)
    {
        Expect_DocStrEq(testString, true);
    }

    inline void BaseJsonSerializerFixture::Expect_DocStrEq(AZStd::string_view testString, bool stripWhitespace)
    {
        rapidjson::StringBuffer scratchBuffer;
        rapidjson::Writer<decltype(scratchBuffer)> writer(scratchBuffer);
        m_jsonDocument->Accept(writer);

        const char* generatedJson = scratchBuffer.GetString();
        if (stripWhitespace)
        {
            AZStd::string cleanedString = testString;
            cleanedString.erase(
                AZStd::remove_if(cleanedString.begin(), cleanedString.end(), ::isspace), cleanedString.end());

            const char* referenceJson = cleanedString.c_str();
            EXPECT_STRCASEEQ(referenceJson, generatedJson);
        }
        else
        {
            const char* referenceJson = testString.data();
            EXPECT_STRCASEEQ(referenceJson, generatedJson);
        }
    }

    inline void BaseJsonSerializerFixture::Expect_DocStrEq(rapidjson::Value& lhs, rapidjson::Value& rhs)
    {
        rapidjson::StringBuffer lhsScratchBuffer;
        rapidjson::PrettyWriter<decltype(lhsScratchBuffer)> lhsWriter(lhsScratchBuffer);
        lhs.Accept(lhsWriter);

        rapidjson::StringBuffer rhsScratchBuffer;
        rapidjson::PrettyWriter<decltype(rhsScratchBuffer)> rhsWriter(rhsScratchBuffer);
        rhs.Accept(rhsWriter);

        EXPECT_STRCASEEQ(lhsScratchBuffer.GetString(), rhsScratchBuffer.GetString());
    }

    inline void BaseJsonSerializerFixture::ResetJsonContexts()
    {
        m_jsonSerializationContext = AZStd::make_unique<AZ::JsonSerializerContext>(*m_serializationSettings, m_jsonDocument->GetAllocator());
        m_jsonDeserializationContext = AZStd::make_unique<AZ::JsonDeserializerContext>(*m_deserializationSettings);
    }

    inline rapidjson::Value BaseJsonSerializerFixture::TypeToInjectionValue(rapidjson::Type type)
    {
        rapidjson::Value result;
        switch (type)
        {
        case rapidjson::Type::kNullType:
            result.SetNull();
            break;
        case rapidjson::Type::kFalseType:
            result.SetBool(false);
            break;
        case rapidjson::Type::kTrueType:
            result.SetBool(true);
            break;
        case rapidjson::Type::kObjectType:
            result.SetObject();
            break;
        case rapidjson::Type::kArrayType:
            result.SetArray();
            break;
        case rapidjson::Type::kStringType:
            result.SetString("Added to object for testing purposes");
            break;
        case rapidjson::Type::kNumberType:
            result.SetUint(0xbadf00d);
            break;
        default:
            AZ_Assert(false, "Unsupported RapidJSON type: %i.", static_cast<int>(type));
        }
        return result;
    }

    inline void BaseJsonSerializerFixture::InjectAdditionalFields(rapidjson::Value& value, rapidjson::Type typeToInject,
        rapidjson::Document::AllocatorType& allocator)
    {
        if (value.IsObject())
        {
            uint32_t counter = 0;
            rapidjson::Value temp(rapidjson::kObjectType);
            for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it)
            {
                AZStd::string name = AZStd::string::format("Comment %i", counter);
                temp.AddMember(rapidjson::Value(name.c_str(), aznumeric_caster(name.length()), allocator),
                    TypeToInjectionValue(typeToInject), allocator);
                InjectAdditionalFields(it->value, typeToInject, allocator);
                temp.AddMember(AZStd::move(it->name), AZStd::move(it->value), allocator);
                counter++;
            }
            temp.Swap(value);
        }
        else if (value.IsArray())
        {
            rapidjson::Value temp(rapidjson::kArrayType);
            for (auto it = value.Begin(); it != value.End(); ++it)
            {
                temp.PushBack(TypeToInjectionValue(typeToInject), allocator);
                InjectAdditionalFields(*it, typeToInject, allocator);
                temp.PushBack(AZStd::move(*it), allocator);
            }
            temp.Swap(value);
        }
    }

    inline void BaseJsonSerializerFixture::CorruptFields(rapidjson::Value& value, rapidjson::Type typeToInject)
    {
        if (value.IsObject())
        {
            rapidjson::SizeType count = value.MemberCount();
            if (count > 0)
            {
                count /= 2;
                auto memberIt = value.MemberBegin();
                std::advance(memberIt, count);
                CorruptFields(memberIt->value, typeToInject);
            }
        }
        else if (value.IsArray())
        {
            rapidjson::SizeType count = value.Size();
            if (count > 0)
            {
                count /= 2;
                CorruptFields(value[count], typeToInject);
            }
        }
        else
        {
            value = TypeToInjectionValue(typeToInject);
        }
    }
} // namespace JsonSerializationTests
