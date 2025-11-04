/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityFunctions.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>

#include <Include/ScriptCanvas/Libraries/Entity/EntityFunctions.generated.h>

namespace ScriptCanvas
{
    namespace EntityFunctions
    {
        AZ::Vector3 GetEntityRight(AZ::EntityId entityId, double scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            AZ::Vector3 vector = worldTransform.GetBasisX();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }

        AZ::Vector3 GetEntityForward(AZ::EntityId entityId, double scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            AZ::Vector3 vector = worldTransform.GetBasisY();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }

        AZ::Vector3 GetEntityUp(AZ::EntityId entityId, double scale)
        {
            AZ::Transform worldTransform = {};
            AZ::TransformBus::EventResult(worldTransform, entityId, &AZ::TransformInterface::GetWorldTM);

            AZ::Vector3 vector = worldTransform.GetBasisZ();
            vector.SetLength(aznumeric_cast<float>(scale));
            return vector;
        }

        void Rotate(const AZ::EntityId& targetEntity, const AZ::Vector3& angles)
        {
            if (!targetEntity.IsValid())
            {
                AZ_Warning("ScriptCanvas", targetEntity.IsValid(), "Invalid entity specified.");
                return;
            }

            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, targetEntity);
            if (entity)
            {
                if (entity->GetState() == AZ::Entity::State::Active)
                {
                    AZ::Quaternion rotation = AZ::ConvertEulerDegreesToQuaternion(angles);

                    AZ::Transform transform = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(transform, targetEntity, &AZ::TransformInterface::GetWorldTM);

                    transform.SetRotation((rotation * transform.GetRotation()).GetNormalized());

                    AZ::TransformBus::Event(targetEntity, &AZ::TransformInterface::SetWorldTM, transform);
                }
            }
        }

        bool IsActive(const AZ::EntityId& entityId)
        {
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            return (entity && entity->GetState() == AZ::Entity::State::Active);
        }

        bool IsValid(const AZ::EntityId& source)
        {
            return source.IsValid();
        }

        AZStd::string ToString(const AZ::EntityId& source)
        {
            return source.ToString();
        }
    } // namespace EntityFunctions
} // namespace ScriptCanvas
