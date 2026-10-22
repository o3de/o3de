/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/CrcSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class CrcSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<AZ::Crc32>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonCrcSerializer>();
        }

        AZStd::shared_ptr<AZ::Crc32> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::Crc32>();
        }

        AZStd::shared_ptr<AZ::Crc32> CreateFullySetInstance() override
        {
            return AZStd::make_shared<AZ::Crc32>(12345678);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "12345678";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kNumberType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const AZ::Crc32& lhs, const AZ::Crc32& rhs) override
        {
            return lhs == rhs;
        }
    };

    using CrcSerializerConformityTestTypes = ::testing::Types<CrcSerializerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_SUITE_P(JsonCrcSerializer, JsonSerializerConformityTests, CrcSerializerConformityTestTypes));

    class JsonCrcSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_CrcSerializer = AZStd::make_unique<AZ::JsonCrcSerializer>();
        }

        void TearDown() override
        {
            m_CrcSerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        void Load(rapidjson::Value& testVal, const AZ::Crc32& expectedCrc, AZ::JsonSerializationResult::Outcomes expectedOutcome)
        {
            using namespace AZ::JsonSerializationResult;

            AZ::Crc32 testCrc = AZ::Crc32();
            ResultCode result = m_CrcSerializer->Load(&testCrc, azrtti_typeid<AZ::Crc32>(), testVal, *m_jsonDeserializationContext);
            EXPECT_EQ(expectedOutcome, result.GetOutcome());
            if (expectedOutcome == AZ::JsonSerializationResult::Outcomes::Success)
            {
                EXPECT_EQ(testCrc, expectedCrc);
            }
            else
            {
                EXPECT_NE(testCrc, expectedCrc);
            }
        }

    protected:
        AZStd::unique_ptr<AZ::JsonCrcSerializer> m_CrcSerializer;
    };

    TEST_F(JsonCrcSerializerTests, Load_u32Value_ReturnsSuccess)
    {
        rapidjson::Document jsonDocument;
        jsonDocument.SetUint(64);

        Load(jsonDocument.SetUint(64), AZ::Crc32(64), AZ::JsonSerializationResult::Outcomes::Success);
        Load(jsonDocument.SetUint(0xFFFFFFFF), AZ::Crc32(0xFFFFFFFF), AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonCrcSerializerTests, Load_Object_ValidValue_ReturnsSuccess)
    {
        rapidjson::Document jsonDocument;
        jsonDocument.SetObject();
        jsonDocument.AddMember("Value", 0x12345678, jsonDocument.GetAllocator());
        Load(jsonDocument, AZ::Crc32(0x12345678), AZ::JsonSerializationResult::Outcomes::Success);
    }

    TEST_F(JsonCrcSerializerTests, Load_EmptyString_ReturnsUnsupportedAndValueIsUnchanged)
    {
        rapidjson::Document jsonDocument;
        jsonDocument.SetString("");
        Load(jsonDocument, AZ::Crc32(123), AZ::JsonSerializationResult::Outcomes::Unsupported);
    }
} // namespace JsonSerializationTests
