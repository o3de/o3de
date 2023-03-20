/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathMatrixSerializer.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/osstring.h>
#include <AzCore/Casting/numeric_cast.h>

namespace AZ::JsonMathMatrixSerializerInternal
{
    template<typename MatrixType, size_t RowCount, size_t ColumnCount>
    JsonSerializationResult::Result LoadArray(MatrixType& output, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        constexpr size_t ElementCount = RowCount * ColumnCount;
        static_assert(ElementCount == 9 || ElementCount == 12 || ElementCount == 16,
            "MathMatrixSerializer only support Matrix3x3, Matrix3x4 and Matrix4x4.");

        rapidjson::SizeType arraySize = inputValue.Size();
        if (arraySize < ElementCount)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Not enough numbers in JSON array to load math matrix from.");
        }

        AZ::BaseJsonSerializer* floatSerializer = context.GetRegistrationContext()->GetSerializerForType(azrtti_typeid<float>());
        if (!floatSerializer)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "Failed to find the JSON float serializer.");
        }

        constexpr const char* names[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"};
        float values[ElementCount];
        for (int i = 0; i < ElementCount; ++i)
        {
            ScopedContextPath subPath(context, names[i]);
            JSR::Result intermediate = floatSerializer->Load(values + i, azrtti_typeid<float>(), inputValue[i], context);
            if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
            {
                return intermediate;
            }
        }

        size_t valueIndex = 0;
        for (size_t r = 0; r < RowCount; ++r)
        {
            for (size_t c = 0; c < ColumnCount; ++c)
            {
                output.SetElement(aznumeric_caster(r), aznumeric_caster(c), values[valueIndex++]);
            }
        }

        return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Success, "Successfully read math matrix.");
    }

    JsonSerializationResult::Result LoadFloatFromObject(
        float& output,
        const rapidjson::Value& inputValue,
        JsonDeserializerContext& context,
        const char* name,
        const char* altName)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        AZ::BaseJsonSerializer* floatSerializer = context.GetRegistrationContext()->GetSerializerForType(azrtti_typeid<float>());
        if (!floatSerializer)
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Catastrophic, "Failed to find the json float serializer.");
        }

        const char* nameUsed = name;
        JSR::ResultCode result(JSR::Tasks::ReadField);
        auto iterator = inputValue.FindMember(rapidjson::StringRef(name));
        if (iterator == inputValue.MemberEnd())
        {
            nameUsed = altName;
            iterator = inputValue.FindMember(rapidjson::StringRef(altName));
            if (iterator == inputValue.MemberEnd())
            {
                // field not found so leave default value
                result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed));
                nameUsed = nullptr;
            }
        }

        if (nameUsed)
        {
            ScopedContextPath subPath(context, nameUsed);
            JSR::Result intermediate = floatSerializer->Load(&output, azrtti_typeid<float>(), iterator->value, context);
            if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
            {
                return intermediate;
            }
            else
            {
                result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success));
            }
        }

        return context.Report(result, "Successfully read float.");
    }

    JsonSerializationResult::Result LoadVector3FromObject(
        Vector3& output,
        const rapidjson::Value& inputValue,
        JsonDeserializerContext& context,
        AZStd::fixed_vector<AZStd::string_view, 6> names)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.
        constexpr size_t ElementCount = 3; // Vector3

        JSR::ResultCode result(JSR::Tasks::ReadField);
        float values[ElementCount];
        for (int i = 0; i < ElementCount; ++i)
        {
            values[i] = output.GetElement(i);
            auto name = names[i * 2];
            auto altName = names[(i * 2) + 1];

            JSR::Result intermediate = LoadFloatFromObject(values[i], inputValue, context, name.data(), altName.data());
            if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
            {
                return intermediate;
            }
            else
            {
                result.Combine(JSR::ResultCode(JSR::Tasks::ReadField, JSR::Outcomes::Success));
            }
        }

        for (int i = 0; i < ElementCount; ++i)
        {
            output.SetElement(i, values[i]);
        }

        return context.Report(result, "Successfully read math matrix.");
    }

    JsonSerializationResult::Result LoadQuaternionAndScale(
        AZ::Quaternion& quaternion,
        float& scale,
        const rapidjson::Value& inputValue,
        JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        JSR::ResultCode result(JSR::Tasks::ReadField);
        scale = 1.0f;
        JSR::Result intermediateScale = LoadFloatFromObject(scale, inputValue, context, "scale", "Scale");
        if (intermediateScale.GetResultCode().GetProcessing() != JSR::Processing::Completed)
        {
            return intermediateScale;
        }
        result.Combine(intermediateScale);

        if (AZ::IsClose(scale, 0.0f))
        {
            result.Combine({ JSR::Tasks::ReadField, JSR::Outcomes::Unsupported });
            return context.Report(result, "Scale can not be zero.");
        }

        AZ::Vector3 degreesRollPitchYaw = AZ::Vector3::CreateZero();
        JSR::Result intermediateDegrees = LoadVector3FromObject(degreesRollPitchYaw, inputValue, context, { "roll", "Roll", "pitch", "Pitch", "yaw", "Yaw" });
        if (intermediateDegrees.GetResultCode().GetProcessing() != JSR::Processing::Completed)
        {
            return intermediateDegrees;
        }
        result.Combine(intermediateDegrees);

        // the quaternion should be equivalent to a series of rotations in the order z, then y, then x
        const AZ::Vector3 eulerRadians = AZ::Vector3DegToRad(degreesRollPitchYaw);
        quaternion = AZ::Quaternion::CreateRotationX(eulerRadians.GetX()) *
                     AZ::Quaternion::CreateRotationY(eulerRadians.GetY()) *
                     AZ::Quaternion::CreateRotationZ(eulerRadians.GetZ());

        return context.Report(result, "Successfully read math yaw, pitch, roll, and scale.");
    }

    template<typename MatrixType>
    JsonSerializationResult::Result LoadObject(MatrixType& output, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.
        output = MatrixType::CreateIdentity();

        JSR::ResultCode result(JSR::Tasks::ReadField);
        float scale;
        AZ::Quaternion rotation;

        JSR::Result intermediate = LoadQuaternionAndScale(rotation, scale, inputValue, context);
        if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
        {
            return intermediate;
        }
        result.Combine(intermediate);

        AZ::Vector3 translation = AZ::Vector3::CreateZero();
        JSR::Result intermediateTranslation = LoadVector3FromObject(translation, inputValue, context, { "x", "X", "y", "Y", "z", "Z" });
        if (intermediateTranslation.GetResultCode().GetProcessing() != JSR::Processing::Completed)
        {
            return intermediateTranslation;
        }
        result.Combine(intermediateTranslation);

        // composed a matrix by rotation, then scale, then translation
        auto matrix = MatrixType::CreateFromQuaternion(rotation);
        matrix.MultiplyByScale(Vector3{ scale });
        matrix.SetTranslation(translation);

        if (matrix == MatrixType::CreateIdentity())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Using identity matrix for empty object.");
        }

        output = matrix;
        return context.Report(result, "Successfully read math matrix.");
    }

    template<>
    JsonSerializationResult::Result LoadObject<Matrix3x3>(Matrix3x3& output, const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.
        output = Matrix3x3::CreateIdentity();

        JSR::ResultCode result(JSR::Tasks::ReadField);
        float scale;
        AZ::Quaternion rotation;

        JSR::Result intermediate = LoadQuaternionAndScale(rotation, scale, inputValue, context);
        if (intermediate.GetResultCode().GetProcessing() != JSR::Processing::Completed)
        {
            return intermediate;
        }
        result.Combine(intermediate);

        // composed a matrix by rotation then scale
        auto matrix = Matrix3x3::CreateFromQuaternion(rotation);
        matrix.MultiplyByScale(Vector3{ scale });

        if (matrix == Matrix3x3::CreateIdentity())
        {
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Using identity matrix for empty object.");
        }

        output = matrix;
        return context.Report(result, "Successfully read math matrix.");
    }

    template<typename MatrixType, size_t RowCount, size_t ColumnCount>
    JsonSerializationResult::Result Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context, bool isExplicitDefault)
    {
        namespace JSR = JsonSerializationResult; // Used remove name conflicts in AzCore in uber builds.

        constexpr size_t ElementCount = RowCount * ColumnCount;
        static_assert(ElementCount == 9 || ElementCount == 12 || ElementCount == 16,
            "MathMatrixSerializer only support Matrix3x3, Matrix3x4 and Matrix4x4.");

        AZ_Assert(azrtti_typeid<MatrixType>() == outputValueTypeId,
            "Unable to deserialize Matrix%zux%zu to json because the provided type is %s",
            RowCount, ColumnCount, outputValueTypeId.ToString<OSString>().c_str());
        AZ_UNUSED(outputValueTypeId);

        MatrixType* matrix = reinterpret_cast<MatrixType*>(outputValue);
        AZ_Assert(matrix, "Output value for JsonMatrix%zux%zuSerializer can't be null.", RowCount, ColumnCount);

        if (isExplicitDefault)
        {
            *matrix = MatrixType::CreateIdentity();
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::DefaultsUsed, "Matrix value set to identity matrix.");
        }

        switch (inputValue.GetType())
        {
        case rapidjson::kArrayType:
            return LoadArray<MatrixType, RowCount, ColumnCount>(*matrix, inputValue, context);
        case rapidjson::kObjectType:
            return LoadObject<MatrixType>(*matrix, inputValue, context);

        case rapidjson::kStringType:
            [[fallthrough]];
        case rapidjson::kNumberType:
            [[fallthrough]];
        case rapidjson::kNullType:
            [[fallthrough]];
        case rapidjson::kFalseType:
            [[fallthrough]];
        case rapidjson::kTrueType:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unsupported,
                "Unsupported type. Math matrix can only be read from arrays or objects.");

        default:
            return context.Report(JSR::Tasks::ReadField, JSR::Outcomes::Unknown,
                "Unknown json type encountered in math matrix.");
        }
    }

    template<typename MatrixType, size_t RowCount, size_t ColumnCount>
    JsonSerializationResult::Result StoreArray(rapidjson::Value& outputValue, const void* inputValue, const void* defaultValue,
        [[maybe_unused]] const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        namespace JSR = JsonSerializationResult; // Used to remove name conflicts in AzCore in uber builds.

        constexpr size_t ElementCount = RowCount * ColumnCount;
        static_assert(ElementCount == 9 || ElementCount == 12 || ElementCount == 16,
            "MathMatrixSerializer only support Matrix3x3, Matrix3x4 and Matrix4x4.");

        const MatrixType* matrix = reinterpret_cast<const MatrixType*>(inputValue);
        AZ_Assert(matrix, "Input value for JsonMatrix%zux%zuSerializer can't be null.", RowCount, ColumnCount);
        const MatrixType* defaultMatrix = reinterpret_cast<const MatrixType*>(defaultValue);

        if (!context.ShouldKeepDefaults() && defaultMatrix && *matrix == *defaultMatrix)
        {
            return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::DefaultsUsed, "Default math Matrix used.");
        }

        outputValue.SetArray();
        for (int rowIndex = 0; rowIndex < RowCount; ++rowIndex)
        {
            for (int columnIndex = 0; columnIndex < ColumnCount; ++columnIndex)
            {
                outputValue.PushBack(matrix->GetElement(rowIndex, columnIndex), context.GetJsonAllocator());
            }
        }

        return context.Report(JSR::Tasks::WriteValue, JSR::Outcomes::Success, "Math Matrix successfully stored.");
    }
}

