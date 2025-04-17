/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CameraRigComponent.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include "CameraFramework/ICameraTargetAcquirer.h"
#include "CameraFramework/ICameraLookAtBehavior.h"
#include "CameraFramework/ICameraTransformBehavior.h"

namespace Camera
{
    // Clean up the behaviours instantiated for us by the editor/serialization system
    CameraRigComponent::~CameraRigComponent()
    {
        for (ICameraTargetAcquirer* targetAcquirer : m_targetAcquirers)
        {
            delete targetAcquirer;
        }
        for (ICameraLookAtBehavior* lookAtBehavior : m_lookAtBehaviors)
        {
            delete lookAtBehavior;
        }
        for (ICameraTransformBehavior* transformBehavior : m_transformBehaviors)
        {
            delete transformBehavior;
        }
    }

    void CameraRigComponent::Init()
    {
        for (ICameraTargetAcquirer* targetAcquirer : m_targetAcquirers)
        {
            targetAcquirer->Init();
        }
        for (ICameraLookAtBehavior* lookAtBehavior : m_lookAtBehaviors)
        {
            lookAtBehavior->Init();
        }
        for (ICameraTransformBehavior* transformBehavior : m_transformBehaviors)
        {
            transformBehavior->Init();
        }
    }

    void CameraRigComponent::Activate()
    {
#ifdef AZ_ENABLE_TRACING
        bool isStaticTransform = false;
        AZ::TransformBus::EventResult(isStaticTransform, GetEntityId(), &AZ::TransformBus::Events::IsStaticTransform);
        AZ_Warning("Camera Rig Component", !isStaticTransform,
            "Camera Rig needs to move, but entity '%s' %s has a static transform.", GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());
#endif

        for (ICameraTargetAcquirer* targetAcquirer : m_targetAcquirers)
        {
            targetAcquirer->Activate(GetEntityId());
        }
        for (ICameraLookAtBehavior* lookAtBehavior : m_lookAtBehaviors)
        {
            lookAtBehavior->Activate(GetEntityId());
        }
        for (ICameraTransformBehavior* transformBehavior : m_transformBehaviors)
        {
            transformBehavior->Activate(GetEntityId());
        }

        m_initialTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_initialTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::TickBus::Handler::BusConnect();
    }

    void CameraRigComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();

        for (ICameraTargetAcquirer* targetAcquirer : m_targetAcquirers)
        {
            targetAcquirer->Deactivate();
        }
        for (ICameraLookAtBehavior* lookAtBehavior : m_lookAtBehaviors)
        {
            lookAtBehavior->Deactivate();
        }
        for (ICameraTransformBehavior* transformBehavior : m_transformBehaviors)
        {
            transformBehavior->Deactivate();
        }
    }

    void CameraRigComponent::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<ICameraTargetAcquirer>()
                ->Version(1);
            serializeContext->Class<ICameraLookAtBehavior>()
                ->Version(1);
            serializeContext->Class<ICameraTransformBehavior>()
                ->Version(1);

            serializeContext->Class<CameraRigComponent, AZ::Component>()
                ->Version(1)
                ->Field("Target Acquirers", &CameraRigComponent::m_targetAcquirers)
                ->Field("Look-at Behaviors", &CameraRigComponent::m_lookAtBehaviors)
                ->Field("Camera Transform Behaviors", &CameraRigComponent::m_transformBehaviors);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<ICameraTargetAcquirer>("ICameraTargetAcquirer", "Base class for all target acquirers.  Implementations can be found in other gems");
                editContext->Class<ICameraLookAtBehavior>("ICameraLookAtBehavior", "Base class for all look at behaviors. Implementations can be found in other gems");
                editContext->Class<ICameraTransformBehavior>("ICameraTransformBehavior", "Base class for all transform behaviors. Implementations can be found in other gems");

                editContext->Class<CameraRigComponent>( "Camera Rig", "The Camera Rig component can be used to add and remove behaviors to drive your camera entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Camera")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/CameraRig.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/CameraRig.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/camera/camera-rig/")
                    ->DataElement(0, &CameraRigComponent::m_targetAcquirers, "Target acquirers",
                    "A list of behaviors that define how a camera will select a target.  They are executed in order until one succeeds")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &CameraRigComponent::m_lookAtBehaviors, "Look-at behaviors",
                    "A list of look-at behaviors.  They are run in order, each having the chance to sequentially modify the look-at target transform")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &CameraRigComponent::m_transformBehaviors, "Transform behaviors",
                    "A list of behaviors that run in order, each having the chance to sequentially modify the camera's transform based on the look-at transform")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void CameraRigComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("CameraRigService"));
    }

    void CameraRigComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("CameraService"));
    }

    void CameraRigComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void CameraRigComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // Step 1 Acquire a target
        AZ::Transform initialCameraTransform = AZ::Transform::Identity();
        AZ::TransformBus::EventResult(initialCameraTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZ::Transform targetTransform(m_initialTransform);
        for (ICameraTargetAcquirer* targetAcquirer : m_targetAcquirers)
        {
            if (targetAcquirer->AcquireTarget(targetTransform))
            {
                break;
            }
        }

        // Step 2 modify the target look at position
        AZ::Transform lookAtTargetTransform(targetTransform);
        for (ICameraLookAtBehavior* CameraLookAtBehavior : m_lookAtBehaviors)
        {
            CameraLookAtBehavior->AdjustLookAtTarget(deltaTime, targetTransform, lookAtTargetTransform);
        }

        // Step 3 modify the camera's position
        AZ::Transform finalTransform(initialCameraTransform);
        for (ICameraTransformBehavior* cameraTransformBehavior : m_transformBehaviors)
        {
            cameraTransformBehavior->AdjustCameraTransform(deltaTime, initialCameraTransform, lookAtTargetTransform, finalTransform);
        }

        // Step 4 Alert the camera component of the new desired transform
        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, finalTransform);
    }
} //namespace Camera
