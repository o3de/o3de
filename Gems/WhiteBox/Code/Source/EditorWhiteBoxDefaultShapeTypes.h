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

    //! Staircase build parameters the Draw Shape tool reads from the component. Plain data
    //! snapshot exposed through EditorWhiteBoxComponentRequestBus::GetDrawStairInfo so callers
    //! fetch the whole group in one request. Defaults match an unset/unhandled component.
    struct DrawStairInfo
    {
        int m_steps = 8;            //!< Number of steps in step-count mode.
        bool m_byHeight = false;    //!< Divide by a fixed riser height instead of a fixed step count.
        float m_stepHeight = 0.25f; //!< Riser height used when dividing by step height.
        int m_rotation = 0;         //!< Orientation in 90-degree steps (0..3) about the surface normal.
    };
} // namespace WhiteBox
