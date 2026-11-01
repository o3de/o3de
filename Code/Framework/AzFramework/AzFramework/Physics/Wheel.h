/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector3.h>

#include <AzFramework/Physics/Shape.h>

#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>
#include <AzFramework/AzFrameworkAPI.h>

namespace AZ
{
    class ReflectContext;
}

namespace Physics
{
    class Vehicle;

    struct AZF_API WheelConfiguration
        : public AzPhysics::SimulatedBodyConfiguration
    {
        AZ_RTTI(WheelConfiguration, "{B1F3C9B7-8E5A-4B0D-BDCD-9F2C7E1F8B9E}", AzPhysics::SimulatedBodyConfiguration);
        AZ_CLASS_ALLOCATOR(WheelConfiguration, AZ::SystemAllocator);
        static void Reflect(AZ::ReflectContext* context);
        float m_radius;
        float m_width;
        float m_mass;
        float m_moi;
        float m_dampingRate;

        // suspension settings
        AZ::Vector3 m_suspensionDirection; // point downward
        float m_suspensionMaxCompressionLength;
        float m_suspensionMaxDroopLength;
        float m_suspensionStiffness;
        float m_suspensionDamping;
    };

    class AZF_API Wheel
        : public AzPhysics::SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR(Wheel, AZ::SystemAllocator);
        AZ_RTTI(Wheel, "{DFBBFFE3-8BF3-45BE-8541-926E92CE014F}", AzPhysics::SimulatedBody);

        ~Wheel() override = default;

        // rotation
        virtual float GetRotationSpeed() const = 0;
        virtual void SetRotationSpeed(float angle) = 0;
        virtual float GetRotationAngle() const = 0;
        virtual void SetRotationAngle(float angle) = 0;

        // road geometry query
        virtual bool IsInContact() const = 0;
        virtual AZ::Vector3 GetContactPosition() const = 0;
        virtual AZ::Vector3 GetContactNormal() const = 0;
        virtual AZ::Vector3 GetContactPointVelocity()  const = 0;
        virtual AzPhysics::SimulatedBodyHandle GetContactBodyHandle() const = 0;
        virtual Physics::Shape* GetContactShape() const = 0;

        // friction
        virtual float GetLongitudinalFriction() const = 0;
        virtual float GetLateralFriction() const = 0;
        virtual float GetLongitudinalSlip() const = 0;
        virtual float GetLateralSlip() const = 0;

        // suspension
        virtual float GetJounce() const = 0;
    };
}