namespace AZ
{
    // BaseJsonMatrixSerializer

    AZ_CLASS_ALLOCATOR_IMPL(BaseJsonMatrixSerializer, SystemAllocator);

    auto BaseJsonMatrixSerializer::GetOperationsFlags() const -> OperationFlags
    {
        return OperationFlags::InitializeNewInstance;
    }


    // Matrix3x3

    AZ_CLASS_ALLOCATOR_IMPL(JsonMatrix3x3Serializer, SystemAllocator);

    JsonSerializationResult::Result JsonMatrix3x3Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        return JsonMathMatrixSerializerInternal::Load<Matrix3x3, 3, 3>(
            outputValue, outputValueTypeId, inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonMatrix3x3Serializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        outputValue.SetObject();

        return JsonMathMatrixSerializerInternal::StoreArray<Matrix3x3, 3, 3>(
            outputValue, inputValue, defaultValue, valueTypeId, context);
    }


    // Matrix3x4

    AZ_CLASS_ALLOCATOR_IMPL(JsonMatrix3x4Serializer, SystemAllocator);

    JsonSerializationResult::Result JsonMatrix3x4Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        return JsonMathMatrixSerializerInternal::Load<Matrix3x4, 3, 4>(
            outputValue, outputValueTypeId, inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonMatrix3x4Serializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        outputValue.SetObject();

        return JsonMathMatrixSerializerInternal::StoreArray<Matrix3x4, 3, 4>(
            outputValue, inputValue, defaultValue, valueTypeId, context);
    }

    // Matrix4x4

    AZ_CLASS_ALLOCATOR_IMPL(JsonMatrix4x4Serializer, SystemAllocator);

    JsonSerializationResult::Result JsonMatrix4x4Serializer::Load(void* outputValue, const Uuid& outputValueTypeId,
        const rapidjson::Value& inputValue, JsonDeserializerContext& context)
    {
        return JsonMathMatrixSerializerInternal::Load<Matrix4x4, 4, 4>(
            outputValue, outputValueTypeId, inputValue, context, IsExplicitDefault(inputValue));
    }

    JsonSerializationResult::Result JsonMatrix4x4Serializer::Store(rapidjson::Value& outputValue, const void* inputValue,
        const void* defaultValue, const Uuid& valueTypeId, JsonSerializerContext& context)
    {
        outputValue.SetObject();

        return JsonMathMatrixSerializerInternal::StoreArray<Matrix4x4, 4, 4>(
            outputValue, inputValue, defaultValue, valueTypeId, context);
    }
}
