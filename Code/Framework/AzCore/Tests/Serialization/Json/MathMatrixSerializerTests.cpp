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

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/MathMatrixSerializer.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/Json/DoubleSerializer.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    template<typename MatrixType, size_t RowCount, size_t ColumnCount, typename Serializer>
    class MathMatrixSerializerTestDescription :
        public JsonSerializerConformityTestDescriptor<MatrixType>
    {
    public:
        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<Serializer>();
        }

        AZStd::shared_ptr<MatrixType> CreateDefaultInstance() override
        {
            auto matrix = AZStd::make_shared<MatrixType>();
            for (size_t r = 0; r < RowCount; ++r)
            {
                for (size_t c = 0; c < ColumnCount; ++c)
                {
                    matrix->SetElement(aznumeric_caster(r), aznumeric_caster(c), 0.0f);
                }
            }
            return matrix;
        }

        AZStd::shared_ptr<MatrixType> CreateFullySetInstance() override
        {
            auto matrix = AZStd::make_shared<MatrixType>();
            for (size_t r = 0; r < RowCount; ++r)
            {
                for (size_t c = 0; c < ColumnCount; ++c)
                {
                    matrix->SetElement(aznumeric_caster(r), aznumeric_caster(c), 1.0f + r + c);
                }
            }
            return matrix;
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            if constexpr (RowCount + ColumnCount == 9)
            {
                return "[[1.0, 2.0, 3.0],[1.0, 2.0, 3.0],[1.0, 2.0, 3.0]]";
            }
            else if constexpr (RowCount + ColumnCount == 12)
            {
                return "[[1.0, 2.0, 3.0, 4.0],[1.0, 2.0, 3.0, 4.0],[1.0, 2.0, 3.0, 4.0]]";
            }
            else if constexpr (RowCount + ColumnCount == 16)
            {
                return "[[1.0, 2.0, 3.0, 4.0],[1.0, 2.0, 3.0, 4.0],[1.0, 2.0, 3.0, 4.0],[1.0, 2.0, 3.0, 4.0]]";
            }
            else
            {
                static_assert((RowCount >= 3 && RowCount <= 4) && (ColumnCount >= 3 && ColumnCount <= 4),
                    "Only matrix 3x3, 3x4 or 4x4 are supported by this test.");
            }
            return "";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kArrayType);
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_fixedSizeArray = true;
            features.m_supportsPartialInitialization = false;
            features.m_supportsInjection = false;
        }

        bool AreEqual(const MatrixType& lhs, const MatrixType& rhs) override
        {
            return lhs == rhs;
        }
    };

    using MathMatrixSerializerConformityTestTypes = ::testing::Types<
        MathMatrixSerializerTestDescription<AZ::Matrix3x3, 3, 3, AZ::JsonMatrix3x3Serializer>,
        MathMatrixSerializerTestDescription<AZ::Matrix3x4, 3, 4, AZ::JsonMatrix3x4Serializer>,
        MathMatrixSerializerTestDescription<AZ::Matrix4x4, 4, 4, AZ::JsonMatrix4x4Serializer>
    >;
    INSTANTIATE_TYPED_TEST_CASE_P(JsonMathMatrixSerializer, JsonSerializerConformityTests, MathMatrixSerializerConformityTestTypes);

    template<typename T>
    class JsonMathMatrixSerializerTests
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


        void AddRotation(rapidjson::Value& value, const AZ::Quaternion& rotation, rapidjson::Document::AllocatorType& allocator)
        {
            const auto degrees = rotation.GetEulerDegrees();
            value.AddMember("yaw", degrees.GetX(), allocator);
            value.AddMember("pitch", degrees.GetY(), allocator);
            value.AddMember("roll", degrees.GetZ(), allocator);
        }

        void AddScale(rapidjson::Value& value, float scale, rapidjson::Document::AllocatorType& allocator)
        {
            value.AddMember("scale", scale, allocator);
        }

        void AddTranslation(rapidjson::Value& value, const AZ::Vector3& translation, rapidjson::Document::AllocatorType& allocator)
        {
            value.AddMember("x", translation.GetX(), allocator);
            value.AddMember("y", translation.GetY(), allocator);
            value.AddMember("z", translation.GetZ(), allocator);
        }

        template <bool>
        void AddData(rapidjson::Value& value, const typename Descriptor::MatrixType& matrix, rapidjson::Document::AllocatorType& allocator);

        template <>
        void AddData<false>(rapidjson::Value& value, const typename Descriptor::MatrixType& matrix, rapidjson::Document::AllocatorType& allocator)
        {
            AddScale(value, matrix.RetrieveScale().GetX(), allocator);
            AddRotation(value, Descriptor::CreateQuaternion(matrix), allocator);
        }

        template <>
        void AddData<true>(rapidjson::Value& value, const typename Descriptor::MatrixType& matrix, rapidjson::Document::AllocatorType& allocator)
        {
            AddScale(value, matrix.RetrieveScale().GetX(), allocator);
            AddTranslation(value, matrix.GetTranslation(), allocator);
            AddRotation(value, Descriptor::CreateQuaternion(matrix), allocator);
        }

        enum class InputTypes
        {
            Zero,
            Identity,
            Incremental
        };

        typename Descriptor::MatrixType CreateMatrixInstance(InputTypes testType)
        {
            typename Descriptor::MatrixType matrix;
            if (testType == InputTypes::Zero)
            {
                matrix = Descriptor::MatrixType::CreateZero();
            }
            else if (testType == InputTypes::Identity)
            {
                matrix = Descriptor::MatrixType::CreateIdentity();
            }
            else if (testType == InputTypes::Incremental)
            {
                float value = 0.0f;
                for (size_t r = 0; r < Descriptor::RowCount; ++r)
                {
                    for (size_t c = 0; c < Descriptor::ColumnCount; ++c)
                    {
                        matrix.SetElement(aznumeric_caster(r), aznumeric_caster(c), value);
                        value += 1.0f;
                    }
                }
            }
            return matrix;
        }

        template <bool HasTranslation>
        typename Descriptor::MatrixType CreateArbitraryMatrix(size_t seed);

        template <>
        typename Descriptor::MatrixType CreateArbitraryMatrix<false>(size_t seed)
        {
            AZ::SimpleLcgRandom random(seed);

            // start a matrix with arbitrary degrees
            float roll = random.GetRandomFloat() * 360.0f;
            float pitch = random.GetRandomFloat() * 360.0f;
            float yaw = random.GetRandomFloat() * 360.0f;
            const AZ::Vector3 eulerRadians = AZ::Vector3DegToRad(AZ::Vector3{ roll, pitch, yaw });
            const auto rotX = typename Descriptor::MatrixType::CreateRotationX(eulerRadians.GetX());
            const auto rotY = typename Descriptor::MatrixType::CreateRotationY(eulerRadians.GetY());
            const auto rotZ = typename Descriptor::MatrixType::CreateRotationZ(eulerRadians.GetZ());
            auto matrix = rotX * rotY * rotZ;

            // apply a scale
            matrix.MultiplyByScale(AZ::Vector3{ random.GetRandomFloat() });
            return matrix;
        }

        template <>
        typename Descriptor::MatrixType CreateArbitraryMatrix<true>(size_t seed)
        {
            auto matrix = CreateArbitraryMatrix<false>(seed);
            AZ::SimpleLcgRandom random(seed * seed);
            float x = random.GetRandomFloat() * 10000.0f;
            float y = random.GetRandomFloat() * 10000.0f;
            float z = random.GetRandomFloat() * 10000.0f;
            matrix.SetTranslation(AZ::Vector3{ x, y, z });
            return matrix;
        }

    protected:
        AZStd::unique_ptr<typename T::Serializer> m_serializer;
    };

    struct Matrix3x3Descriptor
    {
        using MatrixType = AZ::Matrix3x3;
        using Serializer = AZ::JsonMatrix3x3Serializer;
        constexpr static size_t RowCount = 3;
        constexpr static size_t ColumnCount = 3;
        constexpr static size_t ElementCount = RowCount * ColumnCount;
        constexpr static bool HasTranslation = false;
        using CreateQuaternionFunc = AZ::Quaternion(*) (const typename MatrixType&);
        constexpr static CreateQuaternionFunc CreateQuaternion = &AZ::Quaternion::CreateFromMatrix3x3;
    };

    struct Matrix3x4Descriptor
    {
        using MatrixType = AZ::Matrix3x4;
        using Serializer = AZ::JsonMatrix3x4Serializer;
        constexpr static size_t RowCount = 3;
        constexpr static size_t ColumnCount = 4;
        constexpr static size_t ElementCount = RowCount * ColumnCount;
        constexpr static bool HasTranslation = true;
        using CreateQuaternionFunc = AZ::Quaternion(*) (const typename MatrixType&);
        constexpr static CreateQuaternionFunc CreateQuaternion = &AZ::Quaternion::CreateFromMatrix3x4;
    };

    struct Matrix4x4Descriptor
    {
        using MatrixType = AZ::Matrix4x4;
        using Serializer = AZ::JsonMatrix4x4Serializer;
        constexpr static size_t RowCount = 4;
        constexpr static size_t ColumnCount = 4;
        constexpr static size_t ElementCount = RowCount * ColumnCount;
        constexpr static bool HasTranslation = true;
        using CreateQuaternionFunc = AZ::Quaternion(*) (const typename MatrixType&);
        constexpr static CreateQuaternionFunc CreateQuaternion = &AZ::Quaternion::CreateFromMatrix4x4;
    };

    using JsonMathMatrixSerializerTypes = ::testing::Types <
        Matrix3x3Descriptor, Matrix3x4Descriptor, Matrix4x4Descriptor>;
    TYPED_TEST_CASE(JsonMathMatrixSerializerTests, JsonMathMatrixSerializerTypes);

    // Load array tests

    TYPED_TEST(JsonMathMatrixSerializerTests, Load_Array_ReturnsConvertAndLoadsMatrix)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathMatrixSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
        {
            arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
        }

        auto output = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        
        for (int r = 0; r < JsonMathMatrixSerializerTests<TypeParam>::Descriptor::RowCount; ++r)
        {
            for (int c = 0; c < JsonMathMatrixSerializerTests<TypeParam>::Descriptor::ColumnCount; ++c)
            {
                auto testValue = static_cast<float>((r * JsonMathMatrixSerializerTests<TypeParam>::Descriptor::ColumnCount) + c + 1);
                EXPECT_FLOAT_EQ(testValue, output.GetElement(r, c));
            }
        }
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, Load_InvalidEntries_ReturnsUnsupportedAndLeavesMatrixUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathMatrixSerializerTests<TypeParam>::Descriptor::ElementCount; ++i)
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

        auto output = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Unsupported, result.GetOutcome());

        for (int r = 0; r < JsonMathMatrixSerializerTests<TypeParam>::Descriptor::RowCount; ++r)
        {
            for (int c = 0; c < JsonMathMatrixSerializerTests<TypeParam>::Descriptor::ColumnCount; ++c)
            {
                EXPECT_FLOAT_EQ(0.0f, output.GetElement(r, c));
            }
        }
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, Load_FloatSerializerMissingForArray_ReturnsCatastrophic)
    {
        using namespace AZ::JsonSerializationResult;

        this->m_jsonRegistrationContext->EnableRemoveReflection();
        this->m_jsonRegistrationContext->template Serializer<AZ::JsonFloatSerializer>()->template HandlesType<float>();
        this->m_jsonRegistrationContext->DisableRemoveReflection();

        rapidjson::Value& arrayValue = this->m_jsonDocument->SetArray();
        for (size_t i = 0; i < JsonMathMatrixSerializerTests<TypeParam>::Descriptor::ElementCount + 1; ++i)
        {
            arrayValue.PushBack(static_cast<float>(i + 1), this->m_jsonDocument->GetAllocator());
        }

        typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType output;
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        EXPECT_EQ(Outcomes::Catastrophic, result.GetOutcome());

        this->m_jsonRegistrationContext->template Serializer<AZ::JsonFloatSerializer>()->template HandlesType<float>();
    }

    // Load object tests
    TYPED_TEST(JsonMathMatrixSerializerTests, Load_ValidObjectLowerCase_ReturnsSuccessAndLoadsMatrix)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        auto input = this->CreateMatrixInstance(InputTypes::Identity);
        this->AddData<JsonMathMatrixSerializerTests<TypeParam>::Descriptor::HasTranslation>(objectValue, input, this->m_jsonDocument->GetAllocator());

        auto output = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(input, output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, Load_ValidObjectWithExtraFields_ReturnsPartialConvertAndLoadsMatrix)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        auto input = this->CreateMatrixInstance(InputTypes::Identity);
        this->AddScale(objectValue, input.RetrieveScale().GetX(), this->m_jsonDocument->GetAllocator());
        this->AddRotation(objectValue, Descriptor::CreateQuaternion(input), this->m_jsonDocument->GetAllocator());
        objectValue.AddMember(rapidjson::StringRef("extra"), "no value", this->m_jsonDocument->GetAllocator());

        auto output = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Success, result.GetOutcome());
        EXPECT_EQ(input, output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, SaveLoad_Identity_LoadsDefaultMatrixWithIdentity)
    {
        using namespace AZ::JsonSerializationResult;

        auto defaultValue = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateIdentity();

        rapidjson::Value& objectInput = this->m_jsonDocument->SetObject();
        this->m_serializer->Store(
            objectInput,
            &defaultValue,
            &defaultValue,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonSerializationContext);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        objectInput.Accept(writer);

        auto output = defaultValue;
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);

        EXPECT_EQ(defaultValue, output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, LoadSave_Zero_SavesAndLoadsIdentityMatrix)
    {
        using namespace AZ::JsonSerializationResult;

        auto defaultValue = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateIdentity();
        auto input = this->CreateMatrixInstance(InputTypes::Zero);

        rapidjson::Value& objectInput = this->m_jsonDocument->SetObject();
        this->m_serializer->Store(
            objectInput,
            &input,
            &defaultValue,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonSerializationContext);

        auto output = defaultValue;
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);

        ASSERT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(defaultValue, output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, Load_InvalidFields_ReturnsUnsupportedAndLeavesMatrixUntouched)
    {
        using namespace AZ::JsonSerializationResult;

        const auto defaultValue = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateIdentity();
        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        auto input = this->CreateMatrixInstance(InputTypes::Identity);
        this->AddData<JsonMathMatrixSerializerTests<TypeParam>::Descriptor::HasTranslation>(objectValue, input, this->m_jsonDocument->GetAllocator());
        objectValue["yaw"] = "Invalid";

        auto output = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_EQ(input, output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, LoadSave_Arbitrary_SavesAndLoadsArbitraryMatrix)
    {
        using namespace AZ::JsonSerializationResult;
        using Descriptor = typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor;

        auto defaultValue = Descriptor::MatrixType::CreateIdentity();
        auto elementCount = Descriptor::RowCount * Descriptor::ColumnCount;
        auto input = this->CreateArbitraryMatrix<Descriptor::HasTranslation>(elementCount);

        rapidjson::Value& objectInput = this->m_jsonDocument->SetObject();
        this->m_serializer->Store(
            objectInput,
            &input,
            &defaultValue,
            azrtti_typeid<typename Descriptor::MatrixType>(),
            *this->m_jsonSerializationContext);

        auto output = defaultValue;
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);

        EXPECT_EQ(Processing::Completed, result.GetProcessing());

        for (int r = 0; r < Descriptor::RowCount; ++r)
        {
            for (int c = 0; c < Descriptor::ColumnCount; ++c)
            {
                EXPECT_NEAR(input.GetElement(r, c), output.GetElement(r, c), AZ::Constants::Tolerance);
            }
        }
    }

} // namespace JsonSerializationTests
