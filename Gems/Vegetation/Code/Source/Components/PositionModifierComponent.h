/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Vector3.h>
#include <Vegetation/Ebuses/ModifierRequestBus.h>
#include <Vegetation/Ebuses/PositionModifierRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class PositionModifierConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(PositionModifierConfig, AZ::SystemAllocator);
        AZ_RTTI(PositionModifierConfig, "{B7A0A88D-4FDF-487F-A0E6-5BE04C82862A}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_allowOverrides = false;
        bool m_autoSnapToSurface = true;
        SurfaceData::SurfaceTagVector m_surfaceTagsToSnapTo;

        float m_rangeMinX = -0.3f;
        float m_rangeMaxX = 0.3f;
        GradientSignal::GradientSampler m_gradientSamplerX;

        float m_rangeMinY = -0.3f;
        float m_rangeMaxY = 0.3f;
        GradientSignal::GradientSampler m_gradientSamplerY;

        float m_rangeMinZ = 0.0f;
        float m_rangeMaxZ = 0.0f;
        GradientSignal::GradientSampler m_gradientSamplerZ;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(AZStd::string tag);
    };

    inline constexpr AZ::TypeId PositionModifierComponentTypeId{ "{A5E84838-57D7-4F40-B011-73D9FD9AE33D}" };

    /**
    * Component implementing VegetationModifierRequestBus that offsets position
    */      
    class PositionModifierComponent
        : public AZ::Component
        , public ModifierRequestBus::Handler
        , private PositionModifierRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(PositionModifierComponent, PositionModifierComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        PositionModifierComponent(const PositionModifierConfig& configuration);
        PositionModifierComponent() = default;
        ~PositionModifierComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // VegetationModifierRequestBus
        void Execute(InstanceData& instanceData) const override;
        ModifierStage GetModifierStage() const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // PositionModifierRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        AZ::Vector3 GetRangeMin() const override;
        void SetRangeMin(AZ::Vector3 rangeMin) override;
        AZ::Vector3 GetRangeMax() const override;
        void SetRangeMax(AZ::Vector3 rangeMax) override;
        GradientSignal::GradientSampler& GetGradientSamplerX() override;
        GradientSignal::GradientSampler& GetGradientSamplerY() override;
        GradientSignal::GradientSampler& GetGradientSamplerZ() override;
        size_t GetNumTags() const override;
        AZ::Crc32 GetTag(int tagIndex) const override;
        void RemoveTag(int tagIndex) override;
        void AddTag(AZStd::string tag) override;

    private:
        PositionModifierConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;

        //reserve for masks to re-snap to surface
        mutable SurfaceData::SurfaceTagVector m_surfaceTagsToSnapToCombined;

        //point vector reserved for reuse
        mutable SurfaceData::SurfacePointList m_points;
    };
}
