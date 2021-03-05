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
#include <GradientSignal/Ebuses/ReferenceGradientRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class ReferenceGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ReferenceGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(ReferenceGradientConfig, "{121A6DAB-26C1-46B7-83AE-BE750FDABC04}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        GradientSampler m_gradientSampler;
    };

    static const AZ::Uuid ReferenceGradientComponentTypeId = "{C4904252-3386-4820-9BF7-53DE705FA644}";

    /**
    * calculates a gradient value by referencing values from another gradient
    */      
    class ReferenceGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private ReferenceGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ReferenceGradientComponent, ReferenceGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ReferenceGradientComponent(const ReferenceGradientConfig& configuration);
        ReferenceGradientComponent() = default;
        ~ReferenceGradientComponent() = default;

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
        // ReferenceGradientRequestBus
        GradientSampler& GetGradientSampler() override;

    private:
        ReferenceGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}