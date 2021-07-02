/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/MathVectorSerializer.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Serialization/Json/DoubleSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    template<typename VectorType, size_t ComponentCount, typename Serializer>
    class MathVectorSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<VectorType>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<Serializer>();
        }

        AZStd::shared_ptr<VectorType> CreateDefaultInstance() override
        {
            auto vector = AZStd::make_shared<VectorType>();
            for (size_t i = 0; i < ComponentCount; ++i)
            {
                vector->SetElement(aznumeric_caster(i), 0.0f);
            }
            return vector;
        }

        AZStd::shared_ptr<VectorType> CreateFullySetInstance() override
        {
            auto vector = AZStd::make_shared<VectorType>();
            for (size_t i=0; i<ComponentCount; ++i)
            {
                vector->SetElement(aznumeric_caster(i), 1.0f + i);
            }
            return vector;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            if constexpr (ComponentCount == 2)
            {
                return "[1.0, 2.0]";
            }
            else if constexpr (ComponentCount == 3)
            {
                return "[1.0, 2.0, 3.0]";
            }
            else if constexpr (ComponentCount == 4)
            {
                return "[1.0, 2.0, 3.0, 4.0]";
            }
            else
            {
                static_assert(ComponentCount >= 2 && ComponentCount <= 4,
                    "Only vector 2, 3 and 4 are supported by this test.");
            }
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_fixedSizeArray = true;
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const VectorType& lhs, const VectorType& rhs) override
        {
            return lhs == rhs;
        }
    };

    using MathVectorSerializerConformityTestTypes = ::testing::Types<
        MathVectorSerializerTestDescription<AZ::Vector2, 2, AZ::JsonVector2Serializer>,
        MathVectorSerializerTestDescription<AZ::Vector3, 3, AZ::JsonVector3Serializer>,
        MathVectorSerializerTestDescription<AZ::Vector4, 4, AZ::JsonVector4Serializer>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonMathVectorSerializer, JsonSerializerConformityTests, MathVectorSerializerConformityTestTypes);

    template<typename T>
    class JsonMathVectorSerializerTests
        : public BaseJsonSerializerFixture
    {
    public:
        using Descriptor = T;

        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();
            m_serializer = AZStd::make_unique<typename T::Serializer>();
        }

        void TearDown() override
        {
            m_serializer.reset();
            BaseJsonSerializerFixture::TearDown();
        }

    protected:
        constexpr static const char* upperCaseFieldNames[4] = { "X", "Y", "Z", "W" };
        constexpr static const char* lowerCaseFieldNames[4] = { "x", "y", "z", "w" };

        AZStd::unique_ptr<typename T::Serializer> m_serializer;
    };

    struct Vector2Descriptor
    {
        using VectorType = AZ::Vector2;
        using Serializer = AZ::JsonVector2Serializer;
        constexpr static size_t ElementCount = 2;
    };

    struct Vector3Descriptor
    {
        using VectorType = AZ::Vector3;
        using Serializer = AZ::JsonVector3Serializer;
        constexpr static size_t ElementCount = 3;
    };

    struct Vector4Descriptor
    {
        using VectorType = AZ::Vector4;
        using Serializer = AZ::JsonVector4Serializer;
        constexpr static size_t ElementCount = 4;
    };

    struct QuaternionDescriptor
    {
        using VectorType = AZ::Quaternion;
        using Serializer = AZ::JsonQuaternionSerializer;
        constexpr static size_t ElementCount = 4;
    };

    using JsonMathVectorSerializerTypes = ::testing::Types <
        Vector2Descriptor, Vector3Descriptor, Vector4Descriptor>;
    TYPED_TEST_CASE(JsonMathVectorSerializerTests, JsonMathVectorSerializerTypes);

    // Load array tests

    TYPED_TEST(JsonMathVectorSerializerTests, Load_OversizedArray_ReturnsPartialConvertAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount + 1; ++i)
        {
            arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(static_cast<float>(i + 1), output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_InvalidEntries_ReturnsUnsupportedAndLeavesVectorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            if (i == 1)
            {
                arrayValue.PushBack(rapidjson::StringRef("Invalid"), this->m_jsonDocument->GetAllocator());
            }
            else
            {
                arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
            }
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());

        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(0.0f, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_FloatSerializerMissingForArray_ReturnsCatastrophic)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_jsonRegistrationContext->EnableRemoveReflection();
        this->m_jsonRegistrationContext->template Serializer<AZ::JsonFloatSerializer>()->template HandlesType<float>();
        this->m_jsonRegistrationContext->DisableRemoveReflection();

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount + 1; ++i)
        {
            arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
        }
        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output;
        ResultCode result = this->m_serializer->Load(&output,
            azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());

        this->m_jsonRegistrationContext->template Serializer<AZ::JsonFloatSerializer>()->template HandlesType<float>();
    }

    // Load object tests
    TYPED_TEST(JsonMathVectorSerializerTests, Load_ValidObjectUpperCase_ReturnsSuccessAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            objectValue.AddMember(rapidjson::StringRef(JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[i]), 1.0f + i,
                this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output =
            JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(&output,
            azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(1.0f + i, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_ValidObjectLowerCase_ReturnsSuccessAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            objectValue.AddMember(rapidjson::StringRef(JsonMathVectorSerializerTests<TypeParam>::lowerCaseFieldNames[i]), 1.0f + i,
                this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output =
            JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(&output,
            azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(1.0f + i, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_ValidObjectWithExtraFields_ReturnsPartialConvertAndLoadsVector)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            objectValue.AddMember(rapidjson::StringRef(JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[i]), 1.0f + i,
                this->m_jsonDocument->GetAllocator());
        }
        this->InjectAdditionalFields(objectValue, rapidjson::kStringType, this->m_jsonDocument->GetAllocator());

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output =
            JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(&output,
            azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(1.0f + i, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_MissingFields_ReturnsPartialDefaultValueAndVectorHasWhatCouldBeLoaded)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            if (i == 1)
            {
                continue;
            }
            objectValue.AddMember(rapidjson::StringRef(JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[i]), 1.0f + i,
                this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output =
            JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(&output,
            azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::PartialDefaults, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            float expectedValue = i == 1 ? 0.0f : (1.0f + i);
            EXPECT_FLOAT_EQ(expectedValue, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_InvalidFields_ReturnsUnsupportedAndLeavesVectorUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            if (i == 1)
            {
                objectValue.AddMember(rapidjson::StringRef(JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[i]),
                    rapidjson::StringRef("Invalid"), this->m_jsonDocument->GetAllocator());
            }
            else
            {
                objectValue.AddMember(rapidjson::StringRef(JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[i]),
                    1.0f + i, this->m_jsonDocument->GetAllocator());
            }
        }

        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output = JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output, azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(), *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Unsupported, result.GetOutcome());
        
        for (int i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            EXPECT_FLOAT_EQ(0.0f, output.GetElement(i));
        }
    }

    TYPED_TEST(JsonMathVectorSerializerTests, Load_FloatSerializerMissingForObject_ReturnsCatastrophic)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_jsonRegistrationContext->EnableRemoveReflection();
        this->m_jsonRegistrationContext->template Serializer<AZ::JsonFloatSerializer>()->template HandlesType<float>();
        this->m_jsonRegistrationContext->DisableRemoveReflection();

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        for (size_t i = 0; i < JsonMathVectorSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            objectValue.AddMember(rapidjson::StringRef(JsonMathVectorSerializerTests<TypeParam>::upperCaseFieldNames[0]), 1.0f + i,
                this->m_jsonDocument->GetAllocator());
        }
        typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType output;
        ResultCode result = this->m_serializer->Load(&output,
            azrtti_typeid<typename JsonMathVectorSerializerTests<TypeParam>::Descriptor::VectorType>(),
            *this->m_jsonDocument, *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());

        this->m_jsonRegistrationContext->template Serializer<AZ::JsonFloatSerializer>()->template HandlesType<float>();
    }
} // namespace JsonSerializationTests
