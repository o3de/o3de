/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/MixedGradientComponent.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace GradientSignal
{
    void MixedGradientLayer::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MixedGradientLayer>()
                ->Version(0)
                ->Field("Enabled", &MixedGradientLayer::m_enabled)
                ->Field("Operation", &MixedGradientLayer::m_operation)
                ->Field("Gradient", &MixedGradientLayer::m_gradientSampler)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<MixedGradientLayer>(
                    "Mixed Gradient Layer", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &MixedGradientLayer::m_enabled, "Enabled", "Toggle the influence of this gradient layer.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MixedGradientLayer::m_operation, "Operation", "Function used to mix the current gradient with the previous result.")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Initialize, "Initialize")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Multiply, "Multiply")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Screen, "Screen")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Add, "Linear Dodge (Add)")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Subtract, "Subtract")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Min, "Darken (Min)")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Max, "Lighten (Max)")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Average, "Average")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Normal, "Normal")
                    ->EnumAttribute(MixedGradientLayer::MixingOperation::Overlay, "Overlay")
                    ->DataElement(0, &MixedGradientLayer::m_gradientSampler, "Gradient", "Gradient that will contribute to result of gradient mixing.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MixedGradientLayer>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Property("enabled", BehaviorValueProperty(&MixedGradientLayer::m_enabled))
                ->Property("mixingOperation",
                    [](MixedGradientLayer* config) { return (AZ::u8&)(config->m_operation); },
                    [](MixedGradientLayer* config, const AZ::u8& i) { config->m_operation = (MixedGradientLayer::MixingOperation)i; })
                ->Property("gradientSampler", BehaviorValueProperty(&MixedGradientLayer::m_gradientSampler))
                ;
        }
    }

    const char* MixedGradientLayer::GetLayerEntityName() const
    {
        // This needs to be static since the return value is used by the RPE and needs to exist long enough to set the UI data
        static AZStd::string entityName;
        static constexpr auto emptyName = "<empty>";

        AZ::EntityId layerEntityId = m_gradientSampler.m_gradientId;
        if (layerEntityId.IsValid())
        {
            AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, layerEntityId);
        }

        if (entityName.empty())
        {
            return emptyName;
        }
        else
        {
            return entityName.c_str();
        }
    }

    size_t MixedGradientConfig::GetNumLayers() const
    {
        return m_layers.size();
    }

    void MixedGradientConfig::AddLayer()
    {
        m_layers[GetNumLayers()] = MixedGradientLayer();
        OnLayerAdded();
    }

    void MixedGradientConfig::OnLayerAdded()
    {
        // The first layer should always default to "Initialize".
        if (GetNumLayers() == 1)
        {
            m_layers[0].m_operation = MixedGradientLayer::MixingOperation::Initialize;
        }
    }

    void MixedGradientConfig::RemoveLayer(int layerIndex)
    {
        if (layerIndex < m_layers.size() && layerIndex >= 0)
        {
            m_layers.erase(m_layers.begin() + layerIndex);
        }
    }

    MixedGradientLayer* MixedGradientConfig::GetLayer(int layerIndex)
    {
        if (layerIndex < m_layers.size() && layerIndex >= 0)
        {
            return &(m_layers[layerIndex]);
        }

        return nullptr;
    }

    void MixedGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        MixedGradientLayer::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MixedGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("Layers", &MixedGradientConfig::m_layers)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<MixedGradientConfig>(
                    "Mixed Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &MixedGradientConfig::m_layers, "Layers", "List of gradient mixing layers.")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, true)
                    ->Attribute(AZ::Edit::Attributes::AddNotify, &MixedGradientConfig::OnLayerAdded)
                    ->ElementAttribute(AZ::Edit::Attributes::NameLabelOverride, &MixedGradientLayer::GetLayerEntityName)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<MixedGradientConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Method("GetNumLayers", &MixedGradientConfig::GetNumLayers)
                ->Method("AddLayer", &MixedGradientConfig::AddLayer)
                ->Method("RemoveLayer", &MixedGradientConfig::RemoveLayer)
                ->Method("GetLayer", &MixedGradientConfig::GetLayer)
                ;
        }
    }

    void MixedGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void MixedGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void MixedGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void MixedGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        MixedGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MixedGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &MixedGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("MixedGradientComponentTypeId", BehaviorConstant(MixedGradientComponentTypeId));

            behaviorContext->Class<MixedGradientComponent>()->RequestBus("MixedGradientRequestBus");

            behaviorContext->EBus<MixedGradientRequestBus>("MixedGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetNumLayers", &MixedGradientRequestBus::Events::GetNumLayers)
                ->Event("AddLayer", &MixedGradientRequestBus::Events::AddLayer)
                ->Event("RemoveLayer", &MixedGradientRequestBus::Events::RemoveLayer)
                ->Event("GetLayer", &MixedGradientRequestBus::Events::GetLayer)
                ;
        }
    }

    MixedGradientComponent::MixedGradientComponent(const MixedGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void MixedGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();

        m_dependencyMonitor.SetEntityNotificationFunction(
            [this](const AZ::EntityId& ownerId, const AZ::EntityId& dependentId, const AZ::Aabb& dirtyRegion)
            {
                for (const auto& layer : m_configuration.m_layers)
                {
                    if (layer.m_enabled &&
                        (layer.m_gradientSampler.m_gradientId == dependentId) &&
                        layer.m_gradientSampler.m_opacity != 0.0f)
                    {
                        if (dirtyRegion.IsValid())
                        {
                            AZ::Aabb transformedRegion = layer.m_gradientSampler.TransformDirtyRegion(dirtyRegion);

                            LmbrCentral::DependencyNotificationBus::Event(
                                ownerId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, transformedRegion);
                        }
                        else
                        {
                            LmbrCentral::DependencyNotificationBus::Event(
                                ownerId, &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
                        }
                    }
                }
            });

        m_dependencyMonitor.ConnectOwner(GetEntityId());
        for (const auto& layer : m_configuration.m_layers)
        {
            m_dependencyMonitor.ConnectDependency(layer.m_gradientSampler.m_gradientId);
        }

        if (!m_configuration.m_layers.empty())
        {
            // Force the first layer to always be 'Initialize'
            m_configuration.m_layers.front().m_operation = MixedGradientLayer::MixingOperation::Initialize;
        }

        MixedGradientRequestBus::Handler::BusConnect(GetEntityId());

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MixedGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        MixedGradientRequestBus::Handler::BusDisconnect();
    }

    bool MixedGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const MixedGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool MixedGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<MixedGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float MixedGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        AZStd::shared_lock lock(m_queryMutex);

        // accumulate the mixed/combined result of all layers and operations
        float result = 0.0f;

        for (const auto& layer : m_configuration.m_layers)
        {
            // added check to prevent opacity of 0.0, which will bust when we unpremultiply the alpha out
            if (layer.m_enabled && layer.m_gradientSampler.m_opacity != 0.0f)
            {
                // Precalculate the inverse opacity that we'll use for blending the current accumulated value with.
                // In the one case of "Initialize" blending, force this value to 0 so that we erase any accumulated values.
                const float inverseOpacity = (layer.m_operation == MixedGradientLayer::MixingOperation::Initialize)
                    ? 0.0f
                    : (1.0f - layer.m_gradientSampler.m_opacity);

                // this includes leveling and opacity result, we need unpremultiplied opacity to combine properly
                float current = layer.m_gradientSampler.GetValue(sampleParams);
                // unpremultiplied alpha (we clamp the end result)
                const float currentUnpremultiplied = current / layer.m_gradientSampler.m_opacity;
                const float operationResult = PerformMixingOperation(layer.m_operation, result, currentUnpremultiplied);
                // blend layers (re-applying opacity, which is why we needed to use unpremultiplied)
                result = (result * inverseOpacity) + (operationResult * layer.m_gradientSampler.m_opacity);
            }
        }

        return AZ::GetClamp(result, 0.0f, 1.0f);
    }

    void MixedGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        // Initialize all of our output data to 0.0f. Layer blends will combine with this, so we need it to have an initial value.
        AZStd::fill(outValues.begin(), outValues.end(), 0.0f);

        AZStd::vector<float> layerValues(positions.size());

        // accumulate the mixed/combined result of all layers and operations
        for (const auto& layer : m_configuration.m_layers)
        {
            // added check to prevent opacity of 0.0, which will bust when we unpremultiply the alpha out
            if (layer.m_enabled && layer.m_gradientSampler.m_opacity != 0.0f)
            {
                // Precalculate the inverse opacity that we'll use for blending the current accumulated value with.
                // In the one case of "Initialize" blending, force this value to 0 so that we erase any accumulated values.
                const float inverseOpacity = (layer.m_operation == MixedGradientLayer::MixingOperation::Initialize)
                    ? 0.0f
                    : (1.0f - layer.m_gradientSampler.m_opacity);

                // this includes leveling and opacity result, we need unpremultiplied opacity to combine properly
                layer.m_gradientSampler.GetValues(positions, layerValues);

                for (size_t index = 0; index < outValues.size(); index++)
                {
                    // unpremultiplied alpha (we clamp the end result)
                    const float currentUnpremultiplied = layerValues[index] / layer.m_gradientSampler.m_opacity;
                    const float operationResult = PerformMixingOperation(layer.m_operation, outValues[index], currentUnpremultiplied);
                    // blend layers (re-applying opacity, which is why we needed to use unpremultiplied)
                    outValues[index] = (outValues[index] * inverseOpacity) + (operationResult * layer.m_gradientSampler.m_opacity);
                }
            }
        }

        for (auto& outValue : outValues)
        {
            outValue = AZ::GetClamp(outValue, 0.0f, 1.0f);
        }
    }



    bool MixedGradientComponent::IsEntityInHierarchy(const AZ::EntityId& entityId) const
    {
        for (const auto& layer : m_configuration.m_layers)
        {
            if (layer.m_gradientSampler.IsEntityInHierarchy(entityId))
            {
                return true;
            }
        }

        return false;
    }

    size_t MixedGradientComponent::GetNumLayers() const
    {
        return m_configuration.GetNumLayers();
    }

    void MixedGradientComponent::AddLayer()
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.AddLayer();
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void MixedGradientComponent::RemoveLayer(int layerIndex)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.RemoveLayer(layerIndex);
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    MixedGradientLayer* MixedGradientComponent::GetLayer(int layerIndex)
    {
        return m_configuration.GetLayer(layerIndex);
    }
}
