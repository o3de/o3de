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

#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <GradientSignal/GradientSampler.h>
#include <GradientSignal/SmoothStep.h>
#include <AzCore/Component/Component.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/SmoothStepGradientRequestBus.h>
#include <GradientSignal/Ebuses/SmoothStepRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class SmoothStepGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SmoothStepGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(SmoothStepGradientConfig, "{A53D2A38-FFE1-4828-B91E-4D5A8B712BB2}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        GradientSampler m_gradientSampler;
        SmoothStep m_smoothStep;
    private:
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    };

    static const AZ::Uuid SmoothStepGradientComponentTypeId = "{404BD2B5-6229-4C60-998E-77F394FF27A8}";

    /**
    * Component implementing GradientRequestBus based on smooth step
    */
    class SmoothStepGradientComponent
        : public AZ::Component
        , public GradientRequestBus::Handler
        , public SmoothStepGradientRequestBus::Handler
        , public SmoothStepRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SmoothStepGradientComponent, SmoothStepGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SmoothStepGradientComponent(const SmoothStepGradientConfig& configuration);
        SmoothStepGradientComponent() = default;
        ~SmoothStepGradientComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;
        bool IsEntityInHierarchy(const AZ::EntityId& entityId) const override;

    protected:

        //////////////////////////////////////////////////////////////////////////
        // SmoothStepRequestBus
        float GetFallOffRange() const override;
        void SetFallOffRange(float range) override;

        float GetFallOffStrength() const override;
        void SetFallOffStrength(float strength) override;

        float GetFallOffMidpoint() const override;
        void SetFallOffMidpoint(float midpoint) override;

        //////////////////////////////////////////////////////////////////////////
        // SmoothStepGradientRequestBus
        GradientSampler& GetGradientSampler() override;

    private:
        SmoothStepGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}