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

#include <AzCore/Serialization/Json/ArraySerializer.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>
#include <Tests/Serialization/Json/TestCases_Base.h>
#include <Tests/Serialization/Json/TestCases_Classes.h>

namespace JsonSerializationTests
{
    template<typename T>
    class ArraySerializerTestDescriptionBase :
        public JsonSerializerConformityTestDescriptor<T>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::JsonArraySerializer>();
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.m_supportsPartialInitialization = true;
            features.m_supportsInjection = false;
            features.m_requiresTypeIdLookups = true;
            features.m_fixedSizeArray = true;
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<T>();
        }
    };

    class SimpleArraySerializerTestDescription :
        public ArraySerializerTestDescriptionBase<AZStd::array<int, 4>>
    {
    public:
        using Array = AZStd::array<int, 4>;
        using Base = ArraySerializerTestDescriptionBase<Array>;

        AZStd::shared_ptr<Array> CreateDefaultInstance() override
        {
            auto array = AZStd::make_shared<Array>();
            (*array)[0] = 0;
            (*array)[1] = 0;
            (*array)[2] = 0;
            (*array)[3] = 0;
            return array;
        }

        AZStd::shared_ptr<Array> CreatePartialDefaultInstance()
        {
            auto array = AZStd::make_shared<Array>();
            (*array)[0] = 10;
            (*array)[1] = 0;
            (*array)[2] = 30;
            (*array)[3] = 0;
            return array;
        }

        AZStd::shared_ptr<Array> CreateFullySetInstance() override
        {
            auto array = AZStd::make_shared<Array>();
            (*array)[0] = 10;
            (*array)[1] = 20;
            (*array)[2] = 30;
            (*array)[3] = 40;
            return array;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return "[10, {}, 30, {}]";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return "[10, 20, 30, 40]";
        }

        bool AreEqual(const Array& lhs, const Array& rhs) override
        {
            if (lhs.size() != rhs.size())
            {
                return false;
            }

            return AZStd::equal(lhs.begin(), lhs.end(), rhs.begin());
        }
    };

    class ComplexArraySerializerTestDescription :
        public ArraySerializerTestDescriptionBase<AZStd::array<BaseClass2*, 4>>
    {
    public:
        using Array = AZStd::array<BaseClass2*, 4>;
        using Base = ArraySerializerTestDescriptionBase<Array>;

        static void Deleter(Array* instance)
        {
            if (instance)
            {
                for (BaseClass2* pointer : *instance)
                {
                    delete pointer;
                }
                delete instance;
            }
        }

        AZStd::shared_ptr<Array> CreateDefaultInstance() override
        {
            auto array = AZStd::shared_ptr<Array>(new Array(), Deleter);
            (*array)[0] = nullptr;
            (*array)[1] = aznew MultipleInheritence();
            (*array)[2] = nullptr;
            (*array)[3] = aznew MultipleInheritence();
            return array;
        }

        AZStd::shared_ptr<Array> CreatePartialDefaultInstance()
        {
            auto partialInstance = aznew MultipleInheritence();
            partialInstance->m_var1 = 142;
            partialInstance->m_baseVar = 242.0f;

            auto array = AZStd::shared_ptr<Array>(new Array(), Deleter);
            (*array)[0] = aznew MultipleInheritence();
            (*array)[1] = nullptr;
            (*array)[2] = nullptr;
            (*array)[3] = partialInstance;
            return array;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
            [
                { "$type": "MultipleInheritence" },
                null,
                null,
                {
                    "base_var": 242.0,
                    "var1" : 142
                }
            ])";
        }


        AZStd::shared_ptr<Array> CreateFullySetInstance() override
        {
            auto var1 = aznew MultipleInheritence();
            var1->m_baseVar = 1142.0f;
            var1->m_base2Var1 = 1242.0;
            var1->m_base2Var2 = 1342.0;
            var1->m_base2Var3 = 1442.0;
            var1->m_var1 = 1542;
            var1->m_var2 = 1642.0f;

            auto var2 = aznew MultipleInheritence();
            var2->m_baseVar = 2142.0f;
            var2->m_base2Var1 = 2242.0;
            var2->m_base2Var2 = 2342.0;
            var2->m_base2Var3 = 2442.0;
            var2->m_var1 = 2542;
            var2->m_var2 = 2642.0f;

            auto var3 = aznew MultipleInheritence();
            var3->m_baseVar = 3142.0f;
            var3->m_base2Var1 = 3242.0;
            var3->m_base2Var2 = 3342.0;
            var3->m_base2Var3 = 3442.0;
            var3->m_var1 = 3542;
            var3->m_var2 = 3642.0f;

            auto var4 = aznew MultipleInheritence();
            var4->m_baseVar = 4142.0f;
            var4->m_base2Var1 = 4242.0;
            var4->m_base2Var2 = 4342.0;
            var4->m_base2Var3 = 4442.0;
            var4->m_var1 = 4542;
            var4->m_var2 = 4642.0f;

            auto array = AZStd::shared_ptr<Array>(new Array(), Deleter);
            (*array)[0] = var1;
            (*array)[1] = var2;
            (*array)[2] = var3;
            (*array)[3] = var4;
            return array;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
            [
                {
                    "$type": "MultipleInheritence",
                    "base_var": 1142.0,
                    "base2_var1": 1242.0,
                    "base2_var2": 1342.0,
                    "base2_var3": 1442.0,
                    "var1" : 1542,
                    "var2" : 1642.0
                },
                {
                    "$type": "MultipleInheritence",
                    "base_var": 2142.0,
                    "base2_var1": 2242.0,
                    "base2_var2": 2342.0,
                    "base2_var3": 2442.0,
                    "var1" : 2542,
                    "var2" : 2642.0
                },
                {
                    "$type": "MultipleInheritence",
                    "base_var": 3142.0,
                    "base2_var1": 3242.0,
                    "base2_var2": 3342.0,
                    "base2_var3": 3442.0,
                    "var1" : 3542,
                    "var2" : 3642.0
                },
                {
                    "$type": "MultipleInheritence",
                    "base_var": 4142.0,
                    "base2_var1": 4242.0,
                    "base2_var2": 4342.0,
                    "base2_var3": 4442.0,
                    "var1" : 4542,
                    "var2" : 4642.0
                }
            ])";
        }

        AZStd::string_view GetJsonFor_Store_SerializeFullySetInstance() override
        {
            // This is a unique situation because the $type is determined separate from other values, so all
            // member values can be changed, but since the default type matches the stored type the $type
            // will only be written if default values are explicitly kept.
            return R"(
            [
                {
                    "$type": "MultipleInheritence",
                    "base_var": 1142.0,
                    "base2_var1": 1242.0,
                    "base2_var2": 1342.0,
                    "base2_var3": 1442.0,
                    "var1" : 1542,
                    "var2" : 1642.0
                },
                {
                    "base_var": 2142.0,
                    "base2_var1": 2242.0,
                    "base2_var2": 2342.0,
                    "base2_var3": 2442.0,
                    "var1" : 2542,
                    "var2" : 2642.0
                },
                {
                    "$type": "MultipleInheritence",
                    "base_var": 3142.0,
                    "base2_var1": 3242.0,
                    "base2_var2": 3342.0,
                    "base2_var3": 3442.0,
                    "var1" : 3542,
                    "var2" : 3642.0
                },
                {
                    "base_var": 4142.0,
                    "base2_var1": 4242.0,
                    "base2_var2": 4342.0,
                    "base2_var3": 4442.0,
                    "var1" : 4542,
                    "var2" : 4642.0
                }
            ])";
        }

        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            Base::Reflect(context);
            MultipleInheritence::Reflect(context, true);
        }

        bool AreEqual(const Array& lhs, const Array& rhs) override
        {
            size_t size = lhs.size();
            if (size != rhs.size())
            {
                return false;
            }

            for (size_t i = 0; i < size; ++i)
            {
                if (lhs[i] == nullptr)
                {
                    return rhs[i] == nullptr;
                }

                if (!static_cast<const MultipleInheritence*>(lhs[i])->Equals(*static_cast<const MultipleInheritence*>(rhs[i]), true))
                {
                    return false;
                }
            }

            return true;
        }
    };

    using ArraySerializerConformityTestTypes = ::testing::Types<SimpleArraySerializerTestDescription, ComplexArraySerializerTestDescription>;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonArraySerializer, JsonSerializerConformityTests, ArraySerializerConformityTestTypes);

    class JsonArraySerializerTests
        : public BaseJsonSerializerFixture
    {
    protected:
        using Array = AZStd::array<int, 4>;
        AZ::JsonArraySerializer m_serializer;

    public:
        void RegisterAdditional(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Array>();
            context->RegisterGenericType<AZStd::vector<int>>();
        }
    };

    TEST_F(JsonArraySerializerTests, Load_MetaDataIsNotForContainer_ReturnsUnsupportedAndArrayIsNotTouched)
    {
        namespace JSR = AZ::JsonSerializationResult;

        m_jsonDocument->SetArray();
        Array instance = { 10, 20, 30, 40 };
        JSR::ResultCode result = m_serializer.Load(&instance, azrtti_typeid<int>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(10, instance[0]);
        EXPECT_EQ(20, instance[1]);
        EXPECT_EQ(30, instance[2]);
        EXPECT_EQ(40, instance[3]);
    }

    TEST_F(JsonArraySerializerTests, Load_MetaDataIsForOtherContainer_ReturnsUnsupportedAndArrayIsLNotTouched)
    {
        namespace JSR = AZ::JsonSerializationResult;

        m_jsonDocument->SetArray();
        Array instance = { 10, 20, 30, 40 };
        JSR::ResultCode result = m_serializer.Load(&instance, azrtti_typeid<AZStd::vector<int>>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(10, instance[0]);
        EXPECT_EQ(20, instance[1]);
        EXPECT_EQ(30, instance[2]);
        EXPECT_EQ(40, instance[3]);
    }

    TEST_F(JsonArraySerializerTests, Load_MoreValuesThanFitInArray_ReturnsSkippedAndArrayIsLoaded)
    {
        namespace JSR = AZ::JsonSerializationResult;

        m_jsonDocument->SetArray();
        m_jsonDocument->PushBack(10, m_jsonDocument->GetAllocator());
        m_jsonDocument->PushBack(20, m_jsonDocument->GetAllocator());
        m_jsonDocument->PushBack(30, m_jsonDocument->GetAllocator());
        m_jsonDocument->PushBack(40, m_jsonDocument->GetAllocator());
        m_jsonDocument->PushBack(50, m_jsonDocument->GetAllocator());
        m_jsonDocument->PushBack(60, m_jsonDocument->GetAllocator());

        Array instance = {0, 0, 0, 0};
        JSR::ResultCode result = m_serializer.Load(&instance, azrtti_typeid<Array>(),
            *m_jsonDocument, *m_jsonDeserializationContext);

        EXPECT_EQ(JSR::Outcomes::PartialSkip, result.GetOutcome());
        EXPECT_EQ(10, instance[0]);
        EXPECT_EQ(20, instance[1]);
        EXPECT_EQ(30, instance[2]);
        EXPECT_EQ(40, instance[3]);
    }

    TEST_F(JsonArraySerializerTests, Store_MetaDataIsNotForContainer_ReturnsUnsupportedAndNoJsonCreated)
    {
        namespace JSR = AZ::JsonSerializationResult;

        m_jsonDocument->SetNull();
        Array instance = { 10, 20, 30, 40 };
        JSR::ResultCode result = m_serializer.Store(*m_jsonDocument, &instance, nullptr,
            azrtti_typeid<int>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
        EXPECT_TRUE(m_jsonDocument->IsNull());
    }

    TEST_F(JsonArraySerializerTests, Load_MetaDataIsForOtherContainer_ReturnsUnsupportedAndNoJsonCreated)
    {
        namespace JSR = AZ::JsonSerializationResult;

        m_jsonDocument->SetNull();
        Array instance = { 10, 20, 30, 40 };
        JSR::ResultCode result = m_serializer.Store(*m_jsonDocument, &instance, nullptr,
            azrtti_typeid<AZStd::vector<int>>(), *m_jsonSerializationContext);

        EXPECT_EQ(JSR::Outcomes::Unsupported, result.GetOutcome());
        EXPECT_TRUE(m_jsonDocument->IsNull());
    }
} // namespace JsonSerializationTests
