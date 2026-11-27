/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <AzCore/Component/Entity.h>
#include <OpenXRVk/OpenXRVkActionsInterface.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <Source/RigidBodyComponent.h>

namespace OpenXRVk
{
    //! XRInteractableComponent defines that the object can be taken using the RayInteractor
    class XRInteractableComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public Physics::RigidBodyNotificationBus::Handler
    {
    public:

        enum class XRInteractableType : AZ::u32 {
            Simple
        };

        AZ_COMPONENT(OpenXRVk::XRInteractableComponent, "{9234ABCD-234B-12AB-242D-6436DEFC38BB}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);


        // Called when the pointer is hovering over this object
        virtual void OnHoverStart();

        // Called when the pointer stops hovering over this object
        virtual void OnHoverEnd();

        virtual void OnGrab();
        virtual void OnRelease(const AZ::Vector3& impulse);

        bool IsHovering() { return m_isHovering; }
        bool IsGrabbing() { return m_isGrabbing; }

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint timePoint) override;

    private:
        bool m_isHovering = false;
        XRInteractableType m_XRInteractableType = XRInteractableType::Simple;

        // Animation state
        float m_hoverTime = 0.0f;
        bool m_hovering = false;
        bool m_scalingUp = true;

        // Scale state
        AZ::Vector3 m_originalScale = AZ::Vector3::CreateOne();
        bool m_hasNonUniform = false;

        bool m_originalScaleCached = false;
        void CacheOriginalScale();

        bool m_isGrabbing = false;

        PhysX::RigidBodyComponent* m_cachedRigidBodyComponent = nullptr;
    };

} // namespace OpenXRVk
