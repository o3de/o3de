/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/Path/PathReflect.h>
#include <AzCore/Serialization/Json/PathSerializer.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    template<typename PathType>
    class PathTestDescription
        : public JsonSerializerConformityTestDescriptor<PathType>
    {
    public:
        using JsonSerializerConformityTestDescriptor<PathType>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            AZ::IO::PathReflect(serializeContext.get());
        }
        void Reflect(AZStd::unique_ptr<AZ::JsonRegistrationContext>& jsonContext) override
        {
            AZ::IO::PathReflect(jsonContext.get());
        }
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonPathSerializer>();
        }

        AZStd::shared_ptr<PathType> CreateDefaultInstance() override
        {
            return AZStd::make_shared<PathType>();
        }

        AZStd::shared_ptr<PathType> CreateFullySetInstance() override
        {
            return AZStd::make_shared<PathType>("O3DE/Relative/Path");
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"("O3DE/Relative/Path")";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kStringType);
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const PathType& lhs, const PathType& rhs) override
        {
            return lhs == rhs;
        }
    };

    using PathConformityTestTypes = ::testing::Types<
        PathTestDescription<AZ::IO::Path>,
        PathTestDescription<AZ::IO::FixedMaxPath>
    >;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_SUITE_P(Path, JsonSerializerConformityTests, PathConformityTestTypes));


    class PathSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        AZStd::unique_ptr<AZ::JsonPathSerializer> m_serializer;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<AZ::JsonPathSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }
    };

    TEST_F(PathSerializerTests, LoadingIntoFixedMaxPath_GreaterThanMaxPathLength_Fails)
    {
        AZ::IO::Path testPath;
        // Fill a path greater than the AZ::IO::MaxPathLength in write it to Json
        testPath.Native().append(AZ::IO::MaxPathLength + 2, 'a');

        rapidjson::Value loadPathValue;
        AZ::JsonSerializationResult::ResultCode resultCode = m_serializer->Store(loadPathValue,
            &testPath, nullptr, azrtti_typeid<AZ::IO::Path>(), *m_jsonSerializationContext);
        EXPECT_EQ(AZ::JsonSerializationResult::Outcomes::Success, resultCode.GetOutcome());

        AZ::IO::FixedMaxPath resultPath;
        AZ::JsonSerializationResult::ResultCode result = m_serializer->Load(&resultPath, azrtti_typeid<AZ::IO::FixedMaxPath>(),
            loadPathValue, *m_jsonDeserializationContext);
        EXPECT_GE(result.GetOutcome(), AZ::JsonSerializationResult::Outcomes::Invalid);
    }
} // namespace JsonSerializationTests
