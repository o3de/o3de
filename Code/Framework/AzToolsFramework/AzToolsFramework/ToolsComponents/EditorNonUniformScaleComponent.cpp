/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzToolsFramework/ToolsComponents/EditorNonUniformScaleComponent.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/ToString.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorNonUniformScaleComponent::OnScaleChanged()
        {
            m_scaleChangedEvent.Signal(m_scale);
        }

        void EditorNonUniformScaleComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorNonUniformScaleComponent, EditorComponentBase>()
                    ->Version(1)
                    ->Field("NonUniformScale", &EditorNonUniformScaleComponent::m_scale)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorNonUniformScaleComponent>("Non-uniform Scale",
                        "Non-uniform scale for this entity only (does not propagate through hierarchy)")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::FixedComponentListIndex, 1)
                        ->Attribute(AZ::Edit::Attributes::RemoveableByUser, true)
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/NonUniformScale.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/NonUniformScale.svg")
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &EditorNonUniformScaleComponent::m_scale, "Non-uniform Scale",
                            "Non-uniform scale for this entity only (does not propagate through hierarchy)")
                        ->Attribute(AZ::Edit::Attributes::Min, AZ::MinTransformScale)
                        ->Attribute(AZ::Edit::Attributes::Max, AZ::MaxTransformScale)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorNonUniformScaleComponent::OnScaleChanged)
                        ;
                }
            }
        }

        void EditorNonUniformScaleComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC_CE("TransformService"));
        }

        void EditorNonUniformScaleComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        void EditorNonUniformScaleComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        // AZ::Component ...
        void EditorNonUniformScaleComponent::Activate()
        {
            AZ::NonUniformScaleRequestBus::Handler::BusConnect(GetEntityId());
        }

        void EditorNonUniformScaleComponent::Deactivate()
        {
            AZ::NonUniformScaleRequestBus::Handler::BusDisconnect();
        }

        // AZ::NonUniformScaleRequestBus::Handler ...
        AZ::Vector3 EditorNonUniformScaleComponent::GetScale() const
        {
            return m_scale;
        }

        void EditorNonUniformScaleComponent::SetScale(const AZ::Vector3& scale)
        {
            if (scale.GetMinElement() >= AZ::MinTransformScale && scale.GetMaxElement() <= AZ::MaxTransformScale)
            {
                m_scale = scale;
            }
            else
            {
                AZ::Vector3 clampedScale = scale.GetClamp(AZ::Vector3(AZ::MinTransformScale), AZ::Vector3(AZ::MaxTransformScale));
                AZ_Warning("Editor Non-uniform Scale Component", false, "SetScale value was clamped from %s to %s for entity %s",
                    AZ::ToString(scale).c_str(), AZ::ToString(clampedScale).c_str(), GetEntity()->GetName().c_str());
                m_scale = clampedScale;
            }
            m_scaleChangedEvent.Signal(m_scale);
        }

        void EditorNonUniformScaleComponent::RegisterScaleChangedEvent(AZ::NonUniformScaleChangedEvent::Handler& handler)
        {
            handler.Connect(m_scaleChangedEvent);
        }

        // EditorComponentBase ...
        void EditorNonUniformScaleComponent::BuildGameEntity(AZ::Entity* gameEntity)
        {
            auto nonUniformScaleComponent = gameEntity->CreateComponent<AzFramework::NonUniformScaleComponent>();
            nonUniformScaleComponent->SetScale(m_scale);
        }
    } // namespace Components
} // namespace AzToolsFramework
