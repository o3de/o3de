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
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/ConstantGradientRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class ConstantGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ConstantGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(ConstantGradientConfig, "{B0216514-46B5-4A57-9D9D-8D9EC94C3702}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        float m_value = 1.0f;
    };

    static const AZ::Uuid ConstantGradientComponentTypeId = "{08785CA9-FD25-4036-B8A0-E0ED65C6E54B}";

    /**
    * always returns a constant value as a gradient
    */      
    class ConstantGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private ConstantGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ConstantGradientComponent, ConstantGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ConstantGradientComponent(const ConstantGradientConfig& configuration);
        ConstantGradientComponent() = default;
        ~ConstantGradientComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // ConstantGradientRequestBus
        float GetConstantValue() const override;
        void SetConstantValue(float constant) override;

    private:
        ConstantGradientConfig m_configuration;
    };
}