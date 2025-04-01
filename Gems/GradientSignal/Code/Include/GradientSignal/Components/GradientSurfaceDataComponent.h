/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <GradientSignal/GradientSampler.h>
#include <GradientSignal/Ebuses/GradientSurfaceDataRequestBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class GradientSurfaceDataConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientSurfaceDataConfig, AZ::SystemAllocator);
        AZ_RTTI(GradientSurfaceDataConfig, "{34516BA4-2B13-4A84-A46B-01E1980CA778}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        float m_thresholdMin = 0.1f;
        float m_thresholdMax = 1.0f;
        SurfaceData::SurfaceTagVector m_modifierTags;
        AZ::EntityId m_shapeConstraintEntityId;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(AZStd::string tag);
    };

    inline constexpr AZ::TypeId GradientSurfaceDataComponentTypeId{ "{BE5AF9E8-C509-4A8C-8D9E-D24BCD402812}" };

    class GradientSurfaceDataComponent
        : public AZ::Component
        , private SurfaceData::SurfaceDataModifierRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
        , private LmbrCentral::ShapeComponentNotificationsBus::Handler
        , private GradientSurfaceDataRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(GradientSurfaceDataComponent, GradientSurfaceDataComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        GradientSurfaceDataComponent(const GradientSurfaceDataConfig& configuration);
        GradientSurfaceDataComponent() = default;
        ~GradientSurfaceDataComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        ////////////////////////////////////////////////////////////////////////
        // SurfaceData::SurfaceDataModifierRequestBus
        void ModifySurfacePoints(
            AZStd::span<const AZ::Vector3> positions,
            AZStd::span<const AZ::EntityId> creatorEntityIds,
            AZStd::span<SurfaceData::SurfaceTagWeights> weights) const override;

        //////////////////////////////////////////////////////////////////////////
        // LmbrCentral::DependencyNotificationBus
        void OnCompositionChanged() override;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LmbrCentral::ShapeComponentNotificationsBus
        void OnShapeChanged(LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        //////////////////////////////////////////////////////////////////////////
        // GradientSurfaceDataRequestBus
        void SetThresholdMin(float thresholdMin) override;
        float GetThresholdMin() const override;
        void SetThresholdMax(float thresholdMax) override;
        float GetThresholdMax() const override;
        size_t GetNumTags() const override;
        AZ::Crc32 GetTag(int tagIndex) const override;
        void RemoveTag(int tagIndex) override;
        void AddTag(AZStd::string tag) override;
        AZ::EntityId GetShapeConstraintEntityId() const override;
        void SetShapeConstraintEntityId(AZ::EntityId entityId) override;

    private:
        void UpdateRegistryAndCache(SurfaceData::SurfaceDataRegistryHandle& registryHandle);

        SurfaceData::SurfaceDataRegistryHandle m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        GradientSurfaceDataConfig m_configuration;
        GradientSampler m_gradientSampler;

        // cached shape constraint data that allows us to safely perform bounds tests from the vegetation
        // thread while the main thread potentially updates the bounds.
        AZStd::atomic_bool m_validShapeBounds{ false };
        mutable AZStd::recursive_mutex m_cacheMutex;
        AZ::Aabb m_cachedShapeConstraintBounds = AZ::Aabb::CreateNull();
    };
}
