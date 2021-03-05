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
                        ->Attribute(AZ::Edit::Attributes::Category, "Non-uniform Scale")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(
                            AZ::Edit::UIHandlers::Default, &EditorNonUniformScaleComponent::m_scale, "Non-uniform Scale",
                            "Non-uniform scale for this entity only (does not propagate through hierarchy)")
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
            incompatible.push_back(AZ_CRC_CE("SkyCloudService"));
            incompatible.push_back(AZ_CRC_CE("DebugDrawObbService"));
            incompatible.push_back(AZ_CRC_CE("DebugDrawService"));
            incompatible.push_back(AZ_CRC_CE("EMotionFXActorService"));
            incompatible.push_back(AZ_CRC_CE("EMotionFXSimpleMotionService"));
            incompatible.push_back(AZ_CRC_CE("GradientTransformService"));
            incompatible.push_back(AZ_CRC_CE("LegacyMeshService"));
            incompatible.push_back(AZ_CRC_CE("LookAtService"));
            incompatible.push_back(AZ_CRC_CE("SequenceService"));
            incompatible.push_back(AZ_CRC_CE("ClothMeshService"));
            incompatible.push_back(AZ_CRC_CE("PhysXColliderService"));
            incompatible.push_back(AZ_CRC_CE("PhysXTriggerService"));
            incompatible.push_back(AZ_CRC_CE("PhysXJointService"));
            incompatible.push_back(AZ_CRC_CE("PhysXShapeColliderService"));
            incompatible.push_back(AZ_CRC_CE("PhysXCharacterControllerService"));
            incompatible.push_back(AZ_CRC_CE("PhysXRagdollService"));
            incompatible.push_back(AZ_CRC_CE("TouchBendingPhysicsService"));
            incompatible.push_back(AZ_CRC_CE("WaterVolumeService"));
            incompatible.push_back(AZ_CRC_CE("WhiteBoxService"));
            incompatible.push_back(AZ_CRC_CE("NavigationAreaService"));
            incompatible.push_back(AZ_CRC_CE("GeometryService"));
            incompatible.push_back(AZ_CRC_CE("CapsuleShapeService"));
            incompatible.push_back(AZ_CRC_CE("CompoundShapeService"));
            incompatible.push_back(AZ_CRC_CE("CylinderShapeService"));
            incompatible.push_back(AZ_CRC_CE("DiskShapeService"));
            incompatible.push_back(AZ_CRC_CE("FixedVertexContainerService"));
            incompatible.push_back(AZ_CRC_CE("PolygonPrismShapeService"));
            incompatible.push_back(AZ_CRC_CE("SphereShapeService"));
            incompatible.push_back(AZ_CRC_CE("SplineService"));
            incompatible.push_back(AZ_CRC_CE("TubeShapeService"));
            incompatible.push_back(AZ_CRC_CE("VariableVertexContainerService"));
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
            m_scale = scale;
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
