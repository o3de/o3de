/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockSimulatedBody : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(MockSimulatedBody, AZ::SystemAllocator)
        MOCK_CONST_METHOD0(GetAabb, AZ::Aabb ());
        MOCK_CONST_METHOD0(GetEntityId, AZ::EntityId ());
        MOCK_CONST_METHOD0(GetNativePointer, void* ());
        MOCK_CONST_METHOD0(GetNativeType, AZ::Crc32 ());
        MOCK_CONST_METHOD0(GetOrientation, AZ::Quaternion ());
        MOCK_CONST_METHOD0(GetPosition, AZ::Vector3 ());
        MOCK_METHOD0(GetScene, AzPhysics::Scene* ());
        MOCK_CONST_METHOD0(GetTransform, AZ::Transform ());
        MOCK_METHOD1(RayCast, AzPhysics::SceneQueryHit (const AzPhysics::RayCastRequest&));
        MOCK_METHOD1(SetTransform, void (const AZ::Transform&));
    };
} // namespace UnitTest
