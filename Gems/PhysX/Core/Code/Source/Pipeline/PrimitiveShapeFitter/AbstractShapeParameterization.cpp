/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <algorithm>

#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <Source/Pipeline/PrimitiveShapeFitter/AbstractShapeParameterization.h>

namespace PhysX::Pipeline
{
    // Implementation of the abstract shape class for spheres.
    class SphereParameterization
        : public AbstractShapeParameterization
    {
    public:
        SphereParameterization(
            const Vector& origin,
            [[maybe_unused]] const Vector& xAxis,
            [[maybe_unused]] const Vector& yAxis,
            [[maybe_unused]] const Vector& zAxis,
            const Vector& halfRanges
        )
            : m_origin(origin)
            , m_radius((halfRanges[0] + halfRanges[1] + halfRanges[2]) / 3.0)
        {
        }

        AZ::u32 GetDegreesOfFreedom() const override
        {
            return 4;
        }

        AZStd::vector<double> PackArguments() const override
        {
            // The parameters will be packed as follows:
            // [ originVector ] [ radius ]
            // 0              2 3        3
            //
            // Where:
            //   - originVector is m_origin
            //   - radius is m_radius

            AZStd::vector<double> args;
            args.reserve(GetDegreesOfFreedom());

            args.insert(args.end(), m_origin.begin(), m_origin.end());
            args.insert(args.end(), m_radius);

            return args;
        }

        void UnpackArguments(const AZStd::vector<double>& args) override
        {
            auto it = args.begin();

            std::copy_n(it, 3, m_origin.begin());
            it += 3;

            m_radius = fabs(*it);
        }

        double GetVolume() const override
        {
            return FourThirdsPi * m_radius * m_radius * m_radius;
        }

        double SquaredDistanceToShape(const Vector& vertex) const override
        {
            // Get the vector from the origin to the given vertex, compute its length and subtract the radius.
            const double distanceToShape = Norm(vertex - m_origin) - m_radius;
            return distanceToShape * distanceToShape;
        }

        MeshAssetData::ShapeConfigurationPair GetShapeConfigurationPair() const override
        {
            MeshAssetData::ShapeConfigurationPair result(nullptr, nullptr);

            // Don't return a shape if the sphere is too small.
            if (IsAbsoluteValueWithinEpsilon(m_radius))
            {
                return result;
            }

            // Create shape.
            result.second = AZStd::make_shared<Physics::SphereShapeConfiguration>((float)m_radius);

            // Create transform.
            result.first = AZStd::make_shared<AssetColliderConfiguration>();
            result.first->m_transform = AZ::Transform::CreateTranslation(VecToAZVec3(m_origin));

            return result;
        }

    private:
        Vector m_origin;
        double m_radius;
    };

    // Implementation of the abstract shape class for boxes.
    class BoxParameterization
        : public AbstractShapeParameterization
    {
    public:
        BoxParameterization(
            const Vector& origin,
            const Vector& xAxis,
            const Vector& yAxis,
            const Vector& zAxis,
            const Vector& halfRanges
        )
            : m_origin(origin)
            , m_xAxis(xAxis)
            , m_yAxis(yAxis)
            , m_zAxis(zAxis)
            , m_xHalfLength(halfRanges[0])
            , m_yHalfLength(halfRanges[1])
            , m_zHalfLength(halfRanges[2])
        {
        }

        AZ::u32 GetDegreesOfFreedom() const override
        {
            return 9;
        }

        AZStd::vector<double> PackArguments() const override
        {
            // The parameters will be packed as follows:
            // [ originVector ] [ eulerAngles ] [ xHalfLenght ] [ yHalfLenght ] [ zHalfLenght ]
            // 0              2 3             5 6             6 7             7 8             8
            //
            // Where:
            //   - originVector is m_origin
            //   - eulerAngles are the three Euler angles that describe the rotation of the box
            //   - xHalfLenght is m_xHalfLength
            //   - yHalfLenght is m_yHalfLength
            //   - zHalfLenght is m_zHalfLength

            AZStd::vector<double> args;
            args.reserve(GetDegreesOfFreedom());

            args.insert(args.end(), m_origin.begin(), m_origin.end());

            const Vector eulerAngles = RotationMatrixToEulerAngles(m_xAxis, m_yAxis, m_zAxis);
            args.insert(args.end(), eulerAngles.begin(), eulerAngles.end());

            args.insert(args.end(), m_xHalfLength);
            args.insert(args.end(), m_yHalfLength);
            args.insert(args.end(), m_zHalfLength);

            return args;
        }

        void UnpackArguments(const AZStd::vector<double>& args) override
        {
            auto it = args.begin();

            std::copy_n(it, 3, m_origin.begin());
            it += 3;

            Vector eulerAngles{{0.0, 0.0, 0.0}};
            std::copy_n(it, 3, eulerAngles.begin());
            m_xAxis = EulerAnglesToBasisX(eulerAngles);
            m_yAxis = EulerAnglesToBasisY(eulerAngles);
            m_zAxis = EulerAnglesToBasisZ(eulerAngles);
            it += 3;

            m_xHalfLength = fabs(*(it++));
            m_yHalfLength = fabs(*(it++));
            m_zHalfLength = fabs(*it);
        }

        double GetVolume() const override
        {
            return 8.0 * m_xHalfLength * m_yHalfLength * m_zHalfLength;
        }

        double SquaredDistanceToShape(const Vector& vertex) const override
        {
            // Convert the coordinates of the vertex to box space. Due to symmetry we can take the absolute value of the
            // coordinates.
            const Vector vertexInBoxSpace{{
                fabs(Dot(vertex - m_origin, m_xAxis)),
                fabs(Dot(vertex - m_origin, m_yAxis)),
                fabs(Dot(vertex - m_origin, m_zAxis)),
            }};

            if (
                vertexInBoxSpace[0] < m_xHalfLength &&
                vertexInBoxSpace[1] < m_yHalfLength &&
                vertexInBoxSpace[2] < m_zHalfLength
            )
            {
                // The point is inside the box.
                const double distanceToXFace = m_xHalfLength - vertexInBoxSpace[0];
                const double distanceToYFace = m_yHalfLength - vertexInBoxSpace[1];
                const double distanceToZFace = m_zHalfLength - vertexInBoxSpace[2];

                const double distanceToShape = std::min({distanceToXFace, distanceToYFace, distanceToZFace});
                return distanceToShape * distanceToShape;
            }
            else
            {
                // The closest point on the box is the one where we clamp the vertex's coordinates at the box boundary.
                const Vector closestPointOnBox = {{
                    AZStd::clamp(vertexInBoxSpace[0], 0.0, m_xHalfLength),
                    AZStd::clamp(vertexInBoxSpace[1], 0.0, m_yHalfLength),
                    AZStd::clamp(vertexInBoxSpace[2], 0.0, m_zHalfLength)
                }};

                return NormSquared(vertexInBoxSpace - closestPointOnBox);
            }
        }

        MeshAssetData::ShapeConfigurationPair GetShapeConfigurationPair() const override
        {
            MeshAssetData::ShapeConfigurationPair result(nullptr, nullptr);

            // Don't return a shape if the box is too small or not well-shaped.
            if (
                IsAbsoluteValueWithinEpsilon(m_xHalfLength) ||
                IsAbsoluteValueWithinEpsilon(m_yHalfLength) ||
                IsAbsoluteValueWithinEpsilon(m_zHalfLength) ||
                !IsAbsolueValueRatioWithinThreshold(m_xHalfLength, m_yHalfLength) ||
                !IsAbsolueValueRatioWithinThreshold(m_xHalfLength, m_zHalfLength) ||
                !IsAbsolueValueRatioWithinThreshold(m_yHalfLength, m_zHalfLength)
            )
            {
                return result;
            }

            // Create shape.
            const Vector dimensions = Vector{ {m_xHalfLength, m_yHalfLength, m_zHalfLength} } * 2.0;
            result.second = AZStd::make_shared<Physics::BoxShapeConfiguration>(VecToAZVec3(dimensions));

            // Create transform.
            result.first = AZStd::make_shared<AssetColliderConfiguration>();
            result.first->m_transform = CreateTransformFromCoordinateSystem(m_origin, m_xAxis, m_yAxis, m_zAxis);

            return result;
        }

    private:
        Vector m_origin;
        Vector m_xAxis;
        Vector m_yAxis;
        Vector m_zAxis;
        double m_xHalfLength;
        double m_yHalfLength;
        double m_zHalfLength;
    };

    // Implementation of the abstract shape class for capsules.
    class CapsuleParameterization
        : public AbstractShapeParameterization
    {
    public:
        CapsuleParameterization(
            const Vector& origin,
            const Vector& xAxis,
            const Vector& yAxis,
            const Vector& zAxis,
            const Vector& halfRanges
        )
            : m_origin(origin)
            , m_xAxis(xAxis)
            , m_yAxis(yAxis)
            , m_zAxis(zAxis)
            , m_radius(halfRanges[1])
            , m_halfInnerHeight(AZStd::max(halfRanges[0] - halfRanges[1], 0.0))
        {
        }

        AZ::u32 GetDegreesOfFreedom() const override
        {
            return 7;
        }

        AZStd::vector<double> PackArguments() const override
        {
            // The parameters will be packed as follows:
            // [ originVector ] [ halfInnerHeightVector ] [ radius ]
            // 0              2 3                       5 6        6
            //
            // Where:
            //   - originVector is m_origin
            //   - halfInnerHeightVector is the vector from the origin to the center of the semi-sphere along the x axis
            //   - radius is m_radius

            AZStd::vector<double> args;
            args.reserve(GetDegreesOfFreedom());

            args.insert(args.end(), m_origin.begin(), m_origin.end());

            const Vector halfInnerHeightVector = m_xAxis * m_halfInnerHeight;
            args.insert(args.end(), halfInnerHeightVector.begin(), halfInnerHeightVector.end());

            args.insert(args.end(), m_radius);

            return args;
        }

        void UnpackArguments(const AZStd::vector<double>& args) override
        {
            auto it = args.begin();

            std::copy_n(it, 3, m_origin.begin());
            it += 3;

            Vector halfInnerHeightVector{{0.0, 0.0, 0.0}};
            std::copy_n(it, 3, halfInnerHeightVector.begin());
            m_halfInnerHeight = Norm(halfInnerHeightVector);
            m_xAxis = m_halfInnerHeight > 0 ? halfInnerHeightVector / m_halfInnerHeight : Vector{{1.0, 0.0, 0.0}};
            it += 3;

            m_radius = fabs(*it);
            m_yAxis = ComputeAnyOrthogonalVector(m_xAxis);
            m_zAxis = Cross(m_xAxis, m_yAxis);
        }

        double GetVolume() const override
        {
            return FourThirdsPi * m_radius * m_radius * (1.5 * m_halfInnerHeight + m_radius);
        }

        double SquaredDistanceToShape(const Vector& vertex) const override
        {
            // Convert the coordinates of the vertex to capsule space. Due to symmetry we can take the absolute value of
            // the projection onto the x axis.
            const Vector vertexInCapsuleSpace{{
                fabs(Dot(vertex - m_origin, m_xAxis)),
                Dot(vertex - m_origin, m_yAxis),
                Dot(vertex - m_origin, m_zAxis),
            }};

            // Compute the squared perpendicular distance of the point from the x axis.
            const double squaredDistanceFromVertexToXAxis
                = vertexInCapsuleSpace[1] * vertexInCapsuleSpace[1] + vertexInCapsuleSpace[2] * vertexInCapsuleSpace[2];

            double distanceToShape = 0.0;

            if (vertexInCapsuleSpace[0] <= m_halfInnerHeight)
            {
                // The closest point is on the cylindrical part.
                distanceToShape = sqrt(squaredDistanceFromVertexToXAxis) - m_radius;
            }
            else
            {
                // The closest point is on the spherical part.
                const double distanceFromSphereCenterToVertexXCoordinate = vertexInCapsuleSpace[0] - m_halfInnerHeight;

                const double squaredDistanceFromVertexToSphereCenter = squaredDistanceFromVertexToXAxis
                    + distanceFromSphereCenterToVertexXCoordinate * distanceFromSphereCenterToVertexXCoordinate;

                distanceToShape = sqrt(squaredDistanceFromVertexToSphereCenter) - m_radius;
            }

            return distanceToShape * distanceToShape;
        }

        MeshAssetData::ShapeConfigurationPair GetShapeConfigurationPair() const override
        {
            MeshAssetData::ShapeConfigurationPair result(nullptr, nullptr);

            // Don't return a shape if the capsule is too small or not well-shaped.
            if (
                IsAbsoluteValueWithinEpsilon(m_radius) ||
                IsAbsoluteValueWithinEpsilon(m_halfInnerHeight) ||
                !IsAbsolueValueRatioWithinThreshold(m_radius, m_halfInnerHeight + m_radius)
            )
            {
                return result;
            }

            // Create shape.
            result.second = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(
                (float)(2.0 * (m_halfInnerHeight + m_radius)), (float)m_radius
            );

            // Create transform. For capsules the primary axis is the z-axis.
            result.first = AZStd::make_shared<AssetColliderConfiguration>();
            result.first->m_transform = CreateTransformFromCoordinateSystem(m_origin, m_yAxis, m_zAxis, m_xAxis);

            return result;
        }

    private:
        Vector m_origin;
        Vector m_xAxis;
        Vector m_yAxis;
        Vector m_zAxis;
        double m_radius;
        double m_halfInnerHeight;
    };

    // Factory function for creating shapes.
    template<typename SHAPE>
    AbstractShapeParameterization::Ptr CreateAbstractShape(
        const Vector& origin,
        const Vector& xAxis,
        const Vector& yAxis,
        const Vector& zAxis,
        const Vector& halfRanges
    )
    {
        return AZStd::make_unique<SHAPE>(origin, xAxis, yAxis, zAxis, halfRanges);
    }

    // Explicitly template instantiation for spheres.
    template AbstractShapeParameterization::Ptr CreateAbstractShape<SphereParameterization>(
        const Vector&,
        const Vector&,
        const Vector&,
        const Vector&,
        const Vector&
    );

    // Explicitly template instantiation for boxes.
    template AbstractShapeParameterization::Ptr CreateAbstractShape<BoxParameterization>(
        const Vector&,
        const Vector&,
        const Vector&,
        const Vector&,
        const Vector&
    );

    // Explicitly template instantiation for capsules.
    template AbstractShapeParameterization::Ptr CreateAbstractShape<CapsuleParameterization>(
        const Vector&,
        const Vector&,
        const Vector&,
        const Vector&,
        const Vector&
    );
} // PhysX::Pipeline
