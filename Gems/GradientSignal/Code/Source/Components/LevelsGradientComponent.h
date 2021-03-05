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

#include <LmbrCentral/Dependency/DependencyMonitor.h>
#include <GradientSignal/GradientSampler.h>
#include <AzCore/Component/Component.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/LevelsGradientRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class LevelsGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(LevelsGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(LevelsGradientConfig, "{02F01CCC-CA6F-462F-BDEC-9A7EAC730D33}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        GradientSampler m_gradientSampler;
        float m_inputMin = 0.0f;
        float m_inputMid = 1.0f;
        float m_inputMax = 1.0f;
        float m_outputMin = 0.0f;
        float m_outputMax = 1.0f;
    };

    static const AZ::Uuid LevelsGradientComponentTypeId = "{F8EF5F6E-6D4A-441B-A5C2-DE1775918C24}";

    /**
    * calculates a gradient value by converting values from another gradient to another's range
    */      
    class LevelsGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private LevelsGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(LevelsGradientComponent, LevelsGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        LevelsGradientComponent(const LevelsGradientConfig& configuration);
        LevelsGradientComponent() = default;
        ~LevelsGradientComponent() = default;

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
        // LevelsGradientRequestBus
        float GetInputMin() const override;
        void SetInputMin(float value) override;

        float GetInputMid() const override;
        void SetInputMid(float value) override;

        float GetInputMax() const override;
        void SetInputMax(float value) override;

        float GetOutputMin() const override;
        void SetOutputMin(float value) override;

        float GetOutputMax() const override;
        void SetOutputMax(float value) override;

        GradientSampler& GetGradientSampler() override;

    private:
        LevelsGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}