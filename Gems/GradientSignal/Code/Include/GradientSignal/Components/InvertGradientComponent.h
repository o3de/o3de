/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <GradientSignal/GradientSampler.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/InvertGradientRequestBus.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class InvertGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(InvertGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(InvertGradientConfig, "{1A4C0EF2-BF98-4EB3-B134-A6EF7B31B62E}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        GradientSampler m_gradientSampler;
    };

    inline constexpr AZ::TypeId InvertGradientComponentTypeId{ "{FAE8B7AF-5D02-4DE4-860F-1DA31A7FE144}" };

    /**
    * calculates a gradient value by inverting values from another gradient
    */      
    class InvertGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private InvertGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(InvertGradientComponent, InvertGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        InvertGradientComponent(const InvertGradientConfig& configuration);
        InvertGradientComponent() = default;
        ~InvertGradientComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;
        bool IsEntityInHierarchy(const AZ::EntityId& entityId) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // DitheredGradientRequestBus
        GradientSampler& GetGradientSampler() override;

    private:
        InvertGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}
