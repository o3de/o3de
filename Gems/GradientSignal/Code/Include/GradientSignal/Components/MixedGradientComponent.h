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
#include <GradientSignal/Ebuses/MixedGradientRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace LmbrCentral
{
    template<typename, typename>
    class EditorWrappedComponentBase;
}

namespace GradientSignal
{
    class MixedGradientLayer final
    {
    public:
        AZ_CLASS_ALLOCATOR(MixedGradientLayer, AZ::SystemAllocator);
        AZ_RTTI(MixedGradientLayer, "{957264F7-A169-4D47-B94C-659B078026D4}");
        static void Reflect(AZ::ReflectContext* context);

        enum class MixingOperation : AZ::u8
        {
            Initialize = 0,
            Multiply,
            Add,
            Subtract,
            Min,
            Max,
            Average,
            Normal,
            Overlay,
            Screen
        };

        bool m_enabled = true;
        MixingOperation m_operation = MixingOperation::Average;
        GradientSampler m_gradientSampler;

        const char* GetLayerEntityName() const;
    };

    class MixedGradientConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(MixedGradientConfig, AZ::SystemAllocator);
        AZ_RTTI(MixedGradientConfig, "{40403A44-31FE-4D1D-941C-6593759CCCBD}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);
        AZStd::vector<MixedGradientLayer> m_layers;

        size_t GetNumLayers() const;
        void AddLayer();
        void RemoveLayer(int layerIndex);
        MixedGradientLayer* GetLayer(int layerIndex);

        void OnLayerAdded();
    };

    inline constexpr AZ::TypeId MixedGradientComponentTypeId{ "{BB461301-D8FD-431C-9E4A-BEC6A878297C}" };

    /**
    * performs operations to combine multiple gradients
    */
    class MixedGradientComponent
        : public AZ::Component
        , private GradientRequestBus::Handler
        , private MixedGradientRequestBus::Handler
    {
    public:
        template<typename, typename> friend class LmbrCentral::EditorWrappedComponentBase;
        AZ_COMPONENT(MixedGradientComponent, MixedGradientComponentTypeId);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        MixedGradientComponent(const MixedGradientConfig& configuration);
        MixedGradientComponent() = default;
        ~MixedGradientComponent() = default;

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
        // MixedGradientRequestBus
        size_t GetNumLayers() const override;
        void AddLayer() override;
        void RemoveLayer(int layerIndex) override;
        MixedGradientLayer* GetLayer(int layerIndex) override;

    private:
        static float PerformMixingOperation(MixedGradientLayer::MixingOperation operation, float prevValue, float currentUnpremultiplied)
        {
            switch (operation)
            {
            case MixedGradientLayer::MixingOperation::Initialize:
                return currentUnpremultiplied;
            case MixedGradientLayer::MixingOperation::Multiply:
                return prevValue * currentUnpremultiplied;
            case MixedGradientLayer::MixingOperation::Screen:
                return 1.0f - ((1.0f - prevValue) * (1.0f - currentUnpremultiplied));
            case MixedGradientLayer::MixingOperation::Add:
                return prevValue + currentUnpremultiplied;
            case MixedGradientLayer::MixingOperation::Subtract:
                return prevValue - currentUnpremultiplied;
            case MixedGradientLayer::MixingOperation::Min:
                return AZStd::min(prevValue, currentUnpremultiplied);
            case MixedGradientLayer::MixingOperation::Max:
                return AZStd::max(prevValue, currentUnpremultiplied);
            case MixedGradientLayer::MixingOperation::Average:
                return (prevValue + currentUnpremultiplied) / 2.0f;
            case MixedGradientLayer::MixingOperation::Normal:
                return currentUnpremultiplied;
            case MixedGradientLayer::MixingOperation::Overlay:
                return (prevValue >= 0.5f) ? (1.0f - (2.0f * (1.0f - prevValue) * (1.0f - currentUnpremultiplied)))
                                                             : (2.0f * prevValue * currentUnpremultiplied);
            default:
                return currentUnpremultiplied;
            }
        }

        MixedGradientConfig m_configuration;
        LmbrCentral::DependencyMonitor m_dependencyMonitor;
        mutable AZStd::shared_mutex m_queryMutex;
    };
}
