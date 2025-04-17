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
        AZ_CLASS_ALLOCATOR(ConstantGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(ConstantGradientConfig, "{B0216514-46B5-4A57-9D9D-8D9EC94C3702}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        float m_value = 1.0f;
    };

    inline constexpr AZ::TypeId ConstantGradientComponentTypeId{ "{08785CA9-FD25-4036-B8A0-E0ED65C6E54B}" };

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
        void GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // ConstantGradientRequestBus
        float GetConstantValue() const override;
        void SetConstantValue(float constant) override;

    private:
        ConstantGradientConfig m_configuration;
        mutable AZStd::shared_mutex m_queryMutex;
    };
}
