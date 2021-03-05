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
        AZ_CLASS_ALLOCATOR(SlopeAlignmentModifierConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(SlopeAlignmentModifierConfig, "{73BA7B92-1061-4DDB-AA5B-A0D87303CBC8}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        bool m_allowOverrides = false;
        float m_rangeMin = 1.0f;
        float m_rangeMax = 1.0f;
        GradientSignal::GradientSampler m_gradientSampler;
    };

    static const AZ::Uuid SlopeAlignmentModifierComponentTypeId = "{08831F9F-E720-4FBD-9CC5-0EF09212B0A0}";

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