/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotateMethod.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>

#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvas
{
    namespace Entity
    {
        void RotateMethod::Rotate(const AZ::EntityId& targetEntity, const AZ::Vector3& angles)
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

        void RotateMethod::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<RotateMethod>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<RotateMethod>("Entity Transform", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "");
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<RotateMethod>("Entity Transform")
                    ->Method("Rotate", &Rotate,
                        { { {"Entity", "The entity to apply the rotation on."}, {"Euler Angles", "Euler angles, Pitch/Yaw/Roll."} } });
            }
        }
    }
}
