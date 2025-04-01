/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/ReferenceShapeComponentBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;

    class ReferenceShapeConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ReferenceShapeConfig, AZ::SystemAllocator);
        AZ_RTTI(ReferenceShapeConfig, "{3E49974D-2EE0-4AF9-92B9-229A22B515C3}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_shapeEntityId;
    };

    inline constexpr AZ::TypeId ReferenceShapeComponentTypeId{ "{EB9C6DC1-900F-4CE8-AA00-81361127063A}" };

    /**
    * allows reference and reuse of shape entities
    */
    class ReferenceShapeComponent
        : public AZ::Component
        , private AZ::EntityBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private LmbrCentral::ShapeComponentRequestsBus::Handler
        , private ReferenceShapeRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ReferenceShapeComponent, ReferenceShapeComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ReferenceShapeComponent(const ReferenceShapeConfig& configuration);
        ReferenceShapeComponent() = default;
        ~ReferenceShapeComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        ////////////////////////////////////////////////////////////////////////
        // EntityEvents
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        ////////////////////////////////////////////////////////////////////////
        // LmbrCentral::ShapeComponentNotificationsBus
        void OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        //////////////////////////////////////////////////////////////////////////
        // ShapeComponentRequestsBus
        AZ::Crc32 GetShapeType() const override;
        AZ::Aabb GetEncompassingAabb() const override;
        void GetTransformAndLocalBounds(AZ::Transform& transform, AZ::Aabb& bounds) const override;
        bool IsPointInside(const AZ::Vector3& point) const override;
        float DistanceFromPoint(const AZ::Vector3& point) const override;
        float DistanceSquaredFromPoint(const AZ::Vector3& point) const override;
        AZ::Vector3 GenerateRandomPointInside(AZ::RandomDistributionType randomDistribution) const override;
        bool IntersectRay(const AZ::Vector3& src, const AZ::Vector3& dir, float& distance) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // ReferenceShapeRequestsBus
        AZ::EntityId GetShapeEntityId() const override;
        void SetShapeEntityId(AZ::EntityId entityId) override;

    private:
        ReferenceShapeConfig m_configuration;
        mutable AZStd::shared_mutex m_mutex; //!< Mutex to allow multiple readers but single writer for efficient thread safety
        bool m_allowNotifications = true; //!< temporarily disable sending notifications to avoid redundancies

        bool AllowRequest() const;
        bool AllowNotification() const;
        void SetupDependencies();
    };
}
