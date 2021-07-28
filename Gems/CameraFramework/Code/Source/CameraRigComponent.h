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
#include <AzCore/Math/Transform.h>

namespace Camera
{
    class ICameraLookAtBehavior;
    class ICameraTargetAcquirer;
    class ICameraTransformBehavior;

    //////////////////////////////////////////////////////////////////////////
    /// The CameraRigComponent holds a recipe of behaviors.
    /// It will first attempt to acquire a target by iterating over the LookTargetAquirers until one returns true
    /// Next it will pass a modifiable lookAt Transform to all LookAtBehaviors in order, giving each a pass at further modifying the LookAt Transform
    /// Finally it will pass a modifiable transform to all Transform behaviors in turn, giving each a chance to further modify the transform
    //////////////////////////////////////////////////////////////////////////
    class CameraRigComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(CameraRigComponent, "{286BF97A-1B4A-4EE1-944F-C13B2396227B}");
        ~CameraRigComponent() override;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        AZStd::vector<ICameraTargetAcquirer*> m_targetAcquirers;
        AZStd::vector<ICameraLookAtBehavior*> m_lookAtBehaviors;
        AZStd::vector<ICameraTransformBehavior*> m_transformBehaviors;
        AZ::Transform m_initialTransform;
    };
} // Camera
