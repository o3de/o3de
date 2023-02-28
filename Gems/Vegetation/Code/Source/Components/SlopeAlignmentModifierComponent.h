/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Vegetation/Ebuses/ModifierRequestBus.h>
#include <Vegetation/Ebuses/SlopeAlignmentModifierRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class SlopeAlignmentModifierConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SlopeAlignmentModifierConfig, AZ::SystemAllocator);
        AZ_RTTI(SlopeAlignmentModifierConfig, "{73BA7B92-1061-4DDB-AA5B-A0D87303CBC8}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_allowOverrides = false;
        float m_rangeMin = 1.0f;
        float m_rangeMax = 1.0f;
        GradientSignal::GradientSampler m_gradientSampler;
    };

    inline constexpr AZ::TypeId SlopeAlignmentModifierComponentTypeId{ "{08831F9F-E720-4FBD-9CC5-0EF09212B0A0}" };

    /**
    * Component implementing VegetationModifierRequestBus that alignsto slope
    */      
    class SlopeAlignmentModifierComponent
        : public AZ::Component
        , public ModifierRequestBus::Handler
        , private SlopeAlignmentModifierRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SlopeAlignmentModifierComponent, SlopeAlignmentModifierComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SlopeAlignmentModifierComponent(const SlopeAlignmentModifierConfig& configuration);
        SlopeAlignmentModifierComponent() = default;
        ~SlopeAlignmentModifierComponent() = default;

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
        // SlopeAlignmentModifierRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        float GetRangeMin() const override;
        void SetRangeMin(float rangeMin) override;
        float GetRangeMax() const override;
        void SetRangeMax(float rangeMax) override;
        GradientSignal::GradientSampler& GetGradientSampler() override;

    private:
        SlopeAlignmentModifierConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
