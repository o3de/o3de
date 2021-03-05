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
#include <GradientSignal/GradientSampler.h>
#include <GradientSignal/Ebuses/ShapeAreaFalloffGradientRequestBus.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class ShapeAreaFalloffGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeAreaFalloffGradientConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(ShapeAreaFalloffGradientConfig, "{8FB7C786-D8A7-41C4-A703-020020EB4A4F}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        AZ::EntityId m_shapeEntityId;
        float m_falloffWidth = 1.0f;

        FalloffType m_falloffType = FalloffType::Outer;
    };

    static const AZ::Uuid ShapeAreaFalloffGradientComponentTypeId = "{F32A108B-7612-4AC2-B436-96DDDCE9E70B}";

    /**
    * calculates a gradient value based on distance from a shapes surface
    */      
    class ShapeAreaFalloffGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private ShapeAreaFalloffGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(ShapeAreaFalloffGradientComponent, ShapeAreaFalloffGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        ShapeAreaFalloffGradientComponent(const ShapeAreaFalloffGradientConfig& configuration);
        ShapeAreaFalloffGradientComponent() = default;
        ~ShapeAreaFalloffGradientComponent() = default;

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
        // ShapeAreaFalloffGradientRequestBus
        AZ::EntityId GetShapeEntityId() const override;
        void SetShapeEntityId(AZ::EntityId entityId) override;

        float GetFalloffWidth() const override;
        void SetFalloffWidth(float falloffWidth) override;

        FalloffType GetFalloffType() const override;
        void SetFalloffType(FalloffType type) override;

    private:
        ShapeAreaFalloffGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
    };
}