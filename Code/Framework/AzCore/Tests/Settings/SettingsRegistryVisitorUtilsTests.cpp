/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>

namespace SettingsRegistryVisitorUtilsTests
{
    struct VisitCallbackParams
    {
        AZStd::string_view m_inputJsonDocument;

        static inline constexpr size_t MaxFieldCount = 10;
        using ObjectFields = AZStd::fixed_vector<AZStd::pair<AZStd::string_view, AZStd::string_view>, MaxFieldCount>;
        using ArrayFields = AZStd::fixed_vector<AZStd::string_view, MaxFieldCount>;
        ObjectFields m_objectFields;
        ArrayFields m_arrayFields;
    };

    template <typename VisitorParams>
    class SettingsRegistryVisitorUtilsParamFixture
        : public UnitTest::LeakDetectionFixture
        , public ::testing::WithParamInterface<VisitorParams>
    {
    public:

        void SetUp() override
        {
            m_registry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        }

        void TearDown() override
        {
            m_registry.reset();
        }

        AZStd::unique_ptr<AZ::SettingsRegistryImpl> m_registry;
    };

    using SettingsRegistryVisitCallbackFixture = SettingsRegistryVisitorUtilsParamFixture<VisitCallbackParams>;

    TEST_P(SettingsRegistryVisitCallbackFixture, VisitFunction_VisitFieldsOfArrayType_ReturnsFields)
    {
        const VisitCallbackParams& visitParams = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(visitParams.m_inputJsonDocument, AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        AZStd::fixed_vector<AZStd::string, VisitCallbackParams::MaxFieldCount> testArrayFields;
        auto visitorCallback = [this, &testArrayFields](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::string fieldValue;
            EXPECT_TRUE(m_registry->Get(fieldValue, visitArgs.m_jsonKeyPath));
            testArrayFields.emplace_back(AZStd::move(fieldValue));
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitField(*m_registry, visitorCallback, "/Test/Array");

        const AZStd::fixed_vector<AZStd::string, VisitCallbackParams::MaxFieldCount> expectedFields{
            visitParams.m_arrayFields.begin(), visitParams.m_arrayFields.end() };
        EXPECT_THAT(testArrayFields, ::testing::ContainerEq(expectedFields));
    }

    TEST_P(SettingsRegistryVisitCallbackFixture, VisitFunction_VisitFieldsOfObjectType_ReturnsFields)
    {
        const VisitCallbackParams& visitParams = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(visitParams.m_inputJsonDocument, AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        AZStd::fixed_vector<AZStd::pair<AZStd::string, AZStd::string>, VisitCallbackParams::MaxFieldCount> testObjectFields;
        auto visitorCallback = [this, &testObjectFields](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::string fieldValue;
            EXPECT_TRUE(m_registry->Get(fieldValue, visitArgs.m_jsonKeyPath));
            testObjectFields.emplace_back(visitArgs.m_fieldName, AZStd::move(fieldValue));
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitField(*m_registry, visitorCallback, "/Test/Object");

        const AZStd::fixed_vector<AZStd::pair<AZStd::string, AZStd::string>, VisitCallbackParams::MaxFieldCount> expectedFields{
            visitParams.m_objectFields.begin(), visitParams.m_objectFields.end() };
        EXPECT_THAT(testObjectFields, ::testing::ContainerEq(expectedFields));
    }

    TEST_P(SettingsRegistryVisitCallbackFixture, VisitFunction_VisitArrayOfArrayType_ReturnsFields)
    {
        const VisitCallbackParams& visitParams = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(visitParams.m_inputJsonDocument, AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        AZStd::fixed_vector<AZStd::string, VisitCallbackParams::MaxFieldCount> testArrayFields;
        auto visitorCallback = [this, &testArrayFields](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::string fieldValue;
            EXPECT_TRUE(m_registry->Get(fieldValue, visitArgs.m_jsonKeyPath));
            testArrayFields.emplace_back(AZStd::move(fieldValue));
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitArray(*m_registry, visitorCallback, "/Test/Array");

        const AZStd::fixed_vector<AZStd::string, VisitCallbackParams::MaxFieldCount> expectedArrayFields{
            visitParams.m_arrayFields.begin(), visitParams.m_arrayFields.end() };
        EXPECT_THAT(testArrayFields, ::testing::ContainerEq(expectedArrayFields));
    }

    TEST_P(SettingsRegistryVisitCallbackFixture, VisitFunction_VisitArrayOfObjectType_ReturnsEmpty)
    {
        const VisitCallbackParams& visitParams = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(visitParams.m_inputJsonDocument, AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        AZStd::fixed_vector<AZStd::string, VisitCallbackParams::MaxFieldCount> testArrayFields;
        auto visitorCallback = [this, &testArrayFields](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::string fieldValue;
            EXPECT_TRUE(m_registry->Get(fieldValue, visitArgs.m_jsonKeyPath));
            testArrayFields.emplace_back(AZStd::move(fieldValue));
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitArray(*m_registry, visitorCallback, "/Test/Object");

        EXPECT_TRUE(testArrayFields.empty());
    }

    TEST_P(SettingsRegistryVisitCallbackFixture, VisitFunction_VisitObjectOfArrayType_ReturnsEmpty)
    {
        const VisitCallbackParams& visitParams = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(visitParams.m_inputJsonDocument, AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        AZStd::fixed_vector<AZStd::pair<AZStd::string, AZStd::string>, VisitCallbackParams::MaxFieldCount> testObjectFields;
        auto visitorCallback = [this, &testObjectFields](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::string fieldValue;
            EXPECT_TRUE(m_registry->Get(fieldValue, visitArgs.m_jsonKeyPath));
            testObjectFields.emplace_back(visitArgs.m_fieldName, AZStd::move(fieldValue));
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitObject(*m_registry, visitorCallback, "/Test/Array");

        EXPECT_TRUE(testObjectFields.empty());
    }

    TEST_P(SettingsRegistryVisitCallbackFixture, VisitFunction_VisitObjectOfObjectType_ReturnsFields)
    {
        const VisitCallbackParams& visitParams = GetParam();

        ASSERT_TRUE(m_registry->MergeSettings(visitParams.m_inputJsonDocument, AZ::SettingsRegistryInterface::Format::JsonMergePatch));

        AZStd::fixed_vector<AZStd::pair<AZStd::string, AZStd::string>, VisitCallbackParams::MaxFieldCount> testObjectFields;
        auto visitorCallback = [this, &testObjectFields](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
        {
            AZStd::string fieldValue;
            EXPECT_TRUE(m_registry->Get(fieldValue, visitArgs.m_jsonKeyPath));
            testObjectFields.emplace_back(visitArgs.m_fieldName, AZStd::move(fieldValue));
            return AZ::SettingsRegistryInterface::VisitResponse::Skip;
        };

        AZ::SettingsRegistryVisitorUtils::VisitObject(*m_registry, visitorCallback, "/Test/Object");

        const AZStd::fixed_vector<AZStd::pair<AZStd::string, AZStd::string>, VisitCallbackParams::MaxFieldCount> expectedObjectFields{
        visitParams.m_objectFields.begin(), visitParams.m_objectFields.end() };
        EXPECT_THAT(testObjectFields, ::testing::ContainerEq(expectedObjectFields));
    }


    INSTANTIATE_TEST_CASE_P(
        VisitField,
        SettingsRegistryVisitCallbackFixture,
        ::testing::Values(
            VisitCallbackParams
            {
                R"({)" "\n"
                R"(    "Test":)" "\n"
                R"(    {)" "\n"
                R"(        "Array": [ "Hello", "World" ],)" "\n"
                R"(        "Object": { "Foo": "Hello", "Bar": "World"})" "\n"
                R"(    })" "\n"
                R"(})" "\n",
                VisitCallbackParams::ObjectFields{{"Foo", "Hello"}, {"Bar", "World"}},
                VisitCallbackParams::ArrayFields{"Hello", "World"}
            }
        )
    );
}
