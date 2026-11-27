/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XRRayInteractorComponent.h>
#include <XRInteractableComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <OpenXRVk/OpenXRVkActionsInterface.h>

#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzCore/Component/NonUniformScaleBus.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Shape.h>

#include <Atom/RPI.Public/Material/Material.h>

#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AzCore/Name/Name.h>

#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>

#include "OpenXRVk/OpenXRVkUtils.h"

namespace OpenXRVk
{
    const AZStd::string baseColorPropertyName = "baseColor.color";
    const float translationToYScaleCoefficent = 100;
    const float hoveringXZScaleCoefficient = 4;

    void XRRayInteractorComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XRRayInteractorComponent, AZ::Component>()
                ->Version(1)
                ->Field("Max length of the ray", &XRRayInteractorComponent::m_maxLength)
                ->Field("The default color of the ray", &XRRayInteractorComponent::m_initialRayColor)
                ->Field("The color when the ray hovers over some interactable object", &XRRayInteractorComponent::m_hoveringRayColor)
                ->Field("Label of the grip button", &XRRayInteractorComponent::m_gripControlActionLabel)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XRRayInteractorComponent>("XR Ray Interactor", "Draws ray interactor from the XR controller")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "XR")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRRayInteractorComponent::m_maxLength, "Max length of the ray", "Max length of the ray in meters")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRRayInteractorComponent::m_initialRayColor, "Default color", "The default color of the ray")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRRayInteractorComponent::m_hoveringRayColor, "Hovering color", "The color when the ray hovers over some interactable object")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &XRRayInteractorComponent::m_gripControlActionLabel, "Action name of the grip", "Action Handle Label of OpenXRActionsInterface")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<XRRayInteractorComponent>("XRRayInteractor Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "OpenXRVk Gem Group")
                ;
        }
    }

    void XRRayInteractorComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("XRRayInteractorService"));
    }

    void XRRayInteractorComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("XRRayInteractorService"));
    }

    void XRRayInteractorComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void XRRayInteractorComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void XRRayInteractorComponent::Init()
    {
    }

    void XRRayInteractorComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void XRRayInteractorComponent::Deactivate()
    {
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
    }

    void XRRayInteractorComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (!m_colorDefined)
        {
            AZ::Render::MaterialAssignmentMap originalMaterials;
            AZ::Render::MaterialComponentRequestBus::EventResult(
                originalMaterials, GetEntityId(), &AZ::Render::MaterialComponentRequestBus::Events::GetMaterialMap);

            if (!originalMaterials.empty())
            {
                for (const auto& [materialId, assignment] : originalMaterials)
                {
                    auto materialAsset = assignment.m_materialAsset;
                    if (!materialAsset.IsReady())
                        continue;
                    // Waiting when the asset is ready
                    auto const pIndex = assignment.m_materialInstance->FindPropertyIndex(AZ::Name("baseColor.color"));
                    if (!pIndex.IsValid())
                        continue;
                    // Let's find the right material
                    assignmentId = AZ::Render::MaterialAssignmentId::CreateDefault();

                    AZ::Render::MaterialComponentRequestBus::Event(
                        GetEntityId(),
                        &AZ::Render::MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Color>, 
                        assignmentId, baseColorPropertyName, m_initialRayColor);
                    m_colorDefined = true;
                }
            }
        }

        ProcessOpenXRActions();

        if (m_heldEntity.IsValid())
        {
            // Update held object's position relative to ray
            AZ::Transform rayTransform;
            AZ::TransformBus::EventResult(rayTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
            AZ::Transform targetTransform = rayTransform * m_grabOffset;
            AZ::TransformBus::Event(m_heldEntity, &AZ::TransformBus::Events::SetWorldTM, targetTransform);

            // Track velocity
            AZ::Vector3 currentPosition = targetTransform.GetTranslation();
            m_currentVelocity = (currentPosition - m_lastFramePosition) / deltaTime;
            m_lastFramePosition = currentPosition;

            // Release object when button is released
            if (m_currentSqueezeValue < 0.1f)
            {
                ReleaseHeldEntity();
            }
            return;
        }
        else if (m_currentSqueezeValue > 0.9f && m_currentlyHoveredEntity.IsValid())
        {
            GrabHoveredEntity();
            return;
        }
        else {
            m_heldEntity.SetInvalid();
        }

        // Store initial nonUniform scale X and Z
        AZ::Vector3 nonUniformScale;
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, GetEntityId(), &AZ::NonUniformScaleRequestBus::Events::GetScale);
        if (m_XZnonUniformScale == 0 && nonUniformScale.GetX() > 0)
        {
            m_XZnonUniformScale = nonUniformScale.GetX();
        }

        // Get the world transform of the current entity (typically the controller)
        AZ::Transform rayOriginTransform;
        AZ::TransformBus::EventResult(rayOriginTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        // Define the ray start position and normalized direction
        AZ::Vector3 start = rayOriginTransform.GetTranslation();
        AZ::Vector3 direction = rayOriginTransform.GetBasisY().GetNormalized();

        // Maximum ray length
        float maxDistance = m_maxLength;

        // Get the default physics scene
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);

        if (sceneHandle == AzPhysics::InvalidSceneHandle)
        {
            AZ_Warning("XRRayInteractorComponent", false, "Invalid physics scene.");
            return;
        }

        // Prepare the raycast request
        AzPhysics::RayCastRequest request;
        request.m_start = start;
        request.m_direction = direction;
        request.m_distance = maxDistance;
        request.m_reportMultipleHits = false;

        // Perform the raycast query
        AzPhysics::SceneQueryHits hitResult;
        sceneInterface->QueryScene(sceneHandle, &request, hitResult);

        // Handle the result of the raycast
        if (!hitResult.m_hits.empty())
        {
            AzPhysics::SceneQueryHit &hit = hitResult.m_hits[0];

            // You can use hitResult.m_position or hitResult.m_entityId for further logic
            if (hit.m_entityId.IsValid())
            {
                if (m_currentlyHoveredEntity != hit.m_entityId)
                {
                    CheckEndHovering();

                    XRInteractableComponent* xrInteractableComponent = GetXRInterctableComponent(hit.m_entityId);
                    if (xrInteractableComponent)     // Interactable component has changed
                    {
                        AZ::Render::MaterialComponentRequestBus::Event(
                            GetEntityId(),
                            &AZ::Render::MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Color>,
                            assignmentId, baseColorPropertyName, m_hoveringRayColor);

                        xrInteractableComponent->OnHoverStart();
                        m_currentlyHoveredEntity = hit.m_entityId;
                    }
                }
            }
            else
            {
                CheckEndHovering();
            }

            float xzScale = m_currentlyHoveredEntity.IsValid() ? m_XZnonUniformScale * hoveringXZScaleCoefficient : m_XZnonUniformScale;
            nonUniformScale.Set(xzScale, hit.m_distance * translationToYScaleCoefficent, xzScale);
            AZ::NonUniformScaleRequestBus::Event(GetEntityId(), &AZ::NonUniformScaleRequestBus::Events::SetScale, nonUniformScale);
        }
        else
        {
            // Restore initial scale of the ray
            nonUniformScale.Set(m_XZnonUniformScale, m_maxLength* translationToYScaleCoefficent, m_XZnonUniformScale);
            AZ::NonUniformScaleRequestBus::Event(GetEntityId(), &AZ::NonUniformScaleRequestBus::Events::SetScale, nonUniformScale);
            CheckEndHovering();
        }
    }

    void XRRayInteractorComponent::CheckEndHovering()
    {
        if (m_currentlyHoveredEntity.IsValid())
        {
            XRInteractableComponent* oldCRInteractableComponent = GetXRInterctableComponent(m_currentlyHoveredEntity);
            if (oldCRInteractableComponent)
            {
                oldCRInteractableComponent->OnHoverEnd();
                m_currentlyHoveredEntity.SetInvalid();
                AZ::Render::MaterialComponentRequestBus::Event(
                    GetEntityId(),
                    &AZ::Render::MaterialComponentRequestBus::Events::SetPropertyValueT<AZ::Color>,
                    assignmentId, baseColorPropertyName, m_initialRayColor);
            }
        }
    }

    void XRRayInteractorComponent::ProcessOpenXRActions()
    {
        m_currentSqueezeValue = 0;
        auto actionsIFace = OpenXRActionsInterface::Get();
        if (!actionsIFace)
        {
            return;
        }

        if (!m_controllerSqueezeHandle.IsValid())
        {
            // Try to cache all handles.
            m_controllerSqueezeHandle = actionsIFace->GetActionHandle("main_action_set", m_gripControlActionLabel);
            if (!m_controllerSqueezeHandle.IsValid())
            {
                // Most likely the Action System failed to load the ActionSets asset.
                return;
            }

        }
        m_currentSqueezeValue = ReadActionHandleFloat(actionsIFace, m_controllerSqueezeHandle);
    }

    void XRRayInteractorComponent::GrabHoveredEntity()
    {
        m_heldEntity = m_currentlyHoveredEntity;

        AZ::Transform controllerWorldTM;
        AZ::Transform objectWorldTM;
        AZ::TransformBus::EventResult(controllerWorldTM, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::TransformBus::EventResult(objectWorldTM, m_heldEntity, &AZ::TransformBus::Events::GetWorldTM);
        m_grabOffset = controllerWorldTM.GetInverse() * objectWorldTM;

        m_lastFramePosition = objectWorldTM.GetTranslation();
        m_currentVelocity = AZ::Vector3::CreateZero();

        XRInteractableComponent* xrInteractableComponent = GetXRInterctableComponent(m_heldEntity);
        if (xrInteractableComponent)
        {
            xrInteractableComponent->OnGrab();
        }
    }

    void XRRayInteractorComponent::ReleaseHeldEntity()
    {
        // Call OnRelease on the XRInteractableComponent if available
        XRInteractableComponent* xrInteractableComponent = GetXRInterctableComponent(m_heldEntity);
        if (xrInteractableComponent)
        {
            xrInteractableComponent->OnRelease(m_currentVelocity);
        }

        m_heldEntity.SetInvalid();
    }

    XRInteractableComponent* XRRayInteractorComponent::GetXRInterctableComponent(AZ::EntityId entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, entityId);
        if (entity)
        {
            return entity->FindComponent<XRInteractableComponent>();
        }
        return nullptr;
    }


} // namespace OpenXRVk
