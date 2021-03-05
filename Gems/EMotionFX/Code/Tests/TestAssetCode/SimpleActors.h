/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        Mesh* CreatePlane(uint32 nodeIndex, const AZStd::vector<AZ::Vector3>& points) const;
    };

    class PlaneActorWithJoints
        : public PlaneActor
    {
    public:
        explicit PlaneActorWithJoints(size_t jointCount, const char* name = "Test actor");
    };

} // namespace EMotionFX
