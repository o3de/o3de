/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <Atom/RHI.Reflect/Bits.h>

namespace EMotionFX
{
    // The index of the render flag which is 0, 1, 2, 3.. based.
    // Do not confuse this with the actual ActorRenderFlags::Type which is 1, 2, 4, 8.. based.
    enum ActorRenderFlagIndex : AZ::u8
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
        ROOTMOTION = 24,
        NUM_RENDERFLAGINDEXES = 25
    };

    //! A set of combinable flags which indicate which render option in turned on for the actor.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(ActorRenderFlags, AZ::u32,
        (None, 0),
        (Solid, AZ_BIT(ActorRenderFlagIndex::SOLID)),
        (Wireframe, AZ_BIT(ActorRenderFlagIndex::WIREFRAME)),
        (Lighting, AZ_BIT(ActorRenderFlagIndex::LIGHTING)),
        (Default, Solid | Lighting),
        (Shadows, AZ_BIT(ActorRenderFlagIndex::SHADOWS)),
        (FaceNormals, AZ_BIT(ActorRenderFlagIndex::FACENORMALS)),
        (VertexNormals, AZ_BIT(ActorRenderFlagIndex::VERTEXNORMALS)),
        (Tangents, AZ_BIT(ActorRenderFlagIndex::TANGENTS)),
        (AABB, AZ_BIT(ActorRenderFlagIndex::AABB)),
        (Skeleton, AZ_BIT(ActorRenderFlagIndex::SKELETON)),
        (LineSkeleton, AZ_BIT(ActorRenderFlagIndex::LINESKELETON)),
        (NodeOrientation, AZ_BIT(ActorRenderFlagIndex::NODEORIENTATION)),
        (NodeNames, AZ_BIT(ActorRenderFlagIndex::NODENAMES)),
        (Grid, AZ_BIT(ActorRenderFlagIndex::GRID)),
        (BackfaceCulling, AZ_BIT(ActorRenderFlagIndex::BACKFACECULLING)),
        (ActorBindPose, AZ_BIT(ActorRenderFlagIndex::ACTORBINDPOSE)),
        (RagdollColliders, AZ_BIT(ActorRenderFlagIndex::RAGDOLL_COLLIDERS)),
        (RagdollJointLimits, AZ_BIT(ActorRenderFlagIndex::RAGDOLL_JOINTLIMITS)),
        (HitDetectionColliders, AZ_BIT(ActorRenderFlagIndex::HITDETECTION_COLLIDERS)),
        (UseGradientBackground, AZ_BIT(ActorRenderFlagIndex::USE_GRADIENTBACKGROUND)),
        (MotionExtraction, AZ_BIT(ActorRenderFlagIndex::MOTIONEXTRACTION)),
        (ClothColliders, AZ_BIT(ActorRenderFlagIndex::CLOTH_COLLIDERS)),
        (SimulatedObjectColliders, AZ_BIT(ActorRenderFlagIndex::SIMULATEDOBJECT_COLLIDERS)),
        (SimulatedJoints, AZ_BIT(ActorRenderFlagIndex::SIMULATEJOINTS)),
        (EmfxDebug, AZ_BIT(ActorRenderFlagIndex::EMFX_DEBUG)),
        (RootMotion, AZ_BIT(ActorRenderFlagIndex::ROOTMOTION))
    );

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(ActorRenderFlags);

    static constexpr ActorRenderFlags s_requireUpdateTransforms =
        ActorRenderFlags::Solid | ActorRenderFlags::Wireframe | ActorRenderFlags::AABB |
        ActorRenderFlags::FaceNormals |ActorRenderFlags::VertexNormals | ActorRenderFlags::Tangents |
        ActorRenderFlags::Skeleton | ActorRenderFlags::LineSkeleton | ActorRenderFlags::NodeOrientation | ActorRenderFlags::NodeNames |
        ActorRenderFlags::RagdollColliders | ActorRenderFlags::RagdollJointLimits | ActorRenderFlags::HitDetectionColliders |
        ActorRenderFlags::ClothColliders | ActorRenderFlags::SimulatedObjectColliders | ActorRenderFlags::SimulatedJoints |
        ActorRenderFlags::EmfxDebug;

    class ActorRenderFlagUtil
    {
    public:
        // Check the bit value with the offset start at 0 from the right.
        // CheckBit(flags, 0) means check the last digit of the flags, CheckBit(flags, 1) means the second digit from right, etc.
        static bool CheckBit(ActorRenderFlags flags, AZ::u8 offset)
        {
            return (flags & ActorRenderFlags(AZ_BIT(offset))) != ActorRenderFlags(0);
        }
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::ActorRenderFlags, "{2D2187FA-2C1A-4485-AF7C-AD34C0514105}");
}
