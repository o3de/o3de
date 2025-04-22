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
#include <GradientSignal/Ebuses/SmoothStepRequestBus.h>
#include <GradientSignal/Ebuses/SurfaceSlopeGradientRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <SurfaceData/SurfacePointList.h>
#include <GradientSignal/SmoothStep.h>
#include <GradientSignal/Util.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class SurfaceSlopeGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        enum class RampType : AZ::u8
        {
            LINEAR_RAMP_DOWN,
            LINEAR_RAMP_UP,
            SMOOTH_STEP
        };

        AZ_CLASS_ALLOCATOR(SurfaceSlopeGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(SurfaceSlopeGradientConfig, "{691E0F23-37E9-434F-A1D1-E8DE5B4A3405}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        float m_slopeMin = 0.0f;
        float m_slopeMax = 20.0f;
        SurfaceData::SurfaceTagVector m_surfaceTagsToSample;
        RampType m_rampType = RampType::LINEAR_RAMP_DOWN;
        SmoothStep m_smoothStep;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(AZStd::string tag);
 
        bool IsSmoothStepReadOnly() const
        {
            return (m_rampType != RampType::SMOOTH_STEP);
        }
        AZ::Crc32 GetSmoothStepParameterVisibility() const
        {
            return (m_rampType == RampType::SMOOTH_STEP) ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
        }
    };

    inline constexpr AZ::TypeId SurfaceSlopeGradientComponentTypeId{ "{F480A866-6296-4F2D-B97C-E80C7409EF61}" };

    /**
    * Component implementing GradientRequestBus based on slope
    */      
    class SurfaceSlopeGradientComponent
        : public AZ::Component
        , public GradientRequestBus::Handler
        , public SurfaceSlopeGradientRequestBus::Handler
        , public SmoothStepRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceSlopeGradientComponent, SurfaceSlopeGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceSlopeGradientComponent(const SurfaceSlopeGradientConfig& configuration);
        SurfaceSlopeGradientComponent() = default;
        ~SurfaceSlopeGradientComponent() = default;

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
        // SurfaceSlopeGradientRequestBus
        float GetSlopeMin() const override;
        void SetSlopeMin(float slopeMin) override;

        float GetSlopeMax() const override;
        void SetSlopeMax(float slopeMax) override;

        size_t GetNumTags() const override;
        AZ::Crc32 GetTag(int tagIndex) const override;
        void RemoveTag(int tagIndex) override;
        void AddTag(AZStd::string tag) override;

        AZ::u8 GetRampType() const override;
        void SetRampType(AZ::u8 rampType) override;

        //////////////////////////////////////////////////////////////////////////
        // SmoothStepRequestBus
        float GetFallOffRange() const override;
        void SetFallOffRange(float range) override;

        float GetFallOffStrength() const override;
        void SetFallOffStrength(float strength) override;

        float GetFallOffMidpoint() const override;
        void SetFallOffMidpoint(float midpoint) override;

    private:
        SurfaceSlopeGradientConfig m_configuration;
        mutable AZStd::shared_mutex m_queryMutex;
    };
}
