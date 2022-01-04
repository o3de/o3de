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
        
        entityName = "<empty>";

        AZ::EntityId layerEntityId = m_gradientSampler.m_gradientId;
        if (layerEntityId.IsValid())
        {
            AZ::ComponentApplicationBus::BroadcastResult(entityName, &AZ::ComponentApplicationRequests::GetEntityName, layerEntityId);
        }

        return entityName.c_str();
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
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void MixedGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
        services.push_back(AZ_CRC("GradientTransformService", 0x8c8c5ecc));
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
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        for (const auto& layer : m_configuration.m_layers)
        {
            m_dependencyMonitor.ConnectDependency(layer.m_gradientSampler.m_gradientId);
        }

        if (!m_configuration.m_layers.empty())
        {
            //forcing first layer to always be initialize
            m_configuration.m_layers.front().m_operation = MixedGradientLayer::MixingOperation::Initialize;
        }

        GradientRequestBus::Handler::BusConnect(GetEntityId());
        MixedGradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void MixedGradientComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        GradientRequestBus::Handler::BusDisconnect();
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
        AZ_PROFILE_FUNCTION(Entity);

        //accumulate the mixed/combined result of all layers and operations
        float result = 0.0f;
        float operationResult = 0.0f;

        for (const auto& layer : m_configuration.m_layers)
        {
            // added check to prevent opacity of 0.0, which will bust when we unpremultiply the alpha out
            if (layer.m_enabled && layer.m_gradientSampler.m_opacity != 0.0f)
            {
                // this includes leveling and opacity result, we need unpremultiplied opacity to combine properly
                float current = layer.m_gradientSampler.GetValue(sampleParams);
                // unpremultiplied alpha (we clamp the end result)
                float currentUnpremultiplied = current / layer.m_gradientSampler.m_opacity;
                switch (layer.m_operation)
                {
                default:
                case MixedGradientLayer::MixingOperation::Initialize:
                    //reset the result of the mixed/combined layers to the current value
                    result = 0.0f;
                    operationResult = currentUnpremultiplied;
                    break;
                case MixedGradientLayer::MixingOperation::Multiply:
                    operationResult = result * currentUnpremultiplied;
                    break;
                case MixedGradientLayer::MixingOperation::Add:
                    operationResult = result + currentUnpremultiplied;
                    break;
                case MixedGradientLayer::MixingOperation::Subtract:
                    operationResult = result - currentUnpremultiplied;
                    break;
                case MixedGradientLayer::MixingOperation::Min:
                    operationResult = AZStd::min(currentUnpremultiplied, result);
                    break;
                case MixedGradientLayer::MixingOperation::Max:
                    operationResult = AZStd::max(currentUnpremultiplied, result);
                    break;
                case MixedGradientLayer::MixingOperation::Average:
                    operationResult = (result + currentUnpremultiplied) / 2.0f;
                    break;
                case MixedGradientLayer::MixingOperation::Normal:
                    operationResult = currentUnpremultiplied;
                    break;
                case MixedGradientLayer::MixingOperation::Overlay:
                    operationResult = (result >= 0.5f) ? (1.0f - (2.0f * (1.0f - result) * (1.0f - currentUnpremultiplied))) : (2.0f * result * currentUnpremultiplied);
                    break;
                }
                // blend layers (re-applying opacity, which is why we needed to use unpremultiplied)
                result = (result * (1.0f - layer.m_gradientSampler.m_opacity)) + (operationResult * layer.m_gradientSampler.m_opacity);
            }
        }

        return AZ::GetClamp(result, 0.0f, 1.0f);
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
        m_configuration.AddLayer();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void MixedGradientComponent::RemoveLayer(int layerIndex)
    {
        m_configuration.RemoveLayer(layerIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    MixedGradientLayer* MixedGradientComponent::GetLayer(int layerIndex)
    {
        return m_configuration.GetLayer(layerIndex);
    }
}
