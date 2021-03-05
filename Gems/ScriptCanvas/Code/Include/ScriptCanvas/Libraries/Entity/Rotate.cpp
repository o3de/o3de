/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
                        
                        AZ::Vector3 position = currentTransform.GetTranslation();

                        AZ::Quaternion currentRotation = currentTransform.GetRotation();

                        AZ::Quaternion newRotation = (rotation * currentRotation);
                        newRotation.Normalize();

                        AZ::Transform newTransform = AZ::Transform::CreateIdentity();

                        newTransform.SetScale(currentTransform.GetScale());
                        newTransform.SetRotation(newRotation);
                        newTransform.SetTranslation(position);

                        AZ::TransformBus::Event(targetEntity, &AZ::TransformInterface::SetWorldTM, newTransform);
                    }
                }

                SignalOutput(RotateProperty::GetOutSlotId(this));
            }
        }
    }
}
