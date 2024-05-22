/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/typetraits/integral_constant.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/Node.h>
#include <MCore/Source/RefCounted.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/MeshFactory.h>

#include <numeric>

namespace EMotionFX
{
    SimpleJointChainActor::SimpleJointChainActor(size_t jointCount, const char* name)
        : Actor(name)
    {
        if (jointCount)
        {
            AddNode(0, "rootJoint");
            GetBindPose()->SetLocalSpaceTransform(0, Transform::CreateIdentity());
        }

        for (uint32 i = 1; i < jointCount; ++i)
        {
            AddNode(i, ("joint" + AZStd::to_string(i)).c_str(), i - 1);

            Transform transform = Transform::CreateIdentity();
            transform.m_position = AZ::Vector3(static_cast<float>(i), 0.0f, 0.0f);
            GetBindPose()->SetLocalSpaceTransform(i, transform);
        }
    }

    AllRootJointsActor::AllRootJointsActor(size_t jointCount, const char* name)
        : Actor(name)
    {
        for (uint32 i = 0; i < jointCount; ++i)
        {
            AddNode(i, ("rootJoint" + AZStd::to_string(i)).c_str());

            Transform transform = Transform::CreateIdentity();
            transform.m_position = AZ::Vector3(static_cast<float>(i), 0.0f, 0.0f);
            GetBindPose()->SetLocalSpaceTransform(i, transform);
        }
    }

    PlaneActor::PlaneActor(const char* name)
        : SimpleJointChainActor(1, name)
    {
        SetMesh(0, 0, CreatePlane({
            AZ::Vector3(-1.0f, -1.0f, 0.0f),
            AZ::Vector3(1.0f, -1.0f, 0.0f),
            AZ::Vector3(-1.0f,  1.0f, 0.0f),

            AZ::Vector3(1.0f, -1.0f, 0.0f),
            AZ::Vector3(-1.0f,  1.0f, 0.0f),
            AZ::Vector3(1.0f,  1.0f, 0.0f)
        }));
    }

    Mesh* PlaneActor::CreatePlane(const AZStd::vector<AZ::Vector3>& points) const
    {
        const auto vertCount = static_cast<uint32>(points.size());

        AZStd::vector<AZ::u32> indices(vertCount);
        std::iota(indices.begin(), indices.end(), 0);

        AZStd::vector<AZ::Vector3> normals {vertCount, {0.0f, 0.0f, 1.0f}};

        return EMotionFX::MeshFactory::Create(
            indices,
            points,
            normals
        );
    }

    PlaneActorWithJoints::PlaneActorWithJoints(size_t jointCount, const char* name)
        : PlaneActor(name)
    {
        for (uint32 i = 1; i < jointCount; ++i)
        {
            AddNode(i, ("joint" + AZStd::to_string(i)).c_str(), i - 1);

            Transform transform = Transform::CreateIdentity();
            transform.m_position = AZ::Vector3(static_cast<float>(i), 0.0f, 0.0f);
            GetBindPose()->SetLocalSpaceTransform(i, transform);
        }
    }

} // namespace EMotionFX
