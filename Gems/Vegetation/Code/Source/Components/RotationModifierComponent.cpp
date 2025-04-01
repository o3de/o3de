/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotationModifierComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Vegetation/Descriptor.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <Vegetation/InstanceData.h>
#include <VegetationProfiler.h>

namespace Vegetation
{
    namespace RotationModifierUtil
    {
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                AZ::Vector3 rangeMin(0.0f, 0.0f, -180.0f);
                if (classElement.GetChildData(AZ_CRC_CE("RangeMin"), rangeMin))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("RangeMin"));
                    classElement.AddElementWithData(context, "RangeMinX", (float)rangeMin.GetX());
                    classElement.AddElementWithData(context, "RangeMinY", (float)rangeMin.GetY());
                    classElement.AddElementWithData(context, "RangeMinZ", (float)rangeMin.GetZ());
                }

                AZ::Vector3 rangeMax(0.0f, 0.0f, 180.0f);
                if (classElement.GetChildData(AZ_CRC_CE("RangeMax"), rangeMax))
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("RangeMax"));
                    classElement.AddElementWithData(context, "RangeMaxX", (float)rangeMax.GetX());
                    classElement.AddElementWithData(context, "RangeMaxY", (float)rangeMax.GetY());
                    classElement.AddElementWithData(context, "RangeMaxZ", (float)rangeMax.GetZ());
                }
            }
            return true;
        }
    }

    void RotationModifierConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RotationModifierConfig, AZ::ComponentConfig>()
                ->Version(1, &RotationModifierUtil::UpdateVersion)
                ->Field("AllowOverrides", &RotationModifierConfig::m_allowOverrides)
                ->Field("RangeMinX", &RotationModifierConfig::m_rangeMinX)
                ->Field("RangeMaxX", &RotationModifierConfig::m_rangeMaxX)
                ->Field("GradientX", &RotationModifierConfig::m_gradientSamplerX)
                ->Field("RangeMinY", &RotationModifierConfig::m_rangeMinY)
                ->Field("RangeMaxY", &RotationModifierConfig::m_rangeMaxY)
                ->Field("GradientY", &RotationModifierConfig::m_gradientSamplerY)
                ->Field("RangeMinZ", &RotationModifierConfig::m_rangeMinZ)
                ->Field("RangeMaxZ", &RotationModifierConfig::m_rangeMaxZ)
                ->Field("GradientZ", &RotationModifierConfig::m_gradientSamplerZ)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<RotationModifierConfig>(
                    "Vegetation Rotation Modifier", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &RotationModifierConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rotation X")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RotationModifierConfig::m_rangeMinX, "Range Min", "Minimum rotation offset on X axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RotationModifierConfig::m_rangeMaxX, "Range Max", "Maximum rotation offset on X axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                    ->DataElement(0, &RotationModifierConfig::m_gradientSamplerX, "Gradient", "Gradient used as blend factor to lerp between ranges on X axis.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rotation Y")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RotationModifierConfig::m_rangeMinY, "Range Min", "Minimum rotation offset on Y axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RotationModifierConfig::m_rangeMaxY, "Range Max", "Maximum rotation offset on Y axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                    ->DataElement(0, &RotationModifierConfig::m_gradientSamplerY, "Gradient", "Gradient used as blend factor to lerp between ranges on Y axis.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Rotation Z")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RotationModifierConfig::m_rangeMinZ, "Range Min", "Minimum rotation offset on Z axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &RotationModifierConfig::m_rangeMaxZ, "Range Max", "Maximum rotation offset on Z axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -180.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 180.0f)
                    ->DataElement(0, &RotationModifierConfig::m_gradientSamplerZ, "Gradient", "Gradient used as blend factor to lerp between ranges on Z axis.")
                    ;
            }

        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<RotationModifierConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("allowOverrides", BehaviorValueProperty(&RotationModifierConfig::m_allowOverrides))
                ->Property("rangeMinX", BehaviorValueProperty(&RotationModifierConfig::m_rangeMinX))
                ->Property("rangeMaxX", BehaviorValueProperty(&RotationModifierConfig::m_rangeMaxX))
                ->Property("gradientSamplerX", BehaviorValueProperty(&RotationModifierConfig::m_gradientSamplerX))
                ->Property("rangeMinY", BehaviorValueProperty(&RotationModifierConfig::m_rangeMinY))
                ->Property("rangeMaxY", BehaviorValueProperty(&RotationModifierConfig::m_rangeMaxY))
                ->Property("gradientSamplerY", BehaviorValueProperty(&RotationModifierConfig::m_gradientSamplerY))
                ->Property("rangeMinZ", BehaviorValueProperty(&RotationModifierConfig::m_rangeMinZ))
                ->Property("rangeMaxZ", BehaviorValueProperty(&RotationModifierConfig::m_rangeMaxZ))
                ->Property("gradientSamplerZ", BehaviorValueProperty(&RotationModifierConfig::m_gradientSamplerZ))
                ;
        }
    }

    void RotationModifierComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationModifierService"));
        services.push_back(AZ_CRC_CE("VegetationRotationModifierService"));
    }

    void RotationModifierComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationRotationModifierService"));
    }

    void RotationModifierComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("VegetationAreaService"));
    }

    void RotationModifierComponent::Reflect(AZ::ReflectContext* context)
    {
        RotationModifierConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<RotationModifierComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &RotationModifierComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("RotationModifierComponentTypeId", BehaviorConstant(RotationModifierComponentTypeId));

            behaviorContext->Class<RotationModifierComponent>()->RequestBus("RotationModifierRequestBus");

            behaviorContext->EBus<RotationModifierRequestBus>("RotationModifierRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowOverrides", &RotationModifierRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &RotationModifierRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetRangeMin", &RotationModifierRequestBus::Events::GetRangeMin)
                ->Event("SetRangeMin", &RotationModifierRequestBus::Events::SetRangeMin)
                ->VirtualProperty("RangeMin", "GetRangeMin", "SetRangeMin")
                ->Event("GetRangeMax", &RotationModifierRequestBus::Events::GetRangeMax)
                ->Event("SetRangeMax", &RotationModifierRequestBus::Events::SetRangeMax)
                ->VirtualProperty("RangeMax", "GetRangeMax", "SetRangeMax")
                ->Event("GetGradientSamplerX", &RotationModifierRequestBus::Events::GetGradientSamplerX)
                ->Event("GetGradientSamplerY", &RotationModifierRequestBus::Events::GetGradientSamplerY)
                ->Event("GetGradientSamplerZ", &RotationModifierRequestBus::Events::GetGradientSamplerZ)
                ;
        }
    }

    RotationModifierComponent::RotationModifierComponent(const RotationModifierConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void RotationModifierComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependencies({
            m_configuration.m_gradientSamplerX.m_gradientId,
            m_configuration.m_gradientSamplerY.m_gradientId,
            m_configuration.m_gradientSamplerZ.m_gradientId });
        ModifierRequestBus::Handler::BusConnect(GetEntityId());
        RotationModifierRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RotationModifierComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        ModifierRequestBus::Handler::BusDisconnect();
        RotationModifierRequestBus::Handler::BusDisconnect();
    }

    bool RotationModifierComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const RotationModifierConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool RotationModifierComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<RotationModifierConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void RotationModifierComponent::Execute(InstanceData& instanceData) const
    {
        VEGETATION_PROFILE_FUNCTION_VERBOSE

        const GradientSignal::GradientSampleParams sampleParams(instanceData.m_position);
        float factorX = m_configuration.m_gradientSamplerX.GetValue(sampleParams);
        float factorY = m_configuration.m_gradientSamplerY.GetValue(sampleParams);
        float factorZ = m_configuration.m_gradientSamplerZ.GetValue(sampleParams);

        const bool useOverrides = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->m_rotationOverrideEnabled;
        const AZ::Vector3& min = useOverrides ? instanceData.m_descriptorPtr->GetRotationMin() : GetRangeMin();
        const AZ::Vector3& max = useOverrides ? instanceData.m_descriptorPtr->GetRotationMax() : GetRangeMax();

        instanceData.m_rotation = AZ::ConvertEulerDegreesToQuaternion(AZ::Vector3(
            factorX * (max.GetX() - min.GetX()) + min.GetX(),
            factorY * (max.GetY() - min.GetY()) + min.GetY(),
            factorZ * (max.GetZ() - min.GetZ()) + min.GetZ()));
    }

    bool RotationModifierComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void RotationModifierComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 RotationModifierComponent::GetRangeMin() const
    {
        return AZ::Vector3(m_configuration.m_rangeMinX, m_configuration.m_rangeMinY, m_configuration.m_rangeMinZ);
    }

    void RotationModifierComponent::SetRangeMin(AZ::Vector3 rangeMin)
    {
        m_configuration.m_rangeMinX = rangeMin.GetX();
        m_configuration.m_rangeMinY = rangeMin.GetY();
        m_configuration.m_rangeMinZ = rangeMin.GetZ();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 RotationModifierComponent::GetRangeMax() const
    {
        return AZ::Vector3(m_configuration.m_rangeMaxX, m_configuration.m_rangeMaxY, m_configuration.m_rangeMaxZ);
    }

    void RotationModifierComponent::SetRangeMax(AZ::Vector3 rangeMax)
    {
        m_configuration.m_rangeMaxX = rangeMax.GetX();
        m_configuration.m_rangeMaxY = rangeMax.GetY();
        m_configuration.m_rangeMaxZ = rangeMax.GetZ();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSignal::GradientSampler& RotationModifierComponent::GetGradientSamplerX()
    {
        return m_configuration.m_gradientSamplerX;
    }

    GradientSignal::GradientSampler& RotationModifierComponent::GetGradientSamplerY()
    {
        return m_configuration.m_gradientSamplerY;
    }

    GradientSignal::GradientSampler& RotationModifierComponent::GetGradientSamplerZ()
    {
        return m_configuration.m_gradientSamplerZ;
    }
}
