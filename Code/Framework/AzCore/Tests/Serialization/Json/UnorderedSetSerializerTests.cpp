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

#include <AzCore/Serialization/Json/UnorderedSetSerializer.h>
#include <AzCore/std/createdestroy.h>
#include <AzCore/std/iterator.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/containers/unordered_set.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    class UnorderedSetTestDescription final :
        public JsonSerializerConformityTestDescriptor<AZStd::unordered_set<int>>
    {
    public:
        using Set = AZStd::unordered_set<int>;

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonUnorderedSetContainerSerializer>();
        }

        AZStd::shared_ptr<Set> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Set>();
        }

        AZStd::shared_ptr<Set> CreateFullySetInstance() override
        {
            return AZStd::make_shared<Set>(Set{42, -88, 342});
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[-88, 42, 342]";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.m_requiresTypeIdLookups = true;
            features.m_supportsPartialInitialization = false;
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Set>();
        }

        bool AreEqual(const Set& lhs, const Set& rhs) override
        {
            return lhs == rhs;
        }
    };

    class UnorderedMultiSetTestDescription final :
        public JsonSerializerConformityTestDescriptor<AZStd::unordered_multiset<int>>
    {
    public:
        using MultiSet = AZStd::unordered_multiset<int>;

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonUnorderedSetContainerSerializer>();
        }

        AZStd::shared_ptr<MultiSet> CreateDefaultInstance() override
        {
            return AZStd::make_shared<MultiSet>();
        }

        AZStd::shared_ptr<MultiSet> CreateFullySetInstance() override
        {
            return AZStd::make_shared<MultiSet>(MultiSet{ 42, -88, 42, 342 });
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[-88, 42, 42, 342]";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.m_requiresTypeIdLookups = true;
            features.m_supportsPartialInitialization = false;
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<MultiSet>();
        }

        bool AreEqual(const MultiSet& lhs, const MultiSet& rhs) override
        {
            return
                lhs.size() == rhs.size() &&
                lhs.count(-88) == rhs.count(-88) &&
                lhs.count(42) == rhs.count(42) &&
                lhs.count(342) == rhs.count(342);
        }
    };

    using UnorderedSetSerializerConformityTestTypes = ::testing::Types< UnorderedSetTestDescription, UnorderedMultiSetTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(UnorderedSetSerializer, JsonSerializerConformityTests, UnorderedSetSerializerConformityTestTypes);

    class JsonUnorderedSetSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using Set = AZStd::unordered_set<int>;
        
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<AZ::JsonUnorderedSetContainerSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            serializeContext->RegisterGenericType<Set>();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonUnorderedSetContainerSerializer> m_serializer;
    };

    TEST_F(JsonUnorderedSetSerializerTests, Load_DuplicateEntryInSet_CompletesWithDataLeft)
    {
        using namespace AZ::JsonSerializationResult;
        Set instance;

        rapidjson::Value testVal(rapidjson::kArrayType);
        testVal.PushBack(rapidjson::Value(188), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value(288), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value(288), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value(388), m_jsonDocument->GetAllocator());
        ResultCode result = m_serializer->Load(&instance, azrtti_typeid(&instance), testVal, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());
        EXPECT_EQ(Processing::PartialAlter, result.GetProcessing());
        
        EXPECT_NE(instance.end(), instance.find(188));
        EXPECT_NE(instance.end(), instance.find(288));
        EXPECT_NE(instance.end(), instance.find(388));
    }
}
