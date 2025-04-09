/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

namespace AZ::RHI
{
    //! The intent of this class is to provide fast and thin access to the underlying constant
    //! data (inline or from an SRG), with basic validation to protect the user. As a secondary objective, it provides type-specific convenience
    //! operations as long as they don't violate the primary "fast" and "thin" objectives. To clarify, thin means
    //! we don't make assumptions about the data or how the user wants to operate on the data, and the convenience
    //! operations boil down to thin wrappers for single calls to SetConstantRaw and GetConstantRaw. So these
    //! convenience functions are provided in situations that are "low-hanging-fruit".
    class ConstantsData
    {
    public:
        ConstantsData() = default;
        explicit ConstantsData(const ConstantsLayout* layout);

        AZ_DEFAULT_COPY_MOVE(ConstantsData);

        //! Assigns constant data for the given constant shader input index.
        bool SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, size_t byteCount);
        bool SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, size_t byteOffset, size_t byteCount);

        //! Assigns a value of type T to the constant shader input.
        template <typename T>
        bool SetConstant(ShaderInputConstantIndex inputIndex, const T& value);

        //! Assigns a specified number of rows from a Matrix
        bool SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const Matrix3x3& value, uint32_t rowCount);
        bool SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const Matrix3x4& value, uint32_t rowCount);
        bool SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const Matrix4x4& value, uint32_t rowCount);

        //! Assigns a value of type T to the constant shader input, at an array offset.
        template <typename T>
        bool SetConstant(ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex);

        //! Assigns an array of type T to the constant shader input.
        template <typename T>
        bool SetConstantArray(ShaderInputConstantIndex inputIndex, AZStd::span<const T> values);

        //! Assigns constant data as a whole.
        bool SetConstantData(const void* bytes, size_t byteCount);
        bool SetConstantData(const void* bytes, size_t byteOffset, size_t byteCount);

        //! Returns constant data for the given shader input index as a template type.
        //! The stride of T must match the size of the constant input region. The number
        //! of elements in the returned array is the number of evenly divisible elements.
        //! If the strides do not match, an empty array is returned.
        template <typename T>
        AZStd::span<const T> GetConstantArray(ShaderInputConstantIndex inputIndex) const;

        //! Returns the constant data as type 'T' returned by value. The size of the constant region
        //! must match the size of T exactly. Otherwise, an empty instance is returned.
        template <typename T>
        T GetConstant(ShaderInputConstantIndex inputIndex) const;

        //! Treats the constant input as an array of type T, returning the element by value at the
        //! specified array index. The size of the constant region must equally partition into an
        //! array of type T. Otherwise, an empty instance is returned.
        template <typename T>
        T GetConstant(ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const;

        //! Returns constant data for the given shader input index as a span of bytes.
        AZStd::span<const uint8_t> GetConstantRaw(ShaderInputConstantIndex inputIndex) const;

        //! Returns the opaque constant data populated by calls to SetConstant and SetConstantData.
        AZStd::span<const uint8_t> GetConstantData() const;

        //! Returns the constants layout.
        const ConstantsLayout* GetLayout() const;

        //! Returns whether other constant data and this have the same value at the specified shader input index
        bool ConstantIsEqual(const ConstantsData& other, ShaderInputConstantIndex inputIndex) const;

        //! Performs a diff between this and input constant data and returns a list of all the shader input indices
        //! for which the constants are not the same between the two. If one of the two has more constants than the
        //! other, these additional constants will be added to the end of the returned list.
        AZStd::vector<ShaderInputConstantIndex> GetIndicesOfDifferingConstants(const ConstantsData& other) const;

    private:
        enum class ValidateConstantAccessExpect : uint32_t
        {
            //! Size of input is a complete mapping of the allocated region.
            Complete = 0,

            //! Size of input must be less than the size of the allocated region.
            LessThan,

            //! Treating the constant region as an array of elements, the provided byte offset / size must
            //! map exactly to an element in the array.
            ArrayElement,
        };

        bool ValidateConstantAccess(ShaderInputConstantIndex inputIndex, ValidateConstantAccessExpect expect, size_t offsetInBytes, size_t sizeInBytes) const;
        bool ValidateConstantBufferAccess(size_t offsetInBytes, size_t sizeInBytes) const;

        //! Assigns a specified number of rows from a Matrix of type Matrix3x4 and Matrix4x4
        //! the function expects type T and matrixSize (number of floats, which is 12 for Matrix3x4 and 16 for Matrix4x4)
        template <typename T, uint32_t matrixSize>
        bool SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount);

        ConstPtr<ConstantsLayout> m_layout;
        AZStd::vector<uint8_t> m_constantData;
    };

    template <typename T>
    bool ConstantsData::SetConstant(ShaderInputConstantIndex inputIndex, const T& value)
    {
        AZStd::span<const T> valueArray(&value, 1);
        return SetConstantArray(inputIndex, valueArray);
    }

    template <>
    bool ConstantsData::SetConstant<bool>(ShaderInputConstantIndex inputIndex, const bool& value);

    template <>
    bool ConstantsData::SetConstant<Matrix3x3>(ShaderInputConstantIndex inputIndex, const Matrix3x3& value);

    template <>
    bool ConstantsData::SetConstant<Matrix3x4>(ShaderInputConstantIndex inputIndex, const Matrix3x4& value);

    template <>
    bool ConstantsData::SetConstant<Matrix4x4>(ShaderInputConstantIndex inputIndex, const Matrix4x4& value);

    template <>
    bool ConstantsData::SetConstant<Vector2>(ShaderInputConstantIndex inputIndex, const Vector2& value);

    template <>
    bool ConstantsData::SetConstant<Vector3>(ShaderInputConstantIndex inputIndex, const Vector3& value);

    template <>
    bool ConstantsData::SetConstant<Vector4>(ShaderInputConstantIndex inputIndex, const Vector4& value);

    template <>
    bool ConstantsData::SetConstant<Color>(ShaderInputConstantIndex inputIndex, const Color& value);

    template <>
    bool ConstantsData::SetConstantArray<bool>(ShaderInputConstantIndex inputIndex, AZStd::span<const bool> values);

    template <>
    bool ConstantsData::GetConstant<bool>(ShaderInputConstantIndex inputIndex) const;

    template <>
    Matrix3x3 ConstantsData::GetConstant<Matrix3x3>(ShaderInputConstantIndex inputIndex) const;

    template <>
    Matrix3x4 ConstantsData::GetConstant<Matrix3x4>(ShaderInputConstantIndex inputIndex) const;

    template <>
    Matrix4x4 ConstantsData::GetConstant<Matrix4x4>(ShaderInputConstantIndex inputIndex) const;

    template <>
    Vector2 ConstantsData::GetConstant<Vector2>(ShaderInputConstantIndex inputIndex) const;

    template <>
    Vector3 ConstantsData::GetConstant<Vector3>(ShaderInputConstantIndex inputIndex) const;

    template <>
    Vector4 ConstantsData::GetConstant<Vector4>(ShaderInputConstantIndex inputIndex) const;

    template <>
    Color ConstantsData::GetConstant<Color>(ShaderInputConstantIndex inputIndex) const;

    template <typename T, uint32_t matrixSize>
    bool ConstantsData::SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount)
    {
        // Store the matrix into row major order and assigns the specified number of row from a Matrix. Make sure we
        // use the correct number of floats; matrixSize isn't guaranted to be a multiple of 4.
        // Important: we have to zero out this array since not all matrix row types will write all components.
        // Make sure to allow all the rows to be read, but not more than are expected.
        const size_t sizeInBytes = sizeof(float) * AZStd::min(matrixSize, rowCount * 4);
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            const Interval interval = GetLayout()->GetInterval(inputIndex);
            float* row = reinterpret_cast<float*>(&m_constantData[interval.m_min]);
            for (uint32_t i = 0; i < rowCount; i++)
            {
                value.GetRow(i).StoreToFloat4(row + i * 4);
            }

            return true;
        }
        return false;
    }

    template <typename T>
    bool ConstantsData::SetConstant(ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex)
    {
        const size_t sizeInBytes = sizeof(T);
        const size_t offsetInBytes = sizeInBytes * arrayIndex;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::ArrayElement, offsetInBytes, sizeInBytes))
        {
            return SetConstantRaw(inputIndex, &value, offsetInBytes, sizeInBytes);
        }
        return false;
    }

    template <typename T>
    bool ConstantsData::SetConstantArray(ShaderInputConstantIndex inputIndex, AZStd::span<const T> values)
    {
        const size_t sizeInBytes = values.size() * sizeof(T);
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            return SetConstantRaw(inputIndex, values.data(), sizeInBytes);
        }
        return false;
    }

    template <typename T>
    AZStd::span<const T> ConstantsData::GetConstantArray(ShaderInputConstantIndex inputIndex) const
    {
        AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
        const size_t elementSize = sizeof(T);
        const size_t elementCount = AZ::DivideAndRoundUp(constantBytes.size(), elementSize);
        const size_t sizeInBytes = elementCount * elementSize;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            return AZStd::span<const T>(reinterpret_cast<const T*>(constantBytes.data()), elementCount);
        }
        return {};
    }

    template <typename T>
    T ConstantsData::GetConstant(ShaderInputConstantIndex inputIndex) const
    {
        AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
        const size_t sizeInBytes = sizeof(T);
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::Complete, 0, sizeInBytes))
        {
            return *reinterpret_cast<const T*>(constantBytes.data());
        }
        return T();
    }

    template <typename T>
    T ConstantsData::GetConstant(ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const
    {
        AZStd::span<const uint8_t> constantBytes = GetConstantRaw(inputIndex);
        const size_t elementSize = sizeof(T);
        const size_t elementOffset = arrayIndex * elementSize;
        if (ValidateConstantAccess(inputIndex, ValidateConstantAccessExpect::ArrayElement, elementOffset, elementSize))
        {
            return *reinterpret_cast<const T*>(&constantBytes[elementOffset]);
        }
        return {};
    }
}
