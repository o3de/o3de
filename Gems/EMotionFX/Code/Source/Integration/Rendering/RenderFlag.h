/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/bitset.h>

namespace EMotionFX
{
    enum ActorRenderFlag
    {
        RENDER_SOLID = 0,
        RENDER_WIREFRAME = 1,
        RENDER_LIGHTING = 2,
        RENDER_SHADOWS = 3,
        RENDER_FACENORMALS = 4,
        RENDER_VERTEXNORMALS = 5,
        RENDER_TANGENTS = 6,
        RENDER_AABB = 7,
        RENDER_SKELETON = 8,
        RENDER_LINESKELETON = 9,
        RENDER_NODEORIENTATION = 10,
        RENDER_NODENAMES = 11,
        RENDER_GRID = 12,
        RENDER_BACKFACECULLING = 13,
        RENDER_ACTORBINDPOSE = 14,
        RENDER_RAGDOLL_COLLIDERS = 15,
        RENDER_RAGDOLL_JOINTLIMITS = 16,
        RENDER_HITDETECTION_COLLIDERS = 17,
        RENDER_USE_GRADIENTBACKGROUND = 18,
        RENDER_MOTIONEXTRACTION = 19,
        RENDER_CLOTH_COLLIDERS = 20,
        RENDER_SIMULATEDOBJECT_COLLIDERS = 21,
        RENDER_SIMULATEJOINTS = 22,
        RENDER_EMFX_DEBUG = 23,
        NUM_RENDERFLAGS = 24
    };

    using ActorRenderFlagBitset = AZStd::bitset<ActorRenderFlag::NUM_RENDERFLAGS>;
}
