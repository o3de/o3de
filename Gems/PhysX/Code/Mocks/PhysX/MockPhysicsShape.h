/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Physics/Shape.h>
#include <gmock/gmock.h>

namespace UnitTest
{
    class MockPhysicsShape : public Physics::Shape
    {
    public:
        AZ_CLASS_ALLOCATOR(MockPhysicsShape, AZ::SystemAllocator)
        MOCK_METHOD1(AttachedToActor, void (void*));
        MOCK_METHOD0(DetachedFromActor, void ());
        MOCK_CONST_METHOD1(GetAabb, AZ::Aabb (const AZ::Transform&));
        MOCK_CONST_METHOD0(GetAabbLocal, AZ::Aabb ());
        MOCK_CONST_METHOD0(GetCollisionGroup, AzPhysics::CollisionGroup ());
        MOCK_CONST_METHOD0(GetCollisionLayer, AzPhysics::CollisionLayer ());
        MOCK_CONST_METHOD0(GetContactOffset, float ());
        MOCK_CONST_METHOD3(GetGeometry, void(AZStd::vector<AZ::Vector3>&, AZStd::vector<AZ::u32>&, const AZ::Aabb*));
        MOCK_CONST_METHOD0(GetLocalPose, AZStd::pair<AZ::Vector3, AZ::Quaternion> ());
        MOCK_CONST_METHOD0(GetMaterial, AZStd::shared_ptr<Physics::Material>());
        MOCK_CONST_METHOD0(GetMaterialId, Physics::MaterialId());
        MOCK_METHOD0(GetNativePointer, void* ());
        MOCK_CONST_METHOD0(GetNativePointer, const void*());
        MOCK_CONST_METHOD0(GetRestOffset, float ());
        MOCK_CONST_METHOD0(GetTag, AZ::Crc32 ());
        MOCK_METHOD2(RayCast, AzPhysics::SceneQueryHit (const AzPhysics::RayCastRequest&, const AZ::Transform&));
        MOCK_METHOD1(RayCastLocal, AzPhysics::SceneQueryHit (const AzPhysics::RayCastRequest&));
        MOCK_METHOD1(SetCollisionGroup, void (const AzPhysics::CollisionGroup&));
        MOCK_METHOD1(SetCollisionLayer, void (const AzPhysics::CollisionLayer&));
        MOCK_METHOD1(SetContactOffset, void (float));
        MOCK_METHOD2(SetLocalPose, void (const AZ::Vector3&, const AZ::Quaternion&));
        MOCK_METHOD1(SetMaterial, void (const AZStd::shared_ptr<Physics::Material>&));
        MOCK_METHOD1(SetName, void (const char*));
        MOCK_METHOD1(SetRestOffset, void (float));
    };
} // namespace UnitTest
