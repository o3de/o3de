/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Ragdoll.h>
#include <gmock/gmock.h>

namespace EMotionFX
{
    class TestRagdoll
        : public Physics::Ragdoll
    {
    public:
        AZ_CLASS_ALLOCATOR(TestRagdoll, AZ::SystemAllocator)
        AZ_RTTI(TestRagdoll, "{A8FCEA6D-DC28-4D7D-9284-D98AD771E944}", Physics::Ragdoll)

        MOCK_METHOD1(EnableSimulation, void(const Physics::RagdollState&));
        MOCK_METHOD1(EnableSimulationQueued, void(const Physics::RagdollState&));
        MOCK_METHOD0(DisableSimulation, void());
        MOCK_METHOD0(DisableSimulationQueued, void());

        MOCK_CONST_METHOD0(IsSimulated, bool());

        MOCK_CONST_METHOD1(GetState, void(Physics::RagdollState&));
        MOCK_METHOD1(SetState, void(const Physics::RagdollState&));
        MOCK_METHOD1(SetStateQueued, void(const Physics::RagdollState&));

        MOCK_CONST_METHOD2(GetNodeState, void(size_t, Physics::RagdollNodeState&));
        MOCK_METHOD2(SetNodeState, void(size_t, const Physics::RagdollNodeState&));

        MOCK_CONST_METHOD1(GetNode, Physics::RagdollNode*(size_t));
        MOCK_CONST_METHOD0(GetNumNodes, size_t());

        // WorldBody
        MOCK_CONST_METHOD0(GetEntityId, AZ::EntityId());

        MOCK_CONST_METHOD0(GetTransform, AZ::Transform());
        MOCK_METHOD1(SetTransform, void(const AZ::Transform&));

        MOCK_CONST_METHOD0(GetPosition, AZ::Vector3());
        MOCK_CONST_METHOD0(GetOrientation, AZ::Quaternion());

        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb());

        MOCK_METHOD1(RayCast, AzPhysics::SceneQueryHit(const AzPhysics::RayCastRequest&));

        MOCK_CONST_METHOD0(GetNativeType, AZ::Crc32());
        MOCK_CONST_METHOD0(GetNativePointer, void*());
    };
} // namespace EMotionFX
