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
#include <AzCore/Component/Component.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/SurfaceAltitudeGradientRequestBus.h>
#include <SurfaceData/SurfaceDataTypes.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <AzCore/Component/TickBus.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class SurfaceAltitudeGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceAltitudeGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(SurfaceAltitudeGradientConfig, "{3CB05FC9-6E0F-435E-B420-F027B6716804}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        AZ::EntityId m_shapeEntityId;
        float m_altitudeMin = 0.0f;
        float m_altitudeMax = 128.0f;
        SurfaceData::SurfaceTagVector m_surfaceTagsToSample;

        size_t GetNumTags() const;
        AZ::Crc32 GetTag(int tagIndex) const;
        void RemoveTag(int tagIndex);
        void AddTag(AZStd::string tag);

    private:
        bool IsShapeValid() const;
    };

    static const AZ::Uuid SurfaceAltitudeGradientComponentTypeId = "{76359FA6-AD40-4DF9-81C6-F63F2632B665}";

    /**
    * Component implementing GradientRequestBus based on altitude
    */      
    class SurfaceAltitudeGradientComponent
        : public AZ::Component
        , public GradientRequestBus::Handler
        , private SurfaceAltitudeGradientRequestBus::Handler
        , private LmbrCentral::DependencyNotificationBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(SurfaceAltitudeGradientComponent, SurfaceAltitudeGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        SurfaceAltitudeGradientComponent(const SurfaceAltitudeGradientConfig& configuration);
        SurfaceAltitudeGradientComponent() = default;
        ~SurfaceAltitudeGradientComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void UpdateFromShape();

        //////////////////////////////////////////////////////////////////////////
        // GradientRequestBus
        float GetValue(const GradientSampleParams& sampleParams) const override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // SurfaceAltituteGradientRequests
        AZ::EntityId GetShapeEntityId() const override;
        void SetShapeEntityId(AZ::EntityId entityId) override;
        float GetAltitudeMin() const override;
        void SetAltitudeMin(float altitudeMin) override;
        float GetAltitudeMax() const override;
        void SetAltitudeMax(float altitudeMax) override;
        size_t GetNumTags() const override;
        AZ::Crc32 GetTag(int tagIndex) const override;
        void RemoveTag(int tagIndex) override;
        void AddTag(AZStd::string tag) override;

    private:
        mutable AZStd::recursive_mutex m_cacheMutex;
        SurfaceAltitudeGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        AZStd::atomic_bool m_dirty{ false };
    };
}