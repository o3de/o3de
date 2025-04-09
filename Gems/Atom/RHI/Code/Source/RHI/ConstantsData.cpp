/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ConstantsData.h>

namespace AZ::RHI
{
    ConstantsData::ConstantsData(const ConstantsLayout* rootConstantLayout)
        : m_layout(rootConstantLayout)
    {
        AZ_Assert(m_layout, "Null ConstantsLayout");
        if (m_layout->GetDataSize() > 0)
        {
            m_constantData.resize(m_layout->GetDataSize());
        }
    }

    bool ConstantsData::ValidateConstantAccess(ShaderInputConstantIndex inputIndex, ValidateConstantAccessExpect expect, size_t offsetInBytes, size_t sizeInBytes) const
    {
        if (!Validation::IsEnabled())
        {
            return true;
        }
        if (!GetLayout()->ValidateAccess(inputIndex))
        {
            return false;
        }

        const ShaderInputConstantDescriptor& shaderInputConstant = GetLayout()->GetShaderInput(inputIndex);
        switch (expect)
        {
        case ValidateConstantAccessExpect::Complete:
            // The input offset / size must span the entire allocated constant region.
            if (offsetInBytes != 0 || sizeInBytes != shaderInputConstant.m_constantByteCount)
            {
                AZ_Assert(false,
                    "Constant Input '%s': This method requires that the full allocated constant range be accessed. The "
                    "requested offset must be 0, and the requested size must be the full size of the constant shader input. "
                    "Expected Range: [0, %d). Actual Range: [%d, %d).", shaderInputConstant.m_name.GetCStr(), shaderInputConstant.m_constantByteCount, offsetInBytes, offsetInBytes + sizeInBytes);
                return false;
            }
            break;

        case ValidateConstantAccessExpect::LessThan:
            // The input offset + size must be less than the total bytes allocated for this constant input.
            if ((offsetInBytes + sizeInBytes) > shaderInputConstant.m_constantByteCount)
            {
                AZ_Assert(false,
                    "Constant Input '%s': The requested region of constant data exceeds the allocated size of the constant shader input. "
                    "Total Bytes: %d Actual: [%d, %d).", shaderInputConstant.m_name.GetCStr(), shaderInputConstant.m_constantByteCount, offsetInBytes, offsetInBytes + sizeInBytes);
                return false;
            }
            break;

        case ValidateConstantAccessExpect::ArrayElement:
            // Test bounds first.
            if (!ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::LessThan, offsetInBytes, sizeInBytes))
            {
                return false;
            }

            // Test that the offset / size map completely to an array index in an array of elements.
            if (!IsDivisible<size_t>(shaderInputConstant.m_constantByteCount, sizeInBytes) ||
                !IsDivisible(offsetInBytes, sizeInBytes))
            {
                AZ_Assert(false,
                    "Constant Input '%s': The requested region of constant data does not map to an array of elements of size '%d' "
                    "Total Bytes: %d Actual: [%d, %d)", shaderInputConstant.m_name.GetCStr(), sizeInBytes, shaderInputConstant.m_constantByteCount, offsetInBytes, offsetInBytes + sizeInBytes);
                return false;
            }
            break;
        }

