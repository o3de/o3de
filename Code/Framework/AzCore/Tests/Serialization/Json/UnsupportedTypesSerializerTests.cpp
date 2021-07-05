/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/UnsupportedTypesSerializer.h>
#include <AzCore/std/any.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/containers/variant.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    struct AnyInfo
    {
        using Type = AZStd::any;
        using Serializer = AZ::JsonAnySerializer;
    };

    struct VariantInfo
    {
        using Type = AZStd::variant<AZStd::monostate, int, double>;
        using Serializer = AZ::JsonVariantSerializer;
    };

    struct OptionalInfo
    {
        using Type = AZStd::optional<int>;
        using Serializer = AZ::JsonVariantSerializer;
    };

    template<typename Info>
    class JsonUnsupportedTypesSerializerTests : public BaseJsonSerializerFixture
    {
    public:
        using Type = typename Info::Type;
        using Serializer = typename Info::Serializer;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            this->m_serializer = AZStd::make_unique<Serializer>();
        }

        void TearDown() override
        {
            this->m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<Serializer> m_serializer;
        Type m_instance{};
    };

    using UnsupportedTypesTestTypes = ::testing::Types<AnyInfo, VariantInfo, OptionalInfo>;
    TYPED_TEST_CASE(JsonUnsupportedTypesSerializerTests, UnsupportedTypesTestTypes);

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Load_CallDirectly_ReportsIssueAndHalts)
    {
        namespace JSR = AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, JSR::ResultCode result, AZStd::string_view) -> JSR::ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        this->m_jsonDeserializationContext->PushReporter(AZStd::move(callback));

        JSR::Result result = this->m_serializer->Load(
            &this->m_instance, azrtti_typeid(this->m_instance), *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        this->m_jsonDeserializationContext->PopReporter();

        EXPECT_EQ(JSR::Processing::Halted, result.GetResultCode().GetProcessing());
        EXPECT_TRUE(hasMessage);
    }

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Load_CallThroughFrontEnd_ReportsIssueAndHalts)
    {
        namespace JSR = AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, JSR::ResultCode result, AZStd::string_view) -> JSR::ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        this->m_deserializationSettings->m_reporting = AZStd::move(callback);
        
        JSR::ResultCode result = AZ::JsonSerialization::Load(this->m_instance, *this->m_jsonDocument, *this->m_deserializationSettings);
        
        EXPECT_EQ(JSR::Processing::Halted, result.GetProcessing());
        EXPECT_TRUE(hasMessage);
    }

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Save_CallDirectly_ReportsIssueAndHalts)
    {
        namespace JSR = AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, JSR::ResultCode result, AZStd::string_view) -> JSR::ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        this->m_jsonSerializationContext->PushReporter(AZStd::move(callback));

        JSR::Result result = this->m_serializer->Store(
            *this->m_jsonDocument, &this->m_instance, nullptr, azrtti_typeid(this->m_instance), *this->m_jsonSerializationContext);
        this->m_jsonSerializationContext->PopReporter();

        EXPECT_EQ(JSR::Processing::Halted, result.GetResultCode().GetProcessing());
        EXPECT_TRUE(hasMessage);
    }

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Save_CallThroughFrontEnd_ReportsIssueAndHalts)
    {
        namespace JSR = AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, JSR::ResultCode result, AZStd::string_view) -> JSR::ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        this->m_serializationSettings->m_reporting = AZStd::move(callback);
        
        JSR::ResultCode result = AZ::JsonSerialization::Store(
            *this->m_jsonDocument, this->m_jsonDocument->GetAllocator(), this->m_instance, *this->m_serializationSettings);

        EXPECT_EQ(JSR::Processing::Halted, result.GetProcessing());
        EXPECT_TRUE(hasMessage);
    }
} // namespace JsonSerializationTests
