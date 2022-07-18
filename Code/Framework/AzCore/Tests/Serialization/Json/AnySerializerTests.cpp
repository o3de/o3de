/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/AnySerializer.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Base.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/AZStdAnyDataContainer.inl>

#include <AzCore/Math/MathMatrixSerializer.h>
#include <AzCore/Math/MathVectorSerializer.h>

 // test the any serializer against several standard serializers, and then
 // compare results of deserialization of types against each other
 // then just do the erroneous input and empty any stuff

namespace JsonSerializationTests
{
    class AnyFixture
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_anySerializer = AZStd::make_unique<AZ::JsonAnySerializer>();
        }

        void TearDown() override
        {
            m_anySerializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        template<typename T>
        void CompareStoredAgainstLoaded(const T& t)
        {
            using namespace AZ::JsonSerializationResult;

            AZStd::any instanceStore;
            instanceStore = t;
            ASSERT_EQ(azrtti_typeid<T>(), instanceStore.type());
            ASSERT_EQ(t, *AZStd::any_cast<T>(&instanceStore));

            ResultCode resultStore = AZ::JsonSerialization::Store(*m_jsonDocument, m_jsonDocument->GetAllocator(), instanceStore, *m_serializationSettings);
            ASSERT_NE(Processing::Halted, resultStore.GetProcessing());

            AZStd::any instanceLoad;
            ResultCode resultLoad = AZ::JsonSerialization::Load(instanceLoad, *m_jsonDocument, *m_deserializationSettings);
            ASSERT_EQ(Outcomes::Success, resultLoad.GetOutcome());

            ASSERT_TRUE(!instanceLoad.empty());
            ASSERT_EQ(instanceStore.type(), instanceLoad.type());
            ASSERT_TRUE(AZStd::any_cast<T>(&instanceLoad) != nullptr);
            EXPECT_EQ(*AZStd::any_cast<T>(&instanceStore), *AZStd::any_cast<T>(&instanceLoad));
        }

    protected:
        AZStd::unique_ptr<AZ::JsonAnySerializer> m_anySerializer;
    };

    TEST_F(AnyFixture, Any2JSON_StoreAndLoad_Bool)
    {
        CompareStoredAgainstLoaded(true);
        CompareStoredAgainstLoaded(false);
    }

    TEST_F(AnyFixture, Any2JSON_StoreAndLoad_Vector3)
    {
        CompareStoredAgainstLoaded(AZ::Vector3(1.23f, 5.711f, 13.17f));
        CompareStoredAgainstLoaded(AZ::Vector3(9.0f, -9.0f, -81.0f));
    }

    TEST_F(AnyFixture, Any2JSON_StoreAndLoad_String)
    {
        CompareStoredAgainstLoaded(AZStd::string(
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
            "This is a string larger than the small object optimizations in AZStd::any or AZStd::string"
        ));
        CompareStoredAgainstLoaded(AZStd::string("tiny string"));
    }


    // write this against the TestCases_Classes thing but store everything in an any
    class AnySerializerTestDescription final
        : public JsonSerializerConformityTestDescriptor<AZStd::any>
    {
    public:
        using Any = AZStd::any;

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonAnySerializer>();
        }

        AZStd::shared_ptr<AZStd::any> CreateDefaultInstance() override
        {
            return AZStd::make_shared<AZStd::any>();
        }

        AZStd::shared_ptr<AZStd::any> CreateFullySetInstance() override
        {
            AZStd::any asV3;
            asV3 = AZ::Vector3(7.0f, 7.0f, 7.0f);
            return AZStd::make_shared<AZStd::any>(asV3);
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "assetId" :
                    {
                        "guid": "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}"
                    }
                })";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                {
                    "assetId" :
                    {
                        "guid": "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}",
                        "subId": 1
                    },
                    "loadBehavior": "PreLoad",
                    "assetHint": "TestFile"
                })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kNullType);
            features.EnableJsonType(rapidjson::kFalseType);
            features.EnableJsonType(rapidjson::kTrueType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.EnableJsonType(rapidjson::kArrayType);
            features.EnableJsonType(rapidjson::kStringType);
            features.EnableJsonType(rapidjson::kNumberType);
            features.m_enableInitializationTest = false;
        }

        bool AreEqual(const AZStd::any& lhs, const AZStd::any& rhs) override
        {
            if (lhs.type() == azrtti_typeid<AZ::Vector3>() && lhs.type() == rhs.type())
            {
                auto lhsValue = AZStd::any_cast<const AZ::Vector3>(&lhs);
                auto rhsValue = AZStd::any_cast<const AZ::Vector3>(&rhs);
                return lhsValue && rhsValue && (*lhsValue == *rhsValue);
            }

            return lhs.empty() == rhs.empty()
                && lhs.type() == rhs.type();
        }

    private:
    };

    using AnyConformityTestTypes = ::testing::Types<AnySerializerTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(Any, JsonSerializerConformityTests, AnyConformityTestTypes);
}
