/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/BasicContainerSerializer.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/iterator.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    template<typename Container>
    class BasicContainerConformityTestDescriptor :
        public JsonSerializerConformityTestDescriptor<Container>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonBasicContainerSerializer>();
        }

        AZStd::shared_ptr<Container> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Container>();
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.m_supportsPartialInitialization = false;
            features.m_requiresTypeIdLookups = true;
        }
    };

    template<typename Container>
    class SimpleTestDescription final :
        public BasicContainerConformityTestDescriptor<Container>
    {
    public: 
        AZStd::shared_ptr<Container> CreateFullySetInstance() override
        {
            return AZStd::make_shared<Container>(Container{ 188, 288, 388 });
        }

        AZStd::shared_ptr<Container> CreateSingleArrayDefaultInstance() override
        {
            return AZStd::make_shared<Container>(Container{ 0 });
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[188, 288, 388]";
        }

        using BasicContainerConformityTestDescriptor<Container>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Container>();
        }

        bool AreEqual(const Container& lhs, const Container& rhs) override
        {
            return lhs == rhs;
        }
    };

    struct SimplePointerTestDescriptionCompare
    {
        AZ_TYPE_INFO(SimplePointerTestDescriptionCompare, "{115495B5-2572-4DC2-A102-6B59BC85B974}");
        bool operator()(const int* lhs, const int* rhs)
        {
            return *lhs == *rhs;
        };
    };
    struct SimplePointerTestDescriptionLess
    {
        AZ_TYPE_INFO(SimplePointerTestDescriptionLess, "{2C883A26-A2C6-481D-8F10-96E619FB56EE}");
        bool operator()(const int* lhs, const int* rhs)
        {
            return *lhs < *rhs;
        };
    };

    template<typename Container>
    class SimplePointerTestDescription final :
        public BasicContainerConformityTestDescriptor<Container>
    {
    public:
        static void Delete(Container* container)
        {
            for (auto* entry : *container)
            {
                azfree(entry);
            }
            delete container;
        }

        AZStd::shared_ptr<Container> CreateDefaultInstance() override
        {
            return AZStd::shared_ptr<Container>(new Container{}, &SimplePointerTestDescription::Delete);
        }

        AZStd::shared_ptr<Container> CreateFullySetInstance() override
        {
            int* value0 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
            int* value1 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
            int* value2 = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));

            *value0 = 188;
            *value1 = 288;
            *value2 = 388;

            return AZStd::shared_ptr<Container>(new Container{ value0, value1, value2 },
                &SimplePointerTestDescription::Delete);
        }

        AZStd::shared_ptr<Container> CreateSingleArrayDefaultInstance() override
        {
            int* value = reinterpret_cast<int*>(azmalloc(sizeof(int), alignof(int)));
            *value = 0;
            return AZStd::shared_ptr<Container>(new Container{ value }, &SimplePointerTestDescription::Delete);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[188, 288, 388]";
        }

        using BasicContainerConformityTestDescriptor<Container>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Container>();
        }

        bool AreEqual(const Container& lhs, const Container& rhs) override
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }
            return AZStd::equal(lhs.begin(), lhs.end(), rhs.begin(), SimplePointerTestDescriptionCompare{});
        }
    };

    template<typename Container>
    class ComplextTestDescription final :
        public BasicContainerConformityTestDescriptor<Container>
    {
    public:
        AZStd::shared_ptr<Container> CreateFullySetInstance() override
        {
            SimpleClass values[3];
            values[0].m_var1 = 188;
            values[0].m_var2 = 288.0;

            values[1].m_var1 = 388;
            values[1].m_var2 = 488.0;

            values[2].m_var1 = 588;
            values[2].m_var2 = 688.0;

            auto instance = AZStd::make_shared<Container>();
            *instance = { values[0], values[1], values[2] };
            return instance;
        }

        AZStd::shared_ptr<Container> CreatePartialDefaultInstance() override
        {
            SimpleClass values[3];
            // These defaults are specifically chosen to make sure the elements
            // are kept in the same order in AZStd::set.
            values[0].m_var1 = 10;
            values[2].m_var2 = 688.0;

            auto instance = AZStd::make_shared<Container>();
            *instance = { values[0], values[1], values[2] };
            return instance;
        }

        AZStd::shared_ptr<Container> CreateSingleArrayDefaultInstance() override
        {
            auto instance = AZStd::make_shared<Container>();
            *instance = { SimpleClass{} };
            return instance;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"([
                {
                    "var1": 188,
                    "var2": 288.0
                }, 
                {
                    "var1": 388,
                    "var2": 488.0
                }, 
                {
                    "var1": 588,
                    "var2": 688.0
                }])";
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"([
                {
                    "var1": 10
                }, 
                {}, 
                {
                    "var2": 688.0
                }])";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            BasicContainerConformityTestDescriptor<Container>::ConfigureFeatures(features);
            features.m_supportsPartialInitialization = true;
        }

        using BasicContainerConformityTestDescriptor<Container>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            SimpleClass::Reflect(context, true);
            context->RegisterGenericType<Container>();
        }

        bool AreEqual(const Container& lhs, const Container& rhs) override
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }
            
            return AZStd::equal(lhs.begin(), lhs.end(), rhs.begin());
        }
    };

    using BasicContainerConformityTestTypes = ::testing::Types
    <
        SimpleTestDescription<AZStd::list<int>>,
        SimpleTestDescription<AZStd::set<int>>,
        SimpleTestDescription<AZStd::vector<int>>,
        SimpleTestDescription<AZStd::fixed_vector<int, 256>>,
        SimplePointerTestDescription<AZStd::forward_list<int*>>,
        SimplePointerTestDescription<AZStd::list<int*>>,
        SimplePointerTestDescription<AZStd::set<int*, SimplePointerTestDescriptionLess>>,
        SimplePointerTestDescription<AZStd::vector<int*>>,
        SimplePointerTestDescription<AZStd::fixed_vector<int*, 256>>,
        ComplextTestDescription<AZStd::forward_list<SimpleClass>>,
        ComplextTestDescription<AZStd::list<SimpleClass>>,
        ComplextTestDescription<AZStd::set<SimpleClass>>,
        ComplextTestDescription<AZStd::vector<SimpleClass>>,
        ComplextTestDescription<AZStd::fixed_vector<SimpleClass, 256>>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonBasicContainers, JsonSerializerConformityTests, BasicContainerConformityTestTypes);

    class JsonBasicContainerSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<AZ::JsonBasicContainerSerializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        AZStd::unique_ptr<AZ::JsonBasicContainerSerializer> m_serializer;
    };


    // Generic additional tests for the BasicContaienerSerializer using AZStd::vector

    class JsonVectorSerializerTests
        : public JsonBasicContainerSerializerTests
    {
    public:
        using Container = AZStd::vector<SimpleClass>;
        using BaseClassContainer = AZStd::vector<AZStd::shared_ptr<BaseClass>>;

        using JsonBasicContainerSerializerTests::RegisterAdditional;
        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            SimpleClass::Reflect(serializeContext, true);
            SimpleInheritence::Reflect(serializeContext, true);
            serializeContext->RegisterGenericType<Container>();
            serializeContext->RegisterGenericType<AZStd::shared_ptr<BaseClass>>();
            serializeContext->RegisterGenericType<AZStd::shared_ptr<SimpleInheritence>>();
            serializeContext->RegisterGenericType<BaseClassContainer>();
        }
    };

    TEST_F(JsonVectorSerializerTests, Store_SingleObjectWithAllDefaults_ReturnsDefaultsUsed)
    {
        using namespace AZ::JsonSerializationResult;

        Container instance;
        instance.emplace_back();
        
        ResultCode result = m_serializer->Store(*m_jsonDocument, &instance, &instance, azrtti_typeid(&instance), *m_jsonSerializationContext);
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq("[{}]");
    }

    TEST_F(JsonVectorSerializerTests, Store_NonEmptyVectorWhoseElementsHaveAllDefaults_ReturnsDefaultsUsed)
    {
        using namespace AZ::JsonSerializationResult;

        BaseClassContainer instance;
        instance.emplace_back(AZStd::make_shared<BaseClass>());
        instance.emplace_back(AZStd::make_shared<BaseClass>());

        ResultCode result = m_serializer->Store(*m_jsonDocument, &instance, nullptr, azrtti_typeid(&instance), *m_jsonSerializationContext);
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq("[{},{}]");
    }

    TEST_F(JsonVectorSerializerTests, Store_VectorValuesAreInheritedClassWithAllDefaults_DoesNotReturnDefaultsUsed)
    {
        using namespace AZ::JsonSerializationResult;

        BaseClassContainer instance;
        instance.emplace_back(AZStd::make_shared<SimpleInheritence>());
        instance.emplace_back(AZStd::make_shared<SimpleInheritence>());

        ResultCode result = m_serializer->Store(*m_jsonDocument, &instance, nullptr, azrtti_typeid(&instance), *m_jsonSerializationContext);
        EXPECT_EQ(Processing::Completed, result.GetProcessing());
        EXPECT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        Expect_DocStrEq(R"([{"$type": "SimpleInheritence"},{"$type": "SimpleInheritence"}])");
    }

    // Specific tests for AZStd::fixed_vector

    class JsonFixedVectorSerializerTests
        : public JsonBasicContainerSerializerTests
    {
    public:
        static constexpr size_t ContainerSize = 4;
        using Container = AZStd::fixed_vector<int, ContainerSize>;

        using JsonBasicContainerSerializerTests::RegisterAdditional;
        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            serializeContext->RegisterGenericType<Container>();
        }
    };

    TEST_F(JsonFixedVectorSerializerTests, Load_MoreEntriesThanFit_LoadEntriesThatFitAndRestIsSkipped)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value testVal(rapidjson::kArrayType);
        testVal.PushBack(rapidjson::Value().SetInt64(1), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value().SetInt64(2), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value().SetInt64(3), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value().SetInt64(4), m_jsonDocument->GetAllocator());
        testVal.PushBack(rapidjson::Value().SetInt64(5), m_jsonDocument->GetAllocator());

        Container instance;
        ResultCode result = m_serializer->Load(&instance, azrtti_typeid(&instance), testVal, *m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::PartialSkip, result.GetOutcome());
        
        EXPECT_EQ(1, instance[0]);
        EXPECT_EQ(2, instance[1]);
        EXPECT_EQ(3, instance[2]);
        EXPECT_EQ(4, instance[3]);
    }

    // Specific tests for AZStd::set

    class JsonSetSerializerTests
        : public JsonBasicContainerSerializerTests
    {
    public:
        using Set = AZStd::set<int>;

        using JsonBasicContainerSerializerTests::RegisterAdditional;
        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& serializeContext) override
        {
            serializeContext->RegisterGenericType<Set>();
        }
    };

    TEST_F(JsonSetSerializerTests, Load_DuplicateEntry_CompletesWithDataLeft)
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

    TEST_F(JsonSetSerializerTests, Load_EntryAlreadyInContainer_EntryIgnored)
    {
        using namespace AZ::JsonSerializationResult;
        Set instance;
        instance.insert(188);

        rapidjson::Value testVal(rapidjson::kArrayType);
        testVal.PushBack(rapidjson::Value(188), m_jsonDocument->GetAllocator());

        ResultCode result = m_serializer->Load(&instance, azrtti_typeid(&instance), testVal, *m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Altered, result.GetProcessing());
        EXPECT_EQ(Outcomes::Unavailable, result.GetOutcome());
        EXPECT_NE(instance.end(), instance.find(188));
    }
} // namespace JsonSerializationTests
