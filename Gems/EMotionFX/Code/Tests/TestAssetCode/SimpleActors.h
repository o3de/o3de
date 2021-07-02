/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/Actor.h>

namespace EMotionFX
{
    class Mesh;

    class SimpleJointChainActor
        : public Actor
    {
    public:
        explicit SimpleJointChainActor(size_t jointCount, const char* name = "Test actor");
    };

    class AllRootJointsActor
        : public Actor
    {
    public:
        explicit AllRootJointsActor(size_t jointCount, const char* name = "Test actor");
    };

    class PlaneActor
        : public SimpleJointChainActor
    {
    public:
        explicit PlaneActor(const char* name = "Test actor");
    private:
        Mesh* CreatePlane(const AZStd::vector<AZ::Vector3>& points) const;
    };

    class PlaneActorWithJoints
        : public PlaneActor
    {
    public:
        explicit PlaneActorWithJoints(size_t jointCount, const char* name = "Test actor");
    };

} // namespace EMotionFX
