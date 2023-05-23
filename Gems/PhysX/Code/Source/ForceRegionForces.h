/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Spline.h>
#include <PhysX/ForceRegionComponentBus.h>
#include <AzCore/Math/Aabb.h>

namespace AZ
{
    class ReflectContext;
}

namespace PhysX
{
    /// Parameters of an entity in the force region.
    /// Used to calculate final force.
    struct EntityParams
    {
        AZ::EntityId m_id;
        AZ::Vector3 m_position;
        AZ::Vector3 m_velocity;
        AZ::Aabb m_aabb;
        float m_mass;
    };

    /// Parameters of the force region.
    /// Used to calculate final force.
    struct RegionParams
    {
        AZ::EntityId m_id;
        AZ::Vector3 m_position;
        AZ::Quaternion m_rotation;
        float m_scale;
        AZ::SplinePtr m_spline;
        AZ::Aabb m_aabb;
    };

    /// Requests serviced by all forces used by force regions.
    class BaseForce
    {
    public:
        AZ_CLASS_ALLOCATOR(BaseForce, AZ::SystemAllocator);
        AZ_RTTI(BaseForce, "{0D1DFFE1-16C1-425B-972B-DC70FDC61B56}");
        static void Reflect(AZ::SerializeContext& context);

        virtual ~BaseForce() = default;

        /// Connect to any buses.
        virtual void Activate(AZ::EntityId entityId)
        {
            m_entityId = entityId;
        }

        /// Disconnect from any buses.
        virtual void Deactivate()
        {
            m_entityId.SetInvalid();
        }

        /// Calculate the size and direction the force.
        virtual AZ::Vector3 CalculateForce(const EntityParams& entityParams
            , const RegionParams& volumeParams) const = 0;

    protected:
        void NotifyChanged();

        AZ::EntityId m_entityId;
    };

