/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/tuple.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>


namespace ScriptCanvasPhysics
{
    namespace WorldFunctions
    {
        using Result = AZStd::tuple<
            bool /*true if an object was hit*/,
            AZ::Vector3 /*world space position*/,
            AZ::Vector3 /*surface normal*/,
            float /*distance to the hit*/,
            AZ::EntityId /*entity hit, if any*/,
            AZ::Crc32 /*tag of material surface hit, if any*/
            >;

        using OverlapResult = AZStd::tuple<
            bool, /*true if the overlap returned hits*/
            AZStd::vector<AZ::EntityId> /*list of entityIds*/
            >;

        Result RayCastWorldSpaceWithGroup(
            const AZ::Vector3& start,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        Result RayCastFromScreenWithGroup(
            const AZ::Vector2& screenPosition, float distance, const AZStd::string& collisionGroup, AZ::EntityId ignore);

        Result RayCastLocalSpaceWithGroup(
            const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        AZStd::vector<AzPhysics::SceneQueryHit> RayCastMultipleLocalSpaceWithGroup(
            const AZ::EntityId& fromEntityId,
            const AZ::Vector3& direction,
            float distance,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        OverlapResult OverlapSphereWithGroup(
            const AZ::Vector3& position, float radius, const AZStd::string& collisionGroup, AZ::EntityId ignore);

        OverlapResult OverlapBoxWithGroup(
            const AZ::Transform& pose, const AZ::Vector3& dimensions, const AZStd::string& collisionGroup, AZ::EntityId ignore);

        OverlapResult OverlapCapsuleWithGroup(
            const AZ::Transform& pose, float height, float radius, const AZStd::string& collisionGroup, AZ::EntityId ignore);

        Result SphereCastWithGroup(
            float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        Result BoxCastWithGroup(
            float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            const AZ::Vector3& dimensions,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);

        Result CapsuleCastWithGroup(
            float distance,
            const AZ::Transform& pose,
            const AZ::Vector3& direction,
            float height,
            float radius,
            const AZStd::string& collisionGroup,
            AZ::EntityId ignore);
    } // namespace WorldFunctions
} // namespace ScriptCanvasPhysics

#include <Source/World.generated.h>

