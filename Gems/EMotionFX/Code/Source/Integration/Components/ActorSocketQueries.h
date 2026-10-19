/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/string/string.h>
#include <Integration/ActorComponentBus.h>

namespace AZ
{
    class Entity;
}

namespace EMotionFX
{
    class ActorInstance;

    // Socket queries shared by the runtime and editor actor components; a null actor instance behaves like an actor without sockets.
    namespace Integration::SocketQueries
    {
        size_t GetNumSockets(const ActorInstance* actorInstance);
        AZStd::string GetSocketName(const ActorInstance* actorInstance, size_t socketIndex);
        size_t GetSocketIndexByName(const ActorInstance* actorInstance, const char* name);

        AZ::Transform GetSocketTransform(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex, Space space);
        AZ::Vector3 GetSocketForward(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex, Space space);
        AZ::Transform GetSocketTransformByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name, Space space);
        AZ::Vector3 GetSocketForwardByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name, Space space);

        AZ::Transform GetSocketBindTransform(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex, Space space);
        AZ::Transform GetSocketBindTransformByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name, Space space);

        AZ::Transform GetSocketTransformFromEntity(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex);
        AZ::Transform GetSocketTransformFromEntityByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name);
    } // namespace Integration::SocketQueries
} // namespace EMotionFX
