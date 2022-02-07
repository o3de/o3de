/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x3.h>

#include <Source/Pipeline/PrimitiveShapeFitter/Utils.h>

namespace PhysX::Pipeline
{
    Vector operator+(const Vector& lhs, const Vector& rhs)
    {
        return Vector{{
            lhs[0] + rhs[0],
            lhs[1] + rhs[1],
            lhs[2] + rhs[2]
        }};
    }

    Vector operator-(const Vector& lhs, const Vector& rhs)
    {
        return Vector{{
            lhs[0] - rhs[0],
            lhs[1] - rhs[1],
            lhs[2] - rhs[2]
        }};
    }

    Vector operator*(const Vector& vector, const double scalar)
    {
        return Vector{{
            vector[0] * scalar,
            vector[1] * scalar,
            vector[2] * scalar
        }};
    }

    Vector operator*(const double scalar, const Vector& vector)
    {
        return vector * scalar;
    }

    Vector operator/(const Vector& vector, const double scalar)
    {
        return Vector{{
            vector[0] / scalar,
            vector[1] / scalar,
            vector[2] / scalar
        }};
    }

    Vector Cross(const Vector& lhs, const Vector& rhs)
    {
        return Vector{{
            lhs[1] * rhs[2] - lhs[2] * rhs[1],
            lhs[2] * rhs[0] - lhs[0] * rhs[2],
            lhs[0] * rhs[1] - lhs[1] * rhs[0]
        }};
    }

    double Dot(const Vector& lhs, const Vector& rhs)
    {
        return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
    }

    double NormSquared(const Vector& vector)
    {
        return Dot(vector, vector);
    }

    double Norm(const Vector& vector)
    {
        return sqrt(NormSquared(vector));
    }

    Vector ComputeAnyOrthogonalVector(const Vector& vector)
    {
        double invLength = 1.0;

        if (fabs(vector[0]) > fabs(vector[1]))
        {
            invLength /= sqrt(vector[0] * vector[0] + vector[2] * vector[2]);
            return Vector{{ -vector[2] * invLength, 0.0, vector[0] * invLength }};
        }
        else
        {
            invLength /= sqrt(vector[1] * vector[1] + vector[2] * vector[2]);
            return Vector{{ 0.0, vector[2] * invLength, -vector[1] * invLength }};
        }
    }

    AZ::Vector3 VecToAZVec3(const Vector& vector)
    {
        return AZ::Vector3((float)vector[0], (float)vector[1], (float)vector[2]);
    }

    AZ::Transform CreateTransformFromCoordinateSystem(
        const Vector& origin,
        const Vector& xAxis,
        const Vector& yAxis,
        const Vector& zAxis
    )
    {
        return AZ::Transform::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateFromColumns(
                VecToAZVec3(xAxis),
                VecToAZVec3(yAxis),
                VecToAZVec3(zAxis)),
            VecToAZVec3(origin)
        );
    }

    Vector RotationMatrixToEulerAngles(const Vector& xAxis, const Vector& yAxis, const Vector& zAxis)
    {
        Vector theta{{0.0, 0.0, 0.0}};

        if (zAxis[0] < 1.0)
        {
            if (zAxis[0] > -1.0)
            {
                theta[1] = asin(zAxis[0]);
                theta[0] = atan2(-zAxis[1], zAxis[2]);
                theta[2] = atan2(-yAxis[0], xAxis[0]);
            }
            else
            {
                // Not a unique solution.
                theta[1] = -OneHalfPi;
                theta[0] = -atan2(xAxis[1], yAxis[1]);
                theta[2] = 0.0;
            }
        }
        else
        {
            // Not a unique solution.
            theta[1] = OneHalfPi;
            theta[0] = atan2(xAxis[1], yAxis[1]);
            theta[2] = 0.0;
        }

        return theta;
    }

    Vector EulerAnglesToBasisX(const Vector& theta)
    {
        return Vector{{
            cos(theta[1]) * cos(theta[2]),
            cos(theta[2]) * sin(theta[0]) * sin(theta[1]) + cos(theta[0]) * sin(theta[2]),
            -cos(theta[0]) * cos(theta[2]) * sin(theta[1]) + sin(theta[0]) * sin(theta[2])
        }};
    }

    Vector EulerAnglesToBasisY(const Vector& theta)
    {
        return Vector{{
            -cos(theta[1]) * sin(theta[2]),
            cos(theta[0]) * cos(theta[2]) - sin(theta[0]) * sin(theta[1]) * sin(theta[2]),
            cos(theta[2]) * sin(theta[0]) + cos(theta[0]) * sin(theta[1]) * sin(theta[2])
        }};
    }

    Vector EulerAnglesToBasisZ(const Vector& theta)
    {
        return Vector{{
            sin(theta[1]),
            -cos(theta[1]) * sin(theta[0]),
            cos(theta[0]) * cos(theta[1])
        }};
    }

    bool IsAbsoluteValueWithinEpsilon(double value, const double epsilon)
    {
        // This function checks whether a value's magnitude is within a given threshold. The default value has been
        // chosen to be close enough to zero for all practical purposes but still be representable as a single precision
        // floating point number.
        value = fabs(value);
        return value <= epsilon;
    }

    bool IsAbsolueValueRatioWithinThreshold(double valueOne, double valueTwo, const double threshold)
    {
        // This function checks whether the magnitude of the ratio of two numbers is within a given threshold.
        valueOne = fabs(valueOne);
        valueTwo = fabs(valueTwo);

        if (valueTwo > valueOne)
        {
            AZStd::swap(valueOne, valueTwo);
        }

        return valueOne > 0.0 && valueTwo > 0.0 && valueTwo / valueOne >= threshold;
    }
} // PhysX::Pipeline
