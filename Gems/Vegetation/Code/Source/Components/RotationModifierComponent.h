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
#include <Vegetation/Ebuses/RotationModifierRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class RotationModifierConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RotationModifierConfig, AZ::SystemAllocator);
        AZ_RTTI(RotationModifierConfig, "{FF8B1DED-C1A8-4322-86D2-C8432E4B0526}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_allowOverrides = false;

        float m_rangeMinX = 0.0f;
        float m_rangeMaxX = 0.0f;
        GradientSignal::GradientSampler m_gradientSamplerX;

        float m_rangeMinY = 0.0f;
        float m_rangeMaxY = 0.0f;
        GradientSignal::GradientSampler m_gradientSamplerY;

        float m_rangeMinZ = -180.0f;
        float m_rangeMaxZ = 180.0f;
        GradientSignal::GradientSampler m_gradientSamplerZ;
    };

    inline constexpr AZ::TypeId RotationModifierComponentTypeId{ "{9C9158D1-6386-4375-8A3C-B9CA325246FB}" };

    /**
    * Component implementing VegetationModifierRequestBus that offsets rotation
    */      
    class RotationModifierComponent
        : public AZ::Component
        , public ModifierRequestBus::Handler
        , private RotationModifierRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(RotationModifierComponent, RotationModifierComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        RotationModifierComponent(const RotationModifierConfig& configuration);
        RotationModifierComponent() = default;
        ~RotationModifierComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // VegetationModifierRequestBus
        void Execute(InstanceData& instanceData) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // RotationModifierRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        AZ::Vector3 GetRangeMin() const override;
        void SetRangeMin(AZ::Vector3 rangeMin) override;
        AZ::Vector3 GetRangeMax() const override;
        void SetRangeMax(AZ::Vector3 rangeMax) override;
        GradientSignal::GradientSampler& GetGradientSamplerX() override;
        GradientSignal::GradientSampler& GetGradientSamplerY() override;
        GradientSignal::GradientSampler& GetGradientSamplerZ() override;
    private:
        RotationModifierConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
