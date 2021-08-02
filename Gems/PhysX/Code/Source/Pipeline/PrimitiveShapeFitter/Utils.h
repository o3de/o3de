/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/array.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>

namespace PhysX::Pipeline
{
    // Double precision constants. The equivalents defined in AZ::Constants are single precision.
    static constexpr double OneHalfPi = 1.57079632679489662;
    static constexpr double FourThirdsPi = 4.18879020478639098;

    //! Type definition for double-precision vectors used by the primitive fitting routine.
    //! The numerical methods gem, which the fitter heavily relies on, uses double precision throughout.
    using Vector = AZStd::array<double, 3>;

    //! Vector addition operator.
    Vector operator+(const Vector& lhs, const Vector& rhs);

    //! Vector subtraction operator.
    Vector operator-(const Vector& lhs, const Vector& rhs);

    //! Vector scalar multiplication operator.
    Vector operator*(const Vector& vector, const double scalar);

    //! Vector scalar multiplication operator.
    Vector operator*(const double scalar, const Vector& vector);

    //! Vector scalar division operator.
    Vector operator/(const Vector& vector, const double scalar);

    //! Vector cross product operator.
    Vector Cross(const Vector& lhs, const Vector& rhs);

    //! Vector dot product operator.
    double Dot(const Vector& lhs, const Vector& rhs);

    //! Compute the squared length of a vector.
    double NormSquared(const Vector& vector);

    //! Compute the length of a vector.
    double Norm(const Vector& vector);

    //! Compute a vector that is orthogonal to the one passed as an argument.
    Vector ComputeAnyOrthogonalVector(const Vector& vector);

    //! Convert a vector to an equivalent \ref AZ::Vector3 instance.
    //! This function will downcast the individual components to single precision floats.
    AZ::Vector3 VecToAZVec3(const Vector& vector);

    //! Create a \ref AZ::Transform for a coordinate system specified by an origin and three basis vectors.
    AZ::Transform CreateTransformFromCoordinateSystem(
        const Vector& origin,
        const Vector& xAxis,
        const Vector& yAxis,
        const Vector& zAxis
    );

    //! Convert three basis vectors to their corresponding XYZ Euler angles.
    Vector RotationMatrixToEulerAngles(const Vector& xAxis, const Vector& yAxis, const Vector& zAxis);

    //! Extract the basis vector along the x-axis from the given XYZ Euler angles.
    Vector EulerAnglesToBasisX(const Vector& theta);

    //! Extract the basis vector along the y-axis from the given XYZ Euler angles.
    Vector EulerAnglesToBasisY(const Vector& theta);

    //! Extract the basis vector along the z-axis from the given XYZ Euler angles.
    Vector EulerAnglesToBasisZ(const Vector& theta);

    //! Check whether the absolute value of a number is within a given threshold.
    bool IsAbsoluteValueWithinEpsilon(double value, const double epsilon = 1.0e-30);

    //! Test that two numbers are non-zero and that the absolute value of their ratio is no less than a given threshold.
    bool IsAbsolueValueRatioWithinThreshold(double valueOne, double valueTwo, const double threshold = 0.005);
} // PhysX::Pipeline
