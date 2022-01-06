/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    namespace DataHelper
    {
        // Build Matrix

        template <typename MatrixType>
        MatrixType BuildMatrixRotationWithSale(const AZ::Vector3& angles, float scale)
        {
            // start a matrix with angle degrees
            const AZ::Vector3 eulerRadians = AZ::Vector3DegToRad(angles);
            const auto rotX = MatrixType::CreateRotationX(eulerRadians.GetX());
            const auto rotY = MatrixType::CreateRotationY(eulerRadians.GetY());
            const auto rotZ = MatrixType::CreateRotationZ(eulerRadians.GetZ());
            auto matrix = rotX * rotY * rotZ;

            // apply a scale
            matrix.MultiplyByScale(AZ::Vector3{ scale });
            return matrix;
        }

        template <typename MatrixType>
        MatrixType BuildMatrix(const AZ::Vector3& angles, float scale, const AZ::Vector3& translation)
        {
            auto matrix = BuildMatrixRotationWithSale<MatrixType>(angles, scale);
            matrix.SetTranslation(translation);
            return matrix;
        }

        template <>
        AZ::Matrix3x3 BuildMatrix(const AZ::Vector3& angles, float scale, const AZ::Vector3&)
        {
            return BuildMatrixRotationWithSale<AZ::Matrix3x3>(angles, scale);
        }

        // Arbitrary Matrix

        template <typename MatrixType>
        MatrixType CreateArbitraryMatrixRotationAndSale(AZ::SimpleLcgRandom& random)
        {
            // start a matrix with arbitrary degrees
            float roll = random.GetRandomFloat() * 360.0f;
            float pitch = random.GetRandomFloat() * 360.0f;
            float yaw = random.GetRandomFloat() * 360.0f;
            const AZ::Vector3 eulerRadians = AZ::Vector3DegToRad(AZ::Vector3{ roll, pitch, yaw });
            const auto rotX = MatrixType::CreateRotationX(eulerRadians.GetX());
            const auto rotY = MatrixType::CreateRotationY(eulerRadians.GetY());
            const auto rotZ = MatrixType::CreateRotationZ(eulerRadians.GetZ());
            auto matrix = rotX * rotY * rotZ;

            // apply a scale
            matrix.MultiplyByScale(AZ::Vector3{ random.GetRandomFloat() });
            return matrix;
        }

        template <typename MatrixType>
        void AssignArbitrarySetTranslation(MatrixType& matrix, AZ::SimpleLcgRandom& random)
        {
            float x = random.GetRandomFloat() * 10000.0f;
            float y = random.GetRandomFloat() * 10000.0f;
            float z = random.GetRandomFloat() * 10000.0f;
            matrix.SetTranslation(AZ::Vector3{ x, y, z });
        }

        template <typename MatrixType>
        MatrixType CreateArbitraryMatrix(size_t seed);

        template <>
        AZ::Matrix3x3 CreateArbitraryMatrix(size_t seed)
        {
            AZ::SimpleLcgRandom random(seed);
            return CreateArbitraryMatrixRotationAndSale<AZ::Matrix3x3>(random);
        }

        template <>
        AZ::Matrix3x4 CreateArbitraryMatrix(size_t seed)
        {
            AZ::SimpleLcgRandom random(seed);
            auto matrix = CreateArbitraryMatrixRotationAndSale<AZ::Matrix3x4>(random);
            AssignArbitrarySetTranslation<AZ::Matrix3x4>(matrix, random);
            return matrix;
        }

        template <>
        AZ::Matrix4x4 CreateArbitraryMatrix(size_t seed)
        {
            AZ::SimpleLcgRandom random(seed);
            auto matrix = CreateArbitraryMatrixRotationAndSale<AZ::Matrix4x4>(random);
            AssignArbitrarySetTranslation<AZ::Matrix4x4>(matrix, random);
            return matrix;
        }

        // CreateQuaternion   

        template<typename MatrixType>
        AZ::Quaternion CreateQuaternion(const MatrixType& matrix);

        template<>
        AZ::Quaternion CreateQuaternion<AZ::Matrix3x3>(const AZ::Matrix3x3& matrix)
        {
            return AZ::Quaternion::CreateFromMatrix3x3(matrix);
        }

        template<>
        AZ::Quaternion CreateQuaternion<AZ::Matrix3x4>(const AZ::Matrix3x4& matrix)
        {
            return AZ::Quaternion::CreateFromMatrix3x4(matrix);
        }

        template<>
        AZ::Quaternion CreateQuaternion<AZ::Matrix4x4>(const AZ::Matrix4x4& matrix)
        {
            return AZ::Quaternion::CreateFromMatrix4x4(matrix);
        }

        template<typename MatrixType>
        void AddRotation(rapidjson::Value& value, const MatrixType& matrix, rapidjson::Document::AllocatorType& allocator)
        {
            AZ::Quaternion rotation = CreateQuaternion<MatrixType>(matrix);
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

        template <typename MatrixType>
        void AddData(rapidjson::Value& value, const MatrixType& matrix, rapidjson::Document::AllocatorType& allocator);

        template <>
        void AddData(rapidjson::Value& value, const AZ::Matrix3x3& matrix, rapidjson::Document::AllocatorType& allocator)
        {
            AddScale(value, matrix.RetrieveScale().GetX(), allocator);
            AddRotation(value, matrix, allocator);
        }

        template <>
        void AddData(rapidjson::Value& value, const AZ::Matrix3x4& matrix, rapidjson::Document::AllocatorType& allocator)
        {
            AddScale(value, matrix.RetrieveScale().GetX(), allocator);
            AddTranslation(value, matrix.GetTranslation(), allocator);
            AddRotation(value, matrix, allocator);
        }

        template <>
        void AddData(rapidjson::Value& value, const AZ::Matrix4x4& matrix, rapidjson::Document::AllocatorType& allocator)
        {
            AddScale(value, matrix.RetrieveScale().GetX(), allocator);
            AddTranslation(value, matrix.GetTranslation(), allocator);
            AddRotation(value, matrix, allocator);
        }
    };

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
            return AZStd::make_shared<MatrixType>(MatrixType::CreateIdentity());
        }

        AZStd::shared_ptr<MatrixType> CreateFullySetInstance() override
        {
            auto angles = AZ::Vector3 { 0.0f, 0.0f, 0.0f };
            auto scale = 10.0f;
            auto translation = AZ::Vector3{ 10.0f, 20.0f, 30.0f };
            auto matrix = DataHelper::BuildMatrix<MatrixType>(angles, scale, translation);
            return AZStd::make_shared<MatrixType>(matrix);
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            if constexpr (RowCount * ColumnCount == 9)
            {
                return "{\"roll\":0.0,\"pitch\":0.0,\"yaw\":0.0,\"scale\":10.0}";
            }
            else if constexpr (RowCount * ColumnCount == 12)
            {
                return "{\"roll\":0.0,\"pitch\":0.0,\"yaw\":0.0,\"scale\":10.0,\"x\":10.0,\"y\":20.0,\"z\":30.0}";
            }
            else if constexpr (RowCount * ColumnCount == 16)
            {
                return "{\"roll\":0.0,\"pitch\":0.0,\"yaw\":0.0,\"scale\":10.0,\"x\":10.0,\"y\":20.0,\"z\":30.0}";
            }
            else
            {
                static_assert((RowCount >= 3 && RowCount <= 4) && (ColumnCount >= 3 && ColumnCount <= 4),
                    "Only matrix 3x3, 3x4 or 4x4 are supported by this test.");
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

        bool AreEqual(const MatrixType& lhs, const MatrixType& rhs) override
        {
            for (int r = 0; r < RowCount; ++r)
            {
                for (int c = 0; c < ColumnCount; ++c)
                {
                    if (!AZ::IsClose(lhs.GetElement(r, c), rhs.GetElement(r, c), AZ::Constants::Tolerance))
                    {
                        return false;
                    }
                }
            }
            return true;
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
    };

    struct Matrix3x4Descriptor
    {
        using MatrixType = AZ::Matrix3x4;
        using Serializer = AZ::JsonMatrix3x4Serializer;
        constexpr static size_t RowCount = 3;
        constexpr static size_t ColumnCount = 4;
        constexpr static size_t ElementCount = RowCount * ColumnCount;
        constexpr static bool HasTranslation = true;
    };

    struct Matrix4x4Descriptor
    {
        using MatrixType = AZ::Matrix4x4;
        using Serializer = AZ::JsonMatrix4x4Serializer;
        constexpr static size_t RowCount = 4;
        constexpr static size_t ColumnCount = 4;
        constexpr static size_t ElementCount = RowCount * ColumnCount;
        constexpr static bool HasTranslation = true;
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
        auto input = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateIdentity();
        DataHelper::AddData(objectValue, input, this->m_jsonDocument->GetAllocator());

        auto output = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        EXPECT_TRUE(input == output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, Load_ValidObjectWithExtraFields_ReturnsPartialConvertAndLoadsMatrix)
    {
        using namespace AZ::JsonSerializationResult;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        auto input = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateIdentity();
        DataHelper::AddScale(objectValue, input.RetrieveScale().GetX(), this->m_jsonDocument->GetAllocator());
        DataHelper::AddRotation(objectValue, input, this->m_jsonDocument->GetAllocator());
        objectValue.AddMember(rapidjson::StringRef("extra"), "no value", this->m_jsonDocument->GetAllocator());

        auto output = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        EXPECT_TRUE(input == output);
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

        ASSERT_EQ(Outcomes::DefaultsUsed, result.GetOutcome());
        EXPECT_TRUE(defaultValue == output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, LoadSave_Zero_SavesAndLoadsIdentityMatrix)
    {
        using namespace AZ::JsonSerializationResult;

        auto defaultValue = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateIdentity();
        auto input = JsonMathMatrixSerializerTests<TypeParam>::Descriptor::MatrixType::CreateZero();

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
        EXPECT_TRUE(defaultValue == output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, Load_InvalidFields_ReturnsUnsupportedAndLeavesMatrixUntouched)
    {
        using namespace AZ::JsonSerializationResult;
        using Descriptor = typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor;

        rapidjson::Value& objectValue = this->m_jsonDocument->SetObject();
        auto input = Descriptor::MatrixType::CreateIdentity();
        DataHelper::AddData(objectValue, input, this->m_jsonDocument->GetAllocator());
        objectValue["yaw"] = "Invalid";

        auto output = Descriptor::MatrixType::CreateZero();
        ResultCode result = this->m_serializer->Load(
            &output,
            azrtti_typeid<typename Descriptor::MatrixType>(),
            *this->m_jsonDocument,
            *this->m_jsonDeserializationContext);
        ASSERT_EQ(Outcomes::Unsupported, result.GetOutcome());
        EXPECT_TRUE(input == output);
    }

    TYPED_TEST(JsonMathMatrixSerializerTests, LoadSave_Arbitrary_SavesAndLoadsArbitraryMatrix)
    {
        using namespace AZ::JsonSerializationResult;
        using Descriptor = typename JsonMathMatrixSerializerTests<TypeParam>::Descriptor;

        auto defaultValue = Descriptor::MatrixType::CreateIdentity();
        size_t elementCount = Descriptor::RowCount * Descriptor::ColumnCount;
        auto input = DataHelper::CreateArbitraryMatrix<typename Descriptor::MatrixType>(elementCount);

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
