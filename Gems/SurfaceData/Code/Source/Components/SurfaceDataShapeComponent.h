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
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace SurfaceData
{
    class SurfaceDataShapeConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceDataShapeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(SurfaceDataShapeConfig, "{1EE196EF-8986-4A2B-B8DD-DA73F85CD597}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        SurfaceTagVector m_providerTags;
        SurfaceTagVector m_modifierTags;
    };

    class SurfaceDataShapeComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private SurfaceDataModifierRequestBus::Handler
        , private SurfaceDataProviderRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceDataShapeComponent, "{F746C7F6-EF59-45C3-AB5C-011F7AC43415}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceDataShapeComponent(const SurfaceDataShapeConfig& configuration);
        SurfaceDataShapeComponent() = default;
        ~SurfaceDataShapeComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataProviderRequestBus
        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfacePointList& surfacePointList) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceDataModifierRequestBus
        void ModifySurfacePoints(SurfacePointList& surfacePointList) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/) override;

        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeChangeReasons changeReason) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

    private:
        void OnCompositionChanged();
        void UpdateShapeData();

        SurfaceDataShapeConfig m_configuration;

        SurfaceDataRegistryHandle m_providerHandle = InvalidSurfaceDataRegistryHandle;
        SurfaceDataRegistryHandle m_modifierHandle = InvalidSurfaceDataRegistryHandle;

        // cached data
        AZStd::atomic_bool m_refresh{ false };
        mutable AZStd::recursive_mutex m_cacheMutex;
        AZ::Aabb m_shapeBounds = AZ::Aabb::CreateNull();
        bool m_shapeBoundsIsValid = false;
        static const float s_rayAABBHeightPadding;
    };
}
