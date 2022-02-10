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
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(ActorRenderFlags, uint32_t,
        (None, 0),
        (Solid, AZ_BIT(ActorRenderFlag::SOLID)),
        (Wireframe, AZ_BIT(ActorRenderFlag::WIREFRAME)),
        (Lighting, AZ_BIT(ActorRenderFlag::LIGHTING)),
        (Default, Solid | Lighting),
        (Shadows, AZ_BIT(ActorRenderFlag::SHADOWS)),
        (FaceNormals, AZ_BIT(ActorRenderFlag::FACENORMALS)),
        (VertexNormals, AZ_BIT(ActorRenderFlag::VERTEXNORMALS)),
        (Tangents, AZ_BIT(ActorRenderFlag::TANGENTS)),
        (AABB, AZ_BIT(ActorRenderFlag::AABB)),
        (Skeleton, AZ_BIT(ActorRenderFlag::SKELETON)),
        (LineSkeleton, AZ_BIT(ActorRenderFlag::LINESKELETON)),
        (NodeOrientation, AZ_BIT(ActorRenderFlag::NODEORIENTATION)),
        (NodeNames, AZ_BIT(ActorRenderFlag::NODENAMES)),
        (Grid, AZ_BIT(ActorRenderFlag::GRID)),
        (BackfaceCulling, AZ_BIT(ActorRenderFlag::BACKFACECULLING)),
        (ActorBindPose, AZ_BIT(ActorRenderFlag::ACTORBINDPOSE)),
        (RagdollColliders, AZ_BIT(ActorRenderFlag::RAGDOLL_COLLIDERS)),
        (RagdollJointLimits, AZ_BIT(ActorRenderFlag::RAGDOLL_JOINTLIMITS)),
        (HitDetectionColliders, AZ_BIT(ActorRenderFlag::HITDETECTION_COLLIDERS)),
        (UseGradientBackground, AZ_BIT(ActorRenderFlag::USE_GRADIENTBACKGROUND)),
        (MotionExtraction, AZ_BIT(ActorRenderFlag::MOTIONEXTRACTION)),
        (ClothColliders, AZ_BIT(ActorRenderFlag::CLOTH_COLLIDERS)),
        (SimulatedObjectColliders, AZ_BIT(ActorRenderFlag::SIMULATEDOBJECT_COLLIDERS)),
        (SimulatedJoints, AZ_BIT(ActorRenderFlag::SIMULATEJOINTS)),
        (EmfxDebug, AZ_BIT(ActorRenderFlag::EMFX_DEBUG))
    );

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(ActorRenderFlags);

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
