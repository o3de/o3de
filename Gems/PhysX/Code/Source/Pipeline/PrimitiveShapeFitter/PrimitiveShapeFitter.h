/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PhysX/MeshAsset.h>
#include <Cry_Math.h>

namespace PhysX::Pipeline
{
    //! Enum class to specify the primitive shape that should be fitted by the algorithm.
    //! Invoking the algorithm multiple times with different shapes is less efficient than invoking the algorithm once
    //! and letting it decide which shape fits best.
    enum class PrimitiveShapeTarget
    {
        BestFit, //!< The algorithm will try all shapes and discard all but the best fit.
        Sphere,  //!< The algorithm will fit a sphere.
        Box,     //!< The algorithm will fit a box.
        Capsule, //!< The algorithm will fit a capsule.
    };

    //! Fit a primitive shape to a cloud of vertices.
    //! @param meshName A human readable name for the mesh.
    //! @param vertices The vector of vertices that make up the mesh.
    //! @param volumeTermWeight A parameter that controls how aggressively the algorithm tries to minimize the volume of
    //!   the generated primitive. The value must strictly be in the interval (0, 1], but in practice a value no larger
    //!   than 0.002 is recommended.
    //! @param targetShape The shape that the algorithm should fit (by default the best fit is selected automatically).
    //! @return An instance of \ref MeshAssetData::ShapeConfigurationPair.
    //!   The \ref Physics::ShapeConfiguration pointer inside \ref MeshAssetData::ShapeConfigurationPair will be null if
    //!   no shape could be fitted or if an error occurred.
    MeshAssetData::ShapeConfigurationPair FitPrimitiveShape(
        const AZStd::string& meshName,
        const AZStd::vector<Vec3>& vertices,
        const double volumeTermWeight,
        const PrimitiveShapeTarget targetShape = PrimitiveShapeTarget::BestFit
    );
} // PhysX::Pipeline
