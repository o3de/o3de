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
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Quaternion.h>

namespace LmbrCentral
{
    /**
     * Parameters of an entity in the force volume.
     * Used to calculate final force.
     */
    struct EntityParams
    {
        AZ::EntityId m_id;
        AZ::Vector3 m_position;
        AZ::Vector3 m_velocity;
        AZ::Aabb m_aabb;
        float m_mass;
    };

    /**
     * Parameters of the force volume.
     * Used to calculate final force.
     */
    struct VolumeParams
    {
        AZ::EntityId m_id;
        AZ::Vector3 m_position;
        AZ::Quaternion m_rotation;
        AZ::SplinePtr m_spline;
        AZ::Aabb m_aabb;
    };

    /**
     * Represents a single force in the force volume.
     * 
     * Developers should implement this interface and register
     * their class with the EditContext to have their custom
     * force appear in the ForceVolume dropdown box in the editor.
     */
    class Force
    {
    public:
        AZ_CLASS_ALLOCATOR(Force, AZ::SystemAllocator, 0);
        AZ_RTTI(Force, "{9BD236BD-4580-4D6F-B02F-F8F431EBA593}");
        static void Reflect(AZ::SerializeContext& context)
        {
            context.Class<Force>();
        }
        virtual ~Force() = default;

        /**
         * Connect to any busses.
         */
        virtual void Activate(AZ::EntityId /*entityId*/) {}

        /**
         * Disconnect from any busses.
         */
        virtual void Deactivate() {}

        /**
         * Calculate the size and direction the force.
         */
        virtual AZ::Vector3 CalculateForce(const EntityParams& /*entityParams*/, const VolumeParams& /*volumeParams*/)
        {
            return AZ::Vector3::CreateZero();
        }
    };

    /**
     * Requests serviced by the WorldSpaceForce.
     */
    class WorldSpaceForceRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * @brief Sets the direction of the force in worldspace.
         */
        virtual void SetDirection(const AZ::Vector3& direction) = 0;

        /**
         * @brief Gets the direction of the force in world space.
         */
        virtual const AZ::Vector3& GetDirection() = 0;

        /**
         * @brief Sets the magnitude of the force.
         */
        virtual void SetMagnitude(float magnitude) = 0;

        /**
         * @brief Gets the magnitude of the force.
         */
        virtual float GetMagnitude() = 0;
    };

    using WorldSpaceForceRequestBus = AZ::EBus<WorldSpaceForceRequests>;

    /**
     * Requests serviced by the LocalSpaceForce.
     */
    class LocalSpaceForceRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * @brief Sets the direction of the force in localspace.
         */
        virtual void SetDirection(const AZ::Vector3& direction) = 0;

        /**
         * @brief Gets the direction of the force in local space.
         */
        virtual const AZ::Vector3& GetDirection() = 0;

        /**
         * @brief Sets the magnitude of the force.
         */
        virtual void SetMagnitude(float magnitude) = 0;

        /**
         * @brief Gets the magnitude of the force.
          */
        virtual float GetMagnitude() = 0;
    };

    using LocalSpaceForceRequestBus = AZ::EBus<LocalSpaceForceRequests>;

    /**
     * Requests serviced by the PointSpaceForce.
     */
    class PointForceRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * @brief Sets the magnitude of the force.
         */
        virtual void SetMagnitude(float magnitude) = 0;

        /**
         * @brief Gets the magnitude of the force.
         */
        virtual float GetMagnitude() = 0;
    };

    using PointForceRequestBus = AZ::EBus<PointForceRequests>;

    /**
     * Requests serviced by the PointSpaceForce.
     */
    class SplineFollowForceRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * @brief Sets the damping ratio of the force. 
         */
        virtual void SetDampingRatio(float ratio) = 0;

        /**
         * @brief Gets the damping ratio of the force.
         */
        virtual float GetDampingRatio() = 0;

        /**
         * @brief Sets the frequency of the force.
         */
        virtual void SetFrequency(float frequency) = 0;

        /**
         * @brief Gets the frequency of the force.
         */
        virtual float GetFrequency() = 0;

        /**
         * @brief Sets the traget speed of the force.
         */
        virtual void SetTargetSpeed(float targetSpeed) = 0;

        /**
         * @brief Gets the target speed of the force.
         */
        virtual float GetTargetSpeed() = 0;

        /**
         * @brief Sets the lookahead of the force.
         */
        virtual void SetLookAhead(float lookAhead) = 0;

        /**
         * @brief Gets the lookahead of the force.
         */
        virtual float GetLookAhead() = 0;
    };

    using SplineFollowForceRequestBus = AZ::EBus<SplineFollowForceRequests>;

    /**
     * Requests serviced by the LocalSpaceForce.
     */
    class SimpleDragForceRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * @brief Sets the density of the volume.
         */
        virtual void SetDensity(float density) = 0;

        /**
         * @brief Gets the density of the volume.
         */
        virtual float GetDensity() = 0;
    };

    using SimpleDragForceRequestBus = AZ::EBus<SimpleDragForceRequests>;

    /**
     * Requests serviced by the LocalSpaceForce.
     */
    class LinearDampingForceRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * @brief Sets the damping amount of the force.
         */
        virtual void SetDamping(float damping) = 0;

        /**
         * @brief Gets the damping amount of the force.
         */
        virtual float GetDamping() = 0;
    };

    using LinearDampingForceRequestBus = AZ::EBus<LinearDampingForceRequests>;
}
