/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/Memory.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace WhiteBox
{
    // How the white box rigid body should be represented in physics.
    enum class WhiteBoxBodyType
    {
        Static,
        Kinematic
    };

    //! How the mesh is cooked for physics.
    enum class WhiteBoxColliderShape
    {
        TriangleMesh, //!< Follows every face; static or kinematic only, and never a trigger.
        ConvexHull,   //!< One convex shape wrapping the whole mesh.
        ConvexParts,  //!< One hull per convex shell; concave shells are cut along their own faces (V-HACD if that fails).
        SimplifiedMesh //!< Triangle mesh reduced to a chosen share of its triangles (stored last so saved values keep their meaning).
    };

    //! Configuration information to use when setting up a WhiteBoxCollider.
    struct WhiteBoxColliderConfiguration
    {
        AZ_CLASS_ALLOCATOR_DECL

        AZ_TYPE_INFO(WhiteBoxColliderConfiguration, "{36DCCE5D-2E26-4FEE-9A17-6B1D401CE46F}")
        static void Reflect(AZ::ReflectContext* context);

        WhiteBoxBodyType m_bodyType = WhiteBoxBodyType::Static; //!< Default the body type to Static.
        WhiteBoxColliderShape m_shape = WhiteBoxColliderShape::TriangleMesh;
        AZ::u32 m_maxHullsPerShell = 32;            //!< Hull budget for each concave shell (exact cuts, then V-HACD).
        AZ::u32 m_decompositionResolution = 100000; //!< V-HACD voxel count; higher follows detail more closely.
        AZ::u32 m_maxVerticesPerHull = 64;          //!< V-HACD per-hull vertex cap (PhysX allows 255).
        AZ::u32 m_meshResolution = 50;              //!< Percent of triangles Triangle Mesh (Simplified) keeps.

        AZ::Crc32 PartsVisibility() const;
        AZ::Crc32 SimplifiedVisibility() const;
    };
} // namespace WhiteBox