    /// Class for a world space force exerted on bodies in a force region.
    class ForceWorldSpace final
        : public BaseForce
        , private ForceWorldSpaceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceWorldSpace, AZ::SystemAllocator);
        AZ_RTTI(ForceWorldSpace, "{A6C17DD3-7A09-4BC7-8ACC-C0BD04EA8F7C}", BaseForce);
        ForceWorldSpace() = default;
        ForceWorldSpace(const AZ::Vector3& direction, float magnitude);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BaseForce::Activate(entityId);
            ForceWorldSpaceRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForceWorldSpaceRequestBus::Handler::BusDisconnect();
            BaseForce::Deactivate();
        }

    private:
        // ForceWorldSpaceRequestBus
        void SetDirection(const AZ::Vector3& direction) override;
        AZ::Vector3 GetDirection() const override;
        void SetMagnitude(float magnitude) override;
        float GetMagnitude() const override;

        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisZ();
        float m_magnitude = 10.f;
    };

    /// Class for a local space force exerted on bodies in a force region.
    class ForceLocalSpace final
        : public BaseForce
        , private ForceLocalSpaceRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceLocalSpace, AZ::SystemAllocator);
        AZ_RTTI(ForceLocalSpace, "{F0EAFB7C-1BC7-4497-99AE-ECBF7169AB81}", BaseForce);
        ForceLocalSpace() = default;
        ForceLocalSpace(const AZ::Vector3& direction, float magnitude);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BaseForce::Activate(entityId);
            ForceLocalSpaceRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForceLocalSpaceRequestBus::Handler::BusDisconnect();
            BaseForce::Deactivate();
        }

    private:
        // ForceLocalSpaceRequestBus
        void SetDirection(const AZ::Vector3& direction) override;
        AZ::Vector3 GetDirection() const override;
        void SetMagnitude(float magnitude) override;
        float GetMagnitude() const override;

        AZ::Vector3 m_direction = AZ::Vector3::CreateAxisZ();
        float m_magnitude = 10.0f;
    };

    /// Class for a point force exerted on bodies in a force region.
    /// Bodies in a force region with a point force are repelled away from the center of the force region.
    class ForcePoint final
        : public BaseForce
        , private ForcePointRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForcePoint, AZ::SystemAllocator);
        AZ_RTTI(ForcePoint, "{3F8ABEAC-6972-4845-A131-EA9831029E68}", BaseForce);
        ForcePoint() = default;
        explicit ForcePoint(float magnitude);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BaseForce::Activate(entityId);
            ForcePointRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForcePointRequestBus::Handler::BusDisconnect();
            BaseForce::Deactivate();
        }

    private:
        // ForcePointRequestBus
        void SetMagnitude(float magnitude) override;
        float GetMagnitude() const override;

        float m_magnitude = 1.0f;
    };

    /// Class for a spline follow force.
    /// Bodies in a force region with a spline follow force tend to follow the path of the spline.
    class ForceSplineFollow final
        : public BaseForce
        , private ForceSplineFollowRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceSplineFollow, AZ::SystemAllocator);
        AZ_RTTI(ForceSplineFollow, "{AB397D4C-62DA-43F0-8CF1-9BD9013129BB}", BaseForce);
        ForceSplineFollow() = default;
        ForceSplineFollow(float dampingRatio
            , float frequency
            , float targetSpeed
            , float lookAhead);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override;
        void Deactivate() override
        {
            ForceSplineFollowRequestBus::Handler::BusDisconnect();
            BaseForce::Deactivate();
        }

    private:
        // ForceSplineFollowRequestBus
        void SetDampingRatio(float ratio) override;
        float GetDampingRatio() const override;
        void SetFrequency(float frequency) override;
        float GetFrequency() const override;
        void SetTargetSpeed(float targetSpeed) override;
        float GetTargetSpeed() const override;
        void SetLookAhead(float lookAhead) override;
        float GetLookAhead() const override;

        float m_dampingRatio = 1.0f;
        float m_frequency = 3.0f;
        float m_targetSpeed = 1.0f;
        float m_lookAhead = 0.0f;
        bool m_loggedMissingSplineWarning = false;
    };

    /// Class for a simple drag force.
    class ForceSimpleDrag final
        : public BaseForce
        , private ForceSimpleDragRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceSimpleDrag, AZ::SystemAllocator);
        AZ_RTTI(ForceSimpleDrag, "{56A4E393-4724-4486-B4C0-E02C4EF1534C}", BaseForce);
        ForceSimpleDrag() = default;
        ForceSimpleDrag(float dragCoefficient, float volumeDensity);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BaseForce::Activate(entityId);
            ForceSimpleDragRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForceSimpleDragRequestBus::Handler::BusDisconnect();
            BaseForce::Deactivate();
        }

    private:
        // ForceSimpleDragRequests
        void SetDensity(float density) override;
        float GetDensity() const override;

        //Wikipedia: https://en.wikipedia.org/wiki/Drag_coefficient
        float m_dragCoefficient = 0.47f;
        float m_volumeDensity = 1.0f;
    };

    /// Class for a linear damping force.
    class ForceLinearDamping final
        : public BaseForce
        , private ForceLinearDampingRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ForceLinearDamping, AZ::SystemAllocator);
        AZ_RTTI(ForceLinearDamping, "{7EECFBD7-0942-4960-A54A-7582159CFFA3}", BaseForce);
        ForceLinearDamping() = default;
        explicit ForceLinearDamping(float damping);
        static void Reflect(AZ::ReflectContext* context);
        AZ::Vector3 CalculateForce(const EntityParams& entity, const RegionParams& region) const override;

        // BaseForce
        void Activate(AZ::EntityId entityId) override
        {
            BaseForce::Activate(entityId);
            ForceLinearDampingRequestBus::Handler::BusConnect(entityId);
        }
        void Deactivate() override
        {
            ForceLinearDampingRequestBus::Handler::BusDisconnect();
            BaseForce::Deactivate();
        }

    private:
        // ForceLinearDampingRequests
        void SetDamping(float damping) override;
        float GetDamping() const override;

        float m_damping = 1.0f;
    };
}
