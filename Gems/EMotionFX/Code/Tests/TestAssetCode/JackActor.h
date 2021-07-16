/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/Actor.h>

namespace EMotionFX
{
    class JackNoMeshesActor
        : public Actor
    {
    public:
        explicit JackNoMeshesActor(const char* name = "Jack with no meshes actor");
    };
} // namespace EMotionFX