        return true;
    }

    bool ConstantsData::ValidateConstantBufferAccess(size_t offsetInBytes, size_t sizeInBytes) const
    {
        if (Validation::IsEnabled())
        {
            if ((offsetInBytes + sizeInBytes) > m_constantData.size())
            {
                AZ_Error("ConstantsData", false, "Exceeded size declared for constant buffer.");
                return false;
            }
        }
        return true;
    }

    bool ConstantsData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, size_t byteCount)
    {
        return SetConstantRaw(inputIndex, bytes, 0, byteCount);
    }

    bool ConstantsData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, size_t byteOffset, size_t byteCount)
    {
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::LessThan, byteOffset, byteCount))
        {
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            memcpy(&m_constantData[interval.m_min + byteOffset], bytes, byteCount);
            return true;
        }
        return false;
    }

    bool ConstantsData::SetConstantData(const void* bytes, size_t byteCount)
    {
        if (ValidateConstantBufferAccess(0, byteCount))
        {
            memcpy(m_constantData.data(), bytes, byteCount);
            return true;
        }
        return false;
    }

    bool ConstantsData::SetConstantData(const void* bytes, size_t byteOffset, size_t byteCount)
    {
        if (ValidateConstantBufferAccess(byteOffset, byteCount))
        {
            memcpy(&m_constantData[byteOffset], bytes, byteCount);
            return true;
        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstant<bool>(ShaderInputConstantIndex inputIndex, const bool& value)
    {
        // The shader packs type bool as 4 bytes
        const size_t sizeInBytes = 4;
        const uint32_t fourByteValue = value ? 1 : 0;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            return SetConstantRaw(inputIndex, &fourByteValue, sizeInBytes);
        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstantArray<bool>(ShaderInputConstantIndex inputIndex, AZStd::span<const bool> values)
    {
        // The shader packs type bool as 4 bytes
        const size_t elementSize = 4;
        const size_t sizeInBytes = values.size() * elementSize;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            bool isValidAll = true;
            uint32_t offset = 0;

            for (size_t i = 0; i < values.size(); i++)
            {
                const uint32_t fourByteValue = values[i] ? 1 : 0;
                const bool isValid = SetConstantRaw(inputIndex, &fourByteValue, offset, elementSize);

                isValidAll &= isValid;
                offset += elementSize;
            }
            return isValidAll;
        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstant<Matrix3x3>(ShaderInputConstantIndex inputIndex, const Matrix3x3& value)
    {
        // DirectX packing rules pack a float3x3 as such:
        //    [0, 0], [0, 1], [0, 2], <padding>,
        //    [1, 0], [1, 1], [1, 2], <padding>,
        //    [2, 0], [2, 1], [2, 2]
        // It's important to note that the last row is stored in 3 floats, not 4.

        // Packing rules put this at 11 floats (9 data, 2 padding).
        const size_t sizeInBytes = sizeof(float) * 11;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            // Store the matrix into row major order
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* matrixValue = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            value.StoreToRowMajorFloat11(matrixValue);

            return true;
        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstant<Matrix3x4>(ShaderInputConstantIndex inputIndex, const Matrix3x4& value)
    {
        const size_t sizeInBytes = sizeof(Matrix3x4);
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, aznumeric_caster(sizeInBytes)))
        {
            // Store the matrix into row major order
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* matrixValue = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            value.StoreToRowMajorFloat12(matrixValue);

            return true;
        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstant<Matrix4x4>(ShaderInputConstantIndex inputIndex, const Matrix4x4& value)
    {
        const size_t sizeInBytes = sizeof(float) * 16;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            // Store the matrix into row major order
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* matrixValue = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            value.StoreToRowMajorFloat16(matrixValue);

            return true;

        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstant<Vector2>(ShaderInputConstantIndex inputIndex, const Vector2& value)
    {
        constexpr size_t sizeOfVector2 = 8;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, aznumeric_caster(sizeOfVector2)))
        {
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* vectorValue = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            value.StoreToFloat2(vectorValue);

            return true;
        }
        return false;
    }

    // [GFX TODO][ATOM-1869] Fix it for the other types than Vector3
    template <>
    bool ConstantsData::SetConstant<Vector3>(ShaderInputConstantIndex inputIndex, const Vector3& value)
    {
        const size_t sizeInBytes = sizeof(float) * 3;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* vectorValue = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            value.StoreToFloat3(vectorValue);

            return true;
        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstant<Vector4>(ShaderInputConstantIndex inputIndex, const Vector4& value)
    {
        constexpr size_t sizeOfVector4 = 16;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, aznumeric_caster(sizeOfVector4)))
        {
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* vectorValue = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            value.StoreToFloat4(vectorValue);

            return true;
        }
        return false;
    }

    template <>
    bool ConstantsData::SetConstant<Color>(ShaderInputConstantIndex inputIndex, const Color& value)
    {
        constexpr size_t sizeOfColor = sizeof(Color);
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, aznumeric_caster(sizeOfColor)))
        {
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* vectorValue = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            value.StoreToFloat4(vectorValue);

            return true;
        }
        return false;
    }

    bool ConstantsData::SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const Matrix3x3& value, uint32_t rowCount)
    {
        // See the packing comments in ConstantsData::SetConstant<Matrix3x3> for an explanation of why we only use
        // 11 float values here.
        const Matrix3x4& transform = Matrix3x4::CreateFromMatrix3x3(value);
        return SetConstantMatrixRows<Matrix3x4, 11>(inputIndex, transform, rowCount);
    }

    bool ConstantsData::SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const Matrix3x4& value, uint32_t rowCount)
    {
        return SetConstantMatrixRows<Matrix3x4, 12>(inputIndex, value, rowCount);
    }

    bool ConstantsData::SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const Matrix4x4& value, uint32_t rowCount)
    {
        return SetConstantMatrixRows<Matrix4x4, 16>(inputIndex, value, rowCount);
    }

    template <>
    bool ConstantsData::GetConstant<bool>(ShaderInputConstantIndex inputIndex) const
    {
        AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
        // The shader packs bool data as 4 bytes 
        const size_t sizeInBytes = 4;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            return *reinterpret_cast<const bool*>(constantBytes.data());
        }
        return bool();
    }

    template <>
    Matrix3x3 ConstantsData::GetConstant<Matrix3x3>(ShaderInputConstantIndex inputIndex) const
    {
        // See the packing comments in ConstantsData::SetConstant<Matrix3x3> for an explanation of why we only use
        // 11 float values here.
        const size_t sizeInBytes = sizeof(float) * 11;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            const AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);

            // As per shader packing rules the Matrix3x3 is stored as 11 floats (2 are padding).
            float localData[12];
            memcpy(localData, constantBytes.data(), sizeInBytes);
            localData[11] = 0.0f;

            const Matrix3x4 readAsTransform = Matrix3x4::CreateFromRowMajorFloat12(localData);
            const Matrix3x3 resultMatrix = Matrix3x3::CreateFromMatrix3x4(readAsTransform);
            return resultMatrix;
        }
        return Matrix3x3();
    }

    template <>
    Matrix3x4 ConstantsData::GetConstant<Matrix3x4>(ShaderInputConstantIndex inputIndex) const
    {
        const uint32_t sizeInBytes = sizeof(Matrix3x4);
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            const AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
            const Matrix3x4& resultMatrix = Matrix3x4::CreateFromRowMajorFloat12(reinterpret_cast<const float*>(constantBytes.data()));
            return resultMatrix;
        }
        return Matrix3x4();
    }

    template <>
    Matrix4x4 ConstantsData::GetConstant<Matrix4x4>(ShaderInputConstantIndex inputIndex) const
    {
        const size_t sizeInBytes = sizeof(float) * 16;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            const AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
            const Matrix4x4& resultMatrix = Matrix4x4::CreateFromRowMajorFloat16(reinterpret_cast<const float*>(constantBytes.data()));
            return resultMatrix;
        }
        return Matrix4x4();
    }

    template <>
    Vector2 ConstantsData::GetConstant<Vector2>(ShaderInputConstantIndex inputIndex) const
    {
        constexpr size_t vector2Size = 8;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, aznumeric_caster(vector2Size)))
        {
            AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
            return Vector2::CreateFromFloat2(reinterpret_cast<const float*>(constantBytes.data()));
        }
        return Vector2();
    }

    // [GFX TODO][ATOM-1869] Fix it for the other types than Vector3
    template <>
    Vector3 ConstantsData::GetConstant<Vector3>(ShaderInputConstantIndex inputIndex) const
    {
        constexpr size_t vector3Size = sizeof(float) * 3;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, vector3Size))
        {
            AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
            return Vector3::CreateFromFloat3(reinterpret_cast<const float*>(constantBytes.data()));
        }
        return Vector3();
    }

    template <>
    Vector4 ConstantsData::GetConstant<Vector4>(ShaderInputConstantIndex inputIndex) const
    {
        constexpr size_t vector4Size = 16;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, aznumeric_caster(vector4Size)))
        {
            AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
            return Vector4::CreateFromFloat4(reinterpret_cast<const float*>(constantBytes.data()));
        }
        return Vector4();
    }

    template <>
    Color ConstantsData::GetConstant<Color>(ShaderInputConstantIndex inputIndex) const
    {
        constexpr size_t colorSize = sizeof(Color);
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, aznumeric_caster(colorSize)))
        {
            AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
            return Color::CreateFromFloat4(reinterpret_cast<const float*>(constantBytes.data()));
        }
        return Color();
    }

    AZStd::span<const uint8_t> ConstantsData::GetConstantRaw(ShaderInputConstantIndex inputIndex) const
    {
        const Interval interval = GetLayout()->GetInterval(inputIndex);
        return AZStd::span<const uint8_t>(&m_constantData[interval.m_min], interval.m_max - interval.m_min);
    }

    AZStd::span<const uint8_t> ConstantsData::GetConstantData() const
    {
        return m_constantData;
    }

    const ConstantsLayout* ConstantsData::GetLayout() const
    {
        AZ_Assert(m_layout, "Constants layout is null");
        return m_layout.get();
    }

    bool ConstantsData::ConstantIsEqual(const ConstantsData& other, ShaderInputConstantIndex inputIndex) const
    {
        AZStd::span<const uint8_t> myConstant = GetConstantRaw(inputIndex);
        AZStd::span<const uint8_t> otherConstant = other.GetConstantRaw(inputIndex);

        // If they point to the same data, they are equal
        if (myConstant.data() == otherConstant.data() && myConstant.size() == otherConstant.size())
        {
            return true;
        }

        // If they point to data of different size, they are not equal
        if (myConstant.size() != otherConstant.size())
        {
            return false;
        }

        // If they point to differing data of same size, compare the data
        // Note: due to small size of data this loop will be faster than a mem compare
        for(uint32_t i = 0; i < myConstant.size(); ++i)
        {
            if (myConstant[i] != otherConstant[i])
            {
                return false;
            }
        }

        // Arrays point to different locations in memory but all bytes match, return true
        return true;
    }

    AZStd::vector<ShaderInputConstantIndex> ConstantsData::GetIndicesOfDifferingConstants(const ConstantsData& other) const
    {
        AZStd::vector<ShaderInputConstantIndex> differingIndices;

        if (m_layout == nullptr || other.m_layout == nullptr)
        {
            return differingIndices;
        }

        AZStd::span<const ShaderInputConstantDescriptor> myShaderInputs = m_layout->GetShaderInputList();
        AZStd::span<const ShaderInputConstantDescriptor> otherShaderInputs = other.m_layout->GetShaderInputList();

        size_t minSize = AZStd::min(myShaderInputs.size(), otherShaderInputs.size());
        size_t maxSize = AZStd::max(myShaderInputs.size(), otherShaderInputs.size());

        for (size_t idx = 0; idx < minSize; ++idx)
        {
            const ShaderInputConstantIndex inputIndex(idx);
            if (!ConstantIsEqual(other, inputIndex))
            {
                differingIndices.push_back(inputIndex);
            }
        }

        // If sizes are different, add difference at the end
        for (size_t idx = minSize; idx < maxSize; ++idx)
        {
            const ShaderInputConstantIndex inputIndex(idx);
            differingIndices.push_back(inputIndex);
        }

        return differingIndices;
    }
}
