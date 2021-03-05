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
#include <GradientSignal/Ebuses/PosterizeGradientRequestBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class PosterizeGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(PosterizeGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(PosterizeGradientConfig, "{4AFDFD7F-384A-41DF-900C-9B25A4AA8D1E}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        enum class ModeType : AZ::u8
        {
            Ceiling = 0,
            Floor,
            Round,
            Ps,
        };
        ModeType m_mode = ModeType::Ps;
        AZ::s32 m_bands = 3;
        GradientSampler m_gradientSampler;
    };

    static const AZ::Uuid PosterizeGradientComponentTypeId = "{BDA78E8D-DEEE-477B-B1FD-11F9930322AA}";

    /**
    * calculates a gradient value by converting values from another gradient to another's range
    */      
    class PosterizeGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private PosterizeGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(PosterizeGradientComponent, PosterizeGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        PosterizeGradientComponent(const PosterizeGradientConfig& configuration);
        PosterizeGradientComponent() = default;
        ~PosterizeGradientComponent() = default;

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
        // PosterizeGradientRequestBus
        AZ::s32 GetBands() const override;
        void SetBands(AZ::s32 bands) override;
        AZ::u8 GetModeType() const override;
        void SetModeType(AZ::u8 modeType) override;

        GradientSampler& GetGradientSampler() override;

    private:
        PosterizeGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}