/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <PhysX/MeshAsset.h>

#include <Source/Pipeline/PrimitiveShapeFitter/Utils.h>

namespace PhysX::Pipeline
{
    //! This interface encapsulates the concept of a shape expressed as a set of variable parameters.
    //! It supports the operations required by the primitive collider fitting routine to find the optimal
    //! parameterization for a given vertex cloud.
    class AbstractShapeParameterization
    {
    public:
        virtual ~AbstractShapeParameterization() = default;

        //! Pointer definition used as the return type by the factory function for creating abstract shapes.
        using Ptr = AZStd::unique_ptr<AbstractShapeParameterization>;

        //! Number of parameters required to describe this shape.
        virtual AZ::u32 GetDegreesOfFreedom() const = 0;

        //! Pack the parameters into a vector.
        virtual AZStd::vector<double> PackArguments() const = 0;

        //! Unpack the parameters from a vector.
        virtual void UnpackArguments(const AZStd::vector<double>& args) = 0;

        //! Compute the volume of the shape defined by the current set of parameters.
        virtual double GetVolume() const = 0;

        //! Calculate the squared distance of a vertex from the shape defined by the current set of parameters.
        virtual double SquaredDistanceToShape(const Vector& vertex) const = 0;

        //! Extract an actual shape configuration from the current parameterization.
        virtual MeshAssetData::ShapeConfigurationPair GetShapeConfigurationPair() const = 0;
    };

    // Forward declarations for supported abstract shapes.
    class SphereParameterization;
    class BoxParameterization;
    class CapsuleParameterization;

    //! Factory function for creating abstract shapes.
    //! The template parameter must be one of the shapes forward declared in this header.
    //! @param origin A position vector to the center of the shape.
    //! @param xAxis A unit vector pointing along the principal axis of the shape.
    //!   For spheres, the direction of the principal axis does not matter. For boxes, the principal axis should be
    //!   parallel to the longest edge. For capsules, the principal axis should be parallel to the vector between the
    //!   centers of the semi-spheres.
    //! @param yAxis A unit vector perpendicular to the x-axis.
    //! @param zAxis A unit vector perpendicular to both the x-axis and the y-axis.
    //! @param halfRanges Half the dimensions of the shape along each basis vector.
    //!   The components are interpreted as half the lengths along the x-axis, y-axis and z-axis respectively.
    template<typename SHAPE>
    AbstractShapeParameterization::Ptr CreateAbstractShape(
        const Vector& origin,
        const Vector& xAxis,
        const Vector& yAxis,
        const Vector& zAxis,
        const Vector& halfRanges
    );
} // PhysX::Pipeline
