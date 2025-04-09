/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;

    //! N-dimensional vector class.
    class VectorN final
    {
    public:

        AZ_TYPE_INFO(VectorN, "{3C5A461A-3412-4D97-9CBC-856EE737B6DB}");

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        VectorN() = default;

        VectorN(AZStd::size_t numElements);

        VectorN(AZStd::size_t numElements, float x);

        VectorN(const VectorN& v) = default;

        VectorN(VectorN&& v);

        ~VectorN() = default;

        VectorN& operator=(VectorN&&) = default;

        VectorN& operator=(const VectorN&) = default;

        //! Creates a vector with all components set to zero, more efficient than calling VectorN(0.0f).
        static VectorN CreateZero(AZStd::size_t numElements);

        //! Creates a vector with all components set to one.
        static VectorN CreateOne(AZStd::size_t numElements);

        //! Creates a vector with all components set to the provided input values.
        static VectorN CreateFromFloats(AZStd::size_t numElements, const float* inputs);

        //! Creates a vector with all elements set to random numbers in the range [0, 1).
        static VectorN CreateRandom(AZStd::size_t numElements);

        //! Returns the dimensionality of the vector.
        AZStd::size_t GetDimensionality() const;

        //! Changes the dimensionality of the vector.
        void Resize(AZStd::size_t size);

        //! Returns the value at the requested index.
        float GetElement(AZStd::size_t index) const;

        //! Returns the value at the requested index.
        void SetElement(AZStd::size_t index, float value);

        //! Checks whether two vectors of equal dimensionality are equal to each other within a floating point tolerance.
        bool IsClose(const VectorN& v, float tolerance = 0.001f) const;

        //! Checks if the vector is a zero vector, within the provided tolerance for zero.
        bool IsZero(float tolerance = AZ::Constants::FloatEpsilon) const;

        //! Comparison functions, not implemented as operators since that would probably be a little dangerous.
        //! These functions return true only if all components pass the comparison test.
        //! @{
        bool IsLessThan(const VectorN& v) const;
        bool IsLessEqualThan(const VectorN& v) const;
        bool IsGreaterThan(const VectorN& v) const;
        bool IsGreaterEqualThan(const VectorN& v) const;
        //! @}

        //! Floor/Ceil/Round functions, operate on each component individually, result will be stored in output.
        //! @{
        VectorN GetFloor() const;
        VectorN GetCeil() const;
        VectorN GetRound() const; // Ties to even (banker's rounding)
        //! @}

        //! Min/Max functions, operate on each component individually, result will be stored in output.
        //! @{
        VectorN GetMin(const VectorN& v) const;
        VectorN GetMax(const VectorN& v) const;
        VectorN GetClamp(const VectorN& min, const VectorN& max) const;
        //! @}

        //! Returns L1 norm (Manhattan distance) of the vector, full accuracy.
        float L1Norm() const;

        //! Returns L2 norm (Euclidean distance) of the vector, full accuracy.
        float L2Norm() const;

        //! Returns normalized vector, full accuracy.
        VectorN GetNormalized() const;

        //! Normalizes the vector in-place, full accuracy.
        void Normalize();

        //! Returns a new VectorN containing the absolute value of all elements in the source VectorN.
        VectorN GetAbs() const;

        //! Absolute value in-place.
        void Absolute();

        //! Returns a new VectorN containing the square of all elements in the source VectorN.
        VectorN GetSquare() const;

        //! Square value in-place.
        void Square();

        //! Returns the dot product of two vectors of equal dimension.
        float Dot(const VectorN& rhs) const;

        //! Quickly zeros all elements of the vector to create a zero vector.
        void SetZero();

        VectorN& operator+=(const VectorN& rhs);
        VectorN& operator-=(const VectorN& rhs);
        VectorN& operator*=(const VectorN& rhs); //! Hadamard product, not dot product.
        VectorN& operator/=(const VectorN& rhs);

        VectorN& operator+=(float sum);
        VectorN& operator-=(float difference);
        VectorN& operator*=(float multiplier);
        VectorN& operator/=(float divisor);

        VectorN operator-() const;
        VectorN operator+(const VectorN& rhs) const;
        VectorN operator-(const VectorN& rhs) const;
        VectorN operator*(const VectorN& rhs) const; //! Hadamard product, not dot product.
        VectorN operator/(const VectorN& rhs) const;

        VectorN operator*(float multiplier) const;
        VectorN operator/(float divisor) const;

        //! Returns the raw Vector4's that represent this vector instance.
        const AZStd::vector<Vector4>& GetVectorValues() const;
        AZStd::vector<Vector4>& GetVectorValues();

        //! Zeros out unused components of the last vector element
        void FixLastVectorElement();

    private:

        //! Updates the vector internals to match the current dimensionality.
        void OnSizeChanged();

        AZStd::size_t m_numElements = 0;
        AZStd::vector<Vector4> m_values;
    };

    //! Operators that allow scalars as lhs operands.
    //! @{
    VectorN operator+(float lhs, const VectorN& rhs);
    VectorN operator-(float lhs, const VectorN& rhs);
    VectorN operator*(float lhs, const VectorN& rhs);
    //! @}
}

#include <AzCore/Math/VectorN.inl>
