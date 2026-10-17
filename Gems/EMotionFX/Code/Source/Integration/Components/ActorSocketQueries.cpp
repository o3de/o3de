/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/SocketSetup.h>
#include <EMotionFX/Source/TransformData.h>
#include <Integration/Components/ActorSocketQueries.h>

namespace EMotionFX::Integration::SocketQueries
{
    namespace
    {
        // Returns the socket when its index is valid, warning on behalf of the caller otherwise.
        const ActorSocket* FindSocket(const ActorInstance* actorInstance, [[maybe_unused]] const AZ::Entity& entity, size_t socketIndex, [[maybe_unused]] const char* caller)
        {
            if (socketIndex >= GetNumSockets(actorInstance))
            {
                AZ_Warning("EMotionFX", false, "%s: Invalid socket index %zu. Entity: %s", caller, socketIndex, entity.GetName().c_str());
                return nullptr;
            }
            return &actorInstance->GetActor()->GetSocketSetup()->GetSocket(socketIndex);
        }

        bool HasJoint(const ActorInstance* actorInstance, [[maybe_unused]] const AZ::Entity& entity, const ActorSocket& socket, [[maybe_unused]] const char* caller)
        {
            if (socket.GetJointIndex() >= actorInstance->GetActor()->GetNumNodes())
            {
                AZ_WarningOnce("EMotionFX", false, "%s: The parent joint '%s' of socket '%s' is not in the skeleton. Entity: %s",
                    caller, socket.GetJointName().c_str(), socket.GetName().c_str(), entity.GetName().c_str());
                return false;
            }
            return true;
        }

        size_t FindSocketIndexByName(const ActorInstance* actorInstance, [[maybe_unused]] const AZ::Entity& entity, const char* name, [[maybe_unused]] const char* caller)
        {
            const size_t socketIndex = GetSocketIndexByName(actorInstance, name);
            AZ_Warning("EMotionFX", socketIndex != ActorComponentRequests::s_invalidSocketIndex, "%s: Socket '%s' does not exist. Entity: %s",
                caller, name ? name : "", entity.GetName().c_str());
            return socketIndex;
        }

        AZ::Transform GetEntityWorldTM(const AZ::Entity& entity)
        {
            AZ::TransformInterface* transform = entity.GetTransform();
            return transform ? transform->GetWorldTM() : AZ::Transform::CreateIdentity();
        }
    } // namespace

    size_t GetNumSockets(const ActorInstance* actorInstance)
    {
        return actorInstance ? actorInstance->GetActor()->GetSocketSetup()->GetNumSockets() : 0;
    }

    AZStd::string GetSocketName(const ActorInstance* actorInstance, size_t socketIndex)
    {
        if (socketIndex >= GetNumSockets(actorInstance))
        {
            return {};
        }
        return actorInstance->GetActor()->GetSocketSetup()->GetSocket(socketIndex).GetName();
    }

    size_t GetSocketIndexByName(const ActorInstance* actorInstance, const char* name)
    {
        if (!actorInstance || !name)
        {
            return ActorComponentRequests::s_invalidSocketIndex;
        }

        const size_t index = actorInstance->GetActor()->GetSocketSetup()->FindSocketIndex(name);
        return index != SocketSetup::InvalidIndex ? index : ActorComponentRequests::s_invalidSocketIndex;
    }

    AZ::Transform GetSocketTransform(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex, Space space)
    {
        const ActorSocket* socket = FindSocket(actorInstance, entity, socketIndex, "GetSocketTransform");
        if (!socket)
        {
            return AZ::Transform::CreateIdentity();
        }

        if (space == Space::LocalSpace)
        {
            return socket->GetLocalTransform();
        }

        if (!HasJoint(actorInstance, entity, *socket, "GetSocketTransform"))
        {
            return AZ::Transform::CreateIdentity();
        }

        const Pose* currentPose = actorInstance->GetTransformData()->GetCurrentPose();
        const size_t jointIndex = socket->GetJointIndex();
        const AZ::Transform jointTransform = space == Space::ModelSpace
            ? currentPose->GetModelSpaceTransform(jointIndex).ToAZTransform()
            : currentPose->GetWorldSpaceTransform(jointIndex).ToAZTransform();
        return jointTransform * socket->GetLocalTransform();
    }

    AZ::Vector3 GetSocketForward(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex, Space space)
    {
        return GetSocketTransform(actorInstance, entity, socketIndex, space).GetBasisY().GetNormalizedSafe();
    }

    AZ::Transform GetSocketTransformByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name, Space space)
    {
        const size_t socketIndex = FindSocketIndexByName(actorInstance, entity, name, "GetSocketTransformByName");
        if (socketIndex == ActorComponentRequests::s_invalidSocketIndex)
        {
            return AZ::Transform::CreateIdentity();
        }
        return GetSocketTransform(actorInstance, entity, socketIndex, space);
    }

    AZ::Vector3 GetSocketForwardByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name, Space space)
    {
        const size_t socketIndex = FindSocketIndexByName(actorInstance, entity, name, "GetSocketForwardByName");
        if (socketIndex == ActorComponentRequests::s_invalidSocketIndex)
        {
            return AZ::Vector3::CreateAxisY();
        }
        return GetSocketForward(actorInstance, entity, socketIndex, space);
    }

    AZ::Transform GetSocketBindTransform(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex, Space space)
    {
        const ActorSocket* socket = FindSocket(actorInstance, entity, socketIndex, "GetSocketBindTransform");
        if (!socket)
        {
            return AZ::Transform::CreateIdentity();
        }

        if (space == Space::LocalSpace)
        {
            return socket->GetLocalTransform();
        }

        if (!HasJoint(actorInstance, entity, *socket, "GetSocketBindTransform"))
        {
            return AZ::Transform::CreateIdentity();
        }

        const Pose* bindPose = actorInstance->GetTransformData()->GetBindPose();
        const AZ::Transform modelSpace = bindPose->GetModelSpaceTransform(socket->GetJointIndex()).ToAZTransform() * socket->GetLocalTransform();
        return space == Space::ModelSpace ? modelSpace : GetEntityWorldTM(entity) * modelSpace;
    }

    AZ::Transform GetSocketBindTransformByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name, Space space)
    {
        const size_t socketIndex = FindSocketIndexByName(actorInstance, entity, name, "GetSocketBindTransformByName");
        if (socketIndex == ActorComponentRequests::s_invalidSocketIndex)
        {
            return AZ::Transform::CreateIdentity();
        }
        return GetSocketBindTransform(actorInstance, entity, socketIndex, space);
    }

    AZ::Transform GetSocketTransformFromEntity(const ActorInstance* actorInstance, const AZ::Entity& entity, size_t socketIndex)
    {
        if (!FindSocket(actorInstance, entity, socketIndex, "GetSocketTransformFromEntity"))
        {
            return AZ::Transform::CreateIdentity();
        }

        // The actor instance's world transform only catches up with the entity during the animation update, so compose with the entity's.
        return GetEntityWorldTM(entity) * GetSocketTransform(actorInstance, entity, socketIndex, Space::ModelSpace);
    }

    AZ::Transform GetSocketTransformFromEntityByName(const ActorInstance* actorInstance, const AZ::Entity& entity, const char* name)
    {
        const size_t socketIndex = FindSocketIndexByName(actorInstance, entity, name, "GetSocketTransformFromEntityByName");
        if (socketIndex == ActorComponentRequests::s_invalidSocketIndex)
        {
            return AZ::Transform::CreateIdentity();
        }
        return GetSocketTransformFromEntity(actorInstance, entity, socketIndex);
    }
} // namespace EMotionFX::Integration::SocketQueries
