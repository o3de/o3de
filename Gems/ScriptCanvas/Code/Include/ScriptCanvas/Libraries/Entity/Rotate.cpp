/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Entity/Rotate.h>

#include <ScriptCanvas/Execution/ErrorBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Entity
        {
            void Rotate::OnInputSignal(const SlotId&)
            {
                AZ::EntityId targetEntity = RotateProperty::GetEntity(this);

                if (!targetEntity.IsValid())
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Invalid entity specified");
                    return;
                }

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, targetEntity);
                if (entity)
                {
                    AZ::Vector3 angles = AZ::Vector3::CreateZero();
                    if (entity->GetState() == AZ::Entity::State::Active)
                    {
                        angles = RotateProperty::GetEulerAngles(this);

                        AZ::Quaternion rotation = AZ::ConvertEulerDegreesToQuaternion(angles);

                        AZ::Transform currentTransform = AZ::Transform::CreateIdentity();
                        AZ::TransformBus::EventResult(currentTransform, targetEntity, &AZ::TransformInterface::GetWorldTM);

                        currentTransform.SetRotation((rotation * currentTransform.GetRotation().GetNormalized()));

                        AZ::TransformBus::Event(targetEntity, &AZ::TransformInterface::SetWorldTM, currentTransform);
                    }
                }

                SignalOutput(RotateProperty::GetOutSlotId(this));
            }
        }
    }
}
