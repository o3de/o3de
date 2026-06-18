/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace WhiteBox
{
    //! Enum containing default shape types for the white box mesh.
    enum class DefaultShapeType : int
    {
        Cube,
        Tetrahedron,
        Icosahedron,
        Cylinder,
        Sphere,
        Asset
    };

    //! Solid the Draw Shape tool builds from the drawn footprint + pull height.
    enum class DrawShapeType : int
    {
        Box,       //!< rectangular prism matching the drawn rectangle (Draw Sides = 4)
        Cylinder,  //!< N-sided prism inscribed in the rectangle
        Pyramid,   //!< rectangular base + apex
        Cone,      //!< N-sided base + apex
        Sphere,    //!< UV ellipsoid inscribed in the drawn footprint + pull height (Draw Sides = subdivision)
        Staircase, //!< stepped solid rising along the drawn footprint (Draw Steps = number of stairs)
    };
} // namespace WhiteBox
