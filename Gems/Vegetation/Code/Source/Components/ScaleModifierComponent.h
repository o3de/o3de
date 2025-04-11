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
#include <Vegetation/Ebuses/ScaleModifierRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace Vegetation
{
    class ScaleModifierConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ScaleModifierConfig, AZ::SystemAllocator);
        AZ_RTTI(ScaleModifierConfig, "{1CD41DA9-91CA-4A57-A169-B42FC25FC4C3}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_allowOverrides = false;
        float m_rangeMin = 1.0f;
        float m_rangeMax = 1.0f;
        GradientSignal::GradientSampler m_gradientSampler;
    };

    inline constexpr AZ::TypeId ScaleModifierComponentTypeId{ "{A9F4FE60-E652-415A-A8C4-0003D5750E9E}" };

    /**
    * Component implementing VegetationModifierRequestBus that offsets scale
    */      
    class ScaleModifierComponent
        : public AZ::Component
        , public ModifierRequestBus::Handler
        , public ScaleModifierRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ScaleModifierComponent, ScaleModifierComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ScaleModifierComponent(const ScaleModifierConfig& configuration);
        ScaleModifierComponent() = default;
        ~ScaleModifierComponent() = default;

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
        // ScaleModifierRequestBus
        bool GetAllowOverrides() const override;
        void SetAllowOverrides(bool value) override;
        float GetRangeMin() const override;
        void SetRangeMin(float rangeMin) override;
        float GetRangeMax() const override;
        void SetRangeMax(float rangeMax) override;
        GradientSignal::GradientSampler& GetGradientSampler() override;


    private:
        ScaleModifierConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
