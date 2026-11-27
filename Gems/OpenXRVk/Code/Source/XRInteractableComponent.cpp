/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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

#include <Source/RigidBodyComponent.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>

namespace OpenXRVk
{
    void XRInteractableComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<XRInteractableComponent, AZ::Component>()
                ->Version(1)
                ->Field("Type of the interactable", &XRInteractableComponent::m_XRInteractableType)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<XRInteractableComponent>("XR Interactable", "Reacts on Ray Interactor from the XR controller")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "XR")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &XRInteractableComponent::m_XRInteractableType, "Type of the interactable", "Type of the interactable")
                      ->EnumAttribute(XRInteractableComponent::XRInteractableType::Simple, "Simple")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<XRInteractableComponent>("XRInteractable Component Group")
                ->Attribute(AZ::Script::Attributes::Category, "OpenXRVk Gem Group")
                ;
        }
    }

    void XRInteractableComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("XRInteractableService"));
    }

    void XRInteractableComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("XRInteractableService"));
    }

    void XRInteractableComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
        required.push_back(AZ_CRC_CE("PhysicsRigidBodyService"));
    }

    void XRInteractableComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void XRInteractableComponent::Init()
    {
        
    }

    void XRInteractableComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        Physics::RigidBodyNotificationBus::Handler::BusConnect(GetEntityId());

        m_cachedRigidBodyComponent = GetEntity()->FindComponent<PhysX::RigidBodyComponent>();
    }

    void XRInteractableComponent::Deactivate()
    {
        if (AZ::TickBus::Handler::BusIsConnected())
        {
            AZ::TickBus::Handler::BusDisconnect();
        }
        Physics::RigidBodyNotificationBus::Handler::BusDisconnect();
    }

    static float SmoothStep(float edge0, float edge1, float t)
    {
        t = AZ::GetClamp((t - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    void XRInteractableComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint timePoint)
    {
        if (!m_hovering)
            return;

        constexpr float animationDuration = 0.25f;
        constexpr float increaseScaleFactor = 0.3f;

        m_hoverTime += deltaTime;
        float t = AZ::GetClamp(m_hoverTime / animationDuration, 0.0f, 1.0f);

        float factor = m_scalingUp
            ? SmoothStep(0.0f, 1.0f, t) * increaseScaleFactor + 1.0f  // 1.0 -> 1.increaseScaleFactor
            : 1 + increaseScaleFactor - SmoothStep(0.0f, 1.0f, t) * increaseScaleFactor;  // 1.increaseScaleFactor -> 1.0

        AZ::Vector3 newScale = m_originalScale * factor;
        if (t >= 1.0f)
        {
            if (m_scalingUp)
            {
                m_scalingUp = false;
                m_hoverTime = 0.0f;
            }
            else
            {
                newScale = m_originalScale;
                m_hovering = false;
            }
        }

        if (m_hasNonUniform)
        {
            AZ::NonUniformScaleRequestBus::Event(GetEntityId(), &AZ::NonUniformScaleRequests::SetScale, newScale);
        }
        else
        {
            AZ::TransformBus::Event(GetEntityId(), &AZ::TransformInterface::SetLocalUniformScale, newScale.GetX());
        }
    }

    // Called when the pointer is hovering over this object
    void XRInteractableComponent::OnHoverStart()
    {
        if (m_hovering)
        {
            return;
        }
        m_hoverTime = 0.0f;
        m_hovering = true;
        m_scalingUp = true;

        if (!m_originalScaleCached)
        {
            CacheOriginalScale();
        }
    }

    // Called when the pointer stops hovering over this object
    void XRInteractableComponent::OnHoverEnd()
    {
    }

    void XRInteractableComponent::CacheOriginalScale()
    {
        m_originalScale = AZ::Vector3::CreateOne();
        m_hasNonUniform = false;

        AZ::Vector3 testScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(testScale, GetEntityId(), &AZ::NonUniformScaleRequests::GetScale);

        if (!testScale.IsClose(AZ::Vector3::CreateOne()))
        {
            m_hasNonUniform = true;
            m_originalScale = testScale;
        }
        else
        {
            float uniform = 1.0f;
            AZ::TransformBus::EventResult(uniform, GetEntityId(), &AZ::TransformInterface::GetLocalUniformScale);
            m_originalScale = AZ::Vector3(uniform);
        }
        m_originalScaleCached = true;
    }

    void XRInteractableComponent::OnGrab()
    {
        m_isGrabbing = true;
        m_cachedRigidBodyComponent->DisablePhysics();
    }

    void XRInteractableComponent::OnRelease(const AZ::Vector3& impulse)
    {
        m_cachedRigidBodyComponent->EnablePhysics();
        m_cachedRigidBodyComponent->ApplyLinearImpulse(impulse * m_cachedRigidBodyComponent->GetMass());
        m_isGrabbing = false;
    }

} // namespace OpenXRVk
