/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <EMotionFX/Source/PhysicsSetup.h>

namespace EMotionFX
{
    class Actor;

    class PhysicsSetupUtils
    {
    public:
        static size_t CountColliders(const Actor* actor, PhysicsSetup::ColliderConfigType colliderConfigType, bool ignoreShapeType = true, Physics::ShapeType shapeTypeToCount = Physics::ShapeType::Box);
    };
}
