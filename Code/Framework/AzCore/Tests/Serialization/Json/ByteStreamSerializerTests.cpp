/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/ByteStreamSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class ByteStreamSerializerTestDescription : public JsonSerializerConformityTestDescriptor<AZ::JsonByteStream<AZStd::allocator>>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonByteStreamSerializer<AZStd::allocator>>();
        }

        AZStd::shared_ptr<AZ::JsonByteStream<AZStd::allocator>> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZ::JsonByteStream<AZStd::allocator>>();
        }

        AZStd::shared_ptr<AZ::JsonByteStream<AZStd::allocator>> CreateFullySetInstance() override
        {
            // create a JsonByteStream (AZStd::vector<u8>) with ten 'a's
            return AZStd::make_shared<AZ::JsonByteStream<AZStd::allocator>>(10, 'a');
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            // Base64 encoded version of 'aaaaaaaaaa' (see CreateFullySetInstance)
            return R"("YWFhYWFhYWFhYQ==")";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kStringType);
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const AZ::JsonByteStream<AZStd::allocator>& lhs, const AZ::JsonByteStream<AZStd::allocator>& rhs) override
        {
            return lhs == rhs;
        }
    };

    using ByteStreamConformityTestTypes = ::testing::Types<ByteStreamSerializerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(JsonByteStreamSerialzier, JsonSerializerConformityTests, ByteStreamConformityTestTypes));
} // namespace JsonSerializationTests
