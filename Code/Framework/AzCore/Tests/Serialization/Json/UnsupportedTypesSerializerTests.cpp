/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            m_serializer = AZStd::make_unique<Serializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<Serializer> m_serializer;
    };

    using UnsupportedTypesTestTypes = ::testing::Types<AnyInfo, VariantInfo, OptionalInfo>;
    TYPED_TEST_CASE(JsonUnsupportedTypesSerializerTests, UnsupportedTypesTestTypes);

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Load_CallDirectly_ReportsIssueAndHalts)
    {
        using namespace AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, ResultCode result, AZStd::string_view) -> ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        m_jsonDeserializationContext->PushReporter(AZStd::move(callback));

        Type instance{};
        Result result = m_serializer->Load(&instance, azrtti_typeid<Type>(), *m_jsonDocument, *m_jsonDeserializationContext);
        m_jsonDeserializationContext->PopReporter();

        EXPECT_EQ(Processing::Halted, result.GetResultCode().GetProcessing());
        EXPECT_TRUE(hasMessage);
    }

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Load_CallThroughFrontEnd_ReportsIssueAndHalts)
    {
        using namespace AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, ResultCode result, AZStd::string_view) -> ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        m_deserializationSettings->m_reporting = AZStd::move(callback);
        
        Type instance{};
        ResultCode result = AZ::JsonSerialization::Load(instance, *m_jsonDocument, *m_deserializationSettings);
        
        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_TRUE(hasMessage);
    }

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Save_CallDirectly_ReportsIssueAndHalts)
    {
        using namespace AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, ResultCode result, AZStd::string_view) -> ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        m_jsonSerializationContext->PushReporter(AZStd::move(callback));

        Type instance{};
        Result result = m_serializer->Store(*m_jsonDocument, &instance, nullptr, azrtti_typeid<Type>(), *m_jsonSerializationContext);
        m_jsonSerializationContext->PopReporter();

        EXPECT_EQ(Processing::Halted, result.GetResultCode().GetProcessing());
        EXPECT_TRUE(hasMessage);
    }

    TYPED_TEST(JsonUnsupportedTypesSerializerTests, Save_CallThroughFrontEnd_ReportsIssueAndHalts)
    {
        using namespace AZ::JsonSerializationResult;

        bool hasMessage = false;
        auto callback = [&hasMessage](AZStd::string_view message, ResultCode result, AZStd::string_view) -> ResultCode
        {
            hasMessage = !message.empty();
            return result;
        };
        m_serializationSettings->m_reporting = AZStd::move(callback);
        
        Type instance{};
        ResultCode result =
            AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instance, *m_serializationSettings);

        EXPECT_EQ(Processing::Halted, result.GetProcessing());
        EXPECT_TRUE(hasMessage);
    }
} // namespace JsonSerializationTests
