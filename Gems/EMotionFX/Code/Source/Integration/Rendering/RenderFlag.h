/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/bitset.h>
#include <AzCore/Preprocessor/Enum.h>
#include <Atom/RHI.Reflect/Bits.h>

namespace EMotionFX
{
    enum ActorRenderFlag
    {
        SOLID = 0,
        WIREFRAME = 1,
        LIGHTING = 2,
        SHADOWS = 3,
        FACENORMALS = 4,
        VERTEXNORMALS = 5,
        TANGENTS = 6,
        AABB = 7,
        SKELETON = 8,
        LINESKELETON = 9,
        NODEORIENTATION = 10,
        NODENAMES = 11,
        GRID = 12,
        BACKFACECULLING = 13,
        ACTORBINDPOSE = 14,
        RAGDOLL_COLLIDERS = 15,
        RAGDOLL_JOINTLIMITS = 16,
        HITDETECTION_COLLIDERS = 17,
        USE_GRADIENTBACKGROUND = 18,
        MOTIONEXTRACTION = 19,
        CLOTH_COLLIDERS = 20,
        SIMULATEDOBJECT_COLLIDERS = 21,
        SIMULATEJOINTS = 22,
        EMFX_DEBUG = 23,
        NUM_RENDERFLAGS = 24
    };

    //! A set of combinable flags which inform the system how an image is to be
    //! bound to the pipeline at all stages of its lifetime.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(
        ActorRenderFlags,
        uint32_t,
        (None, 0),
        (Solid, AZ_BIT(0)),
        (Wireframe, AZ_BIT(1)),
        (Lighting, AZ_BIT(2)),
        (Default, Solid | Lighting),
        (Shadows, AZ_BIT(3)),
        (FaceNormals, AZ_BIT(4)),
        (VertexNormals, AZ_BIT(5)),
        (Tangents, AZ_BIT(6)),
        (AABB, AZ_BIT(7)),
        (Skeleton, AZ_BIT(8)),
        (LineSkeleton, AZ_BIT(9)),
        (NodeOrientation, AZ_BIT(10)),
        (NodeNames, AZ_BIT(11)),
        (Grid, AZ_BIT(12)),
        (BackfaceCulling, AZ_BIT(13)),
        (ActorBindPose, AZ_BIT(14)),
        (RagdollColliders, AZ_BIT(15)),
        (RagdollJointLimits, AZ_BIT(16)),
        (HitDetectionColliders, AZ_BIT(17)),
        (UseGradientBackground, AZ_BIT(18)),
        (MotionExtraction, AZ_BIT(19)),
        (ClothColliders, AZ_BIT(20)),
        (SimulatedObjectColliders, AZ_BIT(21)),
        (SimulatedJoints, AZ_BIT(22)),
        (EmfxDebug, AZ_BIT(23))
    );

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(ActorRenderFlags);
    /*
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(
        ActorRenderFlags,
        uint32,
        (None, 0),
        (Solid, AZ_BIT(static_cast<uint8>(ActorRenderFlag::SOLID))),
        (Wireframe, AZ_BIT(static_cast<uint8>(ActorRenderFlag::WIREFRAME))),
        (Lighting, AZ_BIT(static_cast<uint8>(ActorRenderFlag::LIGHTING))),
        (Default, Solid | Lighting),
        (Shadows, AZ_BIT(static_cast<uint8>(ActorRenderFlag::SHADOWS))),
        (FaceNormals, AZ_BIT(static_cast<uint8>(ActorRenderFlag::FACENORMALS))),
        (VertexNormals, AZ_BIT(static_cast<uint8>(ActorRenderFlag::VERTEXNORMALS))),
        (Tangents, AZ_BIT(static_cast<uint8>(ActorRenderFlag::TANGENTS))),
        (AABB, AZ_BIT(static_cast<uint8>(ActorRenderFlag::AABB))),
        (Skeleton, AZ_BIT(static_cast<uint8>(ActorRenderFlag::SKELETON))),
        (LineSkeleton, AZ_BIT(static_cast<uint8>(ActorRenderFlag::LINESKELETON))),
        (NodeOrientation, AZ_BIT(static_cast<uint8>(ActorRenderFlag::NODEORIENTATION))),
        (NodeNames, AZ_BIT(static_cast<uint8>(ActorRenderFlag::NODENAMES))),
        (Grid, AZ_BIT(static_cast<uint8>(ActorRenderFlag::GRID))),
        (BackfaceCulling, AZ_BIT(static_cast<uint8>(ActorRenderFlag::BACKFACECULLING))),
        (ActorBindPose, AZ_BIT(static_cast<uint8>(ActorRenderFlag::ACTORBINDPOSE))),
        (RagdollColliders, AZ_BIT(static_cast<uint8>(ActorRenderFlag::RAGDOLL_COLLIDERS))),
        (RagdollJointLimits, AZ_BIT(static_cast<uint8>(ActorRenderFlag::RAGDOLL_JOINTLIMITS))),
        (HitDetectionColliders, AZ_BIT(static_cast<uint8>(ActorRenderFlag::HITDETECTION_COLLIDERS))),
        (UseGradientBackground, AZ_BIT(static_cast<uint8>(ActorRenderFlag::USE_GRADIENTBACKGROUND))),
        (MotionExtraction, AZ_BIT(static_cast<uint8>(ActorRenderFlag::MOTIONEXTRACTION))),
        (ClothColliders, AZ_BIT(static_cast<uint8>(ActorRenderFlag::CLOTH_COLLIDERS))),
        (SimulatedObjectColliders, AZ_BIT(static_cast<uint8>(ActorRenderFlag::SIMULATEDOBJECT_COLLIDERS))),
        (SimulatedJoints, AZ_BIT(static_cast<uint8>(ActorRenderFlag::SIMULATEJOINTS))),
        (EmfxDebug, AZ_BIT(static_cast<uint8>(ActorRenderFlag::EMFX_DEBUG))));
    */

    class ActorRenderFlagUtil
    {
    public:
        static bool CheckBit(ActorRenderFlags flags, AZ::u8 index)
        {
            return (flags & ActorRenderFlags(AZ_BIT(index))) != ActorRenderFlags(0);
        }
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::ActorRenderFlags, "{2D2187FA-2C1A-4485-AF7C-AD34C0514105}");
}
