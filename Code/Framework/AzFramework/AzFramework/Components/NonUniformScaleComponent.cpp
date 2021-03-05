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

#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzFramework
{
    void NonUniformScaleComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NonUniformScaleComponent, AZ::Component>()
                ->Version(1)
                ->Field("NonUniformScale", &NonUniformScaleComponent::m_scale)
                ;
        }
    }

    void NonUniformScaleComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("TransformService"));
    }

    void NonUniformScaleComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
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

    void NonUniformScaleComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void NonUniformScaleComponent::Activate()
    {
        AZ::NonUniformScaleRequestBus::Handler::BusConnect(GetEntityId());
    }

    void NonUniformScaleComponent::Deactivate()
    {
        AZ::NonUniformScaleRequestBus::Handler::BusDisconnect();
    }

    AZ::Vector3 NonUniformScaleComponent::GetScale() const
    {
        return m_scale;
    }

    void NonUniformScaleComponent::SetScale(const AZ::Vector3& scale)
    {
        m_scale = scale;
        m_scaleChangedEvent.Signal(m_scale);
    }

    void NonUniformScaleComponent::RegisterScaleChangedEvent(AZ::NonUniformScaleChangedEvent::Handler& handler)
    {
        handler.Connect(m_scaleChangedEvent);
    }
} // namespace AzFramework
