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

#include "Vegetation_precompiled.h"
#include "PositionModifierComponent.h"
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/sort.h>
#include <Vegetation/Descriptor.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <Vegetation/InstanceData.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <AzCore/Debug/Profiler.h>

namespace Vegetation
{
    namespace PositionModifierUtil
    {
        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                AZ::Vector3 rangeMin(-0.3f, -0.3f, 0.0f);
                if (classElement.GetChildData(AZ_CRC("RangeMin", 0xedb80851), rangeMin))
                {
                    classElement.RemoveElementByName(AZ_CRC("RangeMin", 0xedb80851));
                    classElement.AddElementWithData(context, "RangeMinX", (float)rangeMin.GetX());
                    classElement.AddElementWithData(context, "RangeMinY", (float)rangeMin.GetY());
                    classElement.AddElementWithData(context, "RangeMinZ", (float)rangeMin.GetZ());
                }

                AZ::Vector3 rangeMax(0.3f, 0.3f, 0.0f);
                if (classElement.GetChildData(AZ_CRC("RangeMax", 0xd1b53708), rangeMax))
                {
                    classElement.RemoveElementByName(AZ_CRC("RangeMax", 0xd1b53708));
                    classElement.AddElementWithData(context, "RangeMaxX", (float)rangeMax.GetX());
                    classElement.AddElementWithData(context, "RangeMaxY", (float)rangeMax.GetY());
                    classElement.AddElementWithData(context, "RangeMaxZ", (float)rangeMax.GetZ());
                }
            }
            return true;
        }
    }

    void PositionModifierConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PositionModifierConfig, AZ::ComponentConfig>()
                ->Version(1, &PositionModifierUtil::UpdateVersion)
                ->Field("AllowOverrides", &PositionModifierConfig::m_allowOverrides)
                ->Field("AutoSnapToSurface", &PositionModifierConfig::m_autoSnapToSurface)
                ->Field("SurfacesToSnapTo", &PositionModifierConfig::m_surfaceTagsToSnapTo)
                ->Field("RangeMinX", &PositionModifierConfig::m_rangeMinX)
                ->Field("RangeMaxX", &PositionModifierConfig::m_rangeMaxX)
                ->Field("GradientX", &PositionModifierConfig::m_gradientSamplerX)
                ->Field("RangeMinY", &PositionModifierConfig::m_rangeMinY)
                ->Field("RangeMaxY", &PositionModifierConfig::m_rangeMaxY)
                ->Field("GradientY", &PositionModifierConfig::m_gradientSamplerY)
                ->Field("RangeMinZ", &PositionModifierConfig::m_rangeMinZ)
                ->Field("RangeMaxZ", &PositionModifierConfig::m_rangeMaxZ)
                ->Field("GradientZ", &PositionModifierConfig::m_gradientSamplerZ)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<PositionModifierConfig>(
                    "Vegetation Position Modifier", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PositionModifierConfig::m_allowOverrides, "Allow Per-Item Overrides", "Allow per-descriptor parameters to override component parameters.")

                    ->DataElement(0, &PositionModifierConfig::m_autoSnapToSurface, "Auto Snap To Surface", "Automatically snap to the surface closest to the new position using Surface Tags To Snap To plus the initial surface tags.")
                    ->DataElement(0, &PositionModifierConfig::m_surfaceTagsToSnapTo, "Surface Tags To Snap To", "Additional surface tags to snap to if auto snap is enabled.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Position X")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PositionModifierConfig::m_rangeMinX, "Range Min", "Minimum position offset on X axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PositionModifierConfig::m_rangeMaxX, "Range Max", "Maximum position offset on X axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                    ->DataElement(0, &PositionModifierConfig::m_gradientSamplerX, "Gradient", "Gradient used as blend factor to lerp between ranges on X axis.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Position Y")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PositionModifierConfig::m_rangeMinY, "Range Min", "Minimum position offset on Y axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PositionModifierConfig::m_rangeMaxY, "Range Max", "Maximum position offset on Y axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                    ->DataElement(0, &PositionModifierConfig::m_gradientSamplerY, "Gradient", "Gradient used as blend factor to lerp between ranges on Y axis.")

                    ->ClassElement(AZ::Edit::ClassElements::Group, "Position Z")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PositionModifierConfig::m_rangeMinZ, "Range Min", "Minimum position offset on Z axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &PositionModifierConfig::m_rangeMaxZ, "Range Max", "Maximum position offset on Z axis.")
                    ->Attribute(AZ::Edit::Attributes::Min, std::numeric_limits<float>::lowest())
                    ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
                    ->Attribute(AZ::Edit::Attributes::SoftMin, -2.0f)
                    ->Attribute(AZ::Edit::Attributes::SoftMax, 2.0f)
                    ->DataElement(0, &PositionModifierConfig::m_gradientSamplerZ, "Gradient", "Gradient used as blend factor to lerp between ranges on Z axis.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PositionModifierConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("allowOverrides", BehaviorValueProperty(&PositionModifierConfig::m_allowOverrides))
                ->Property("rangeMinX", BehaviorValueProperty(&PositionModifierConfig::m_rangeMinX))
                ->Property("rangeMaxX", BehaviorValueProperty(&PositionModifierConfig::m_rangeMaxX))
                ->Property("gradientSamplerX", BehaviorValueProperty(&PositionModifierConfig::m_gradientSamplerX))
                ->Property("rangeMinY", BehaviorValueProperty(&PositionModifierConfig::m_rangeMinY))
                ->Property("rangeMaxY", BehaviorValueProperty(&PositionModifierConfig::m_rangeMaxY))
                ->Property("gradientSamplerY", BehaviorValueProperty(&PositionModifierConfig::m_gradientSamplerY))
                ->Property("rangeMinZ", BehaviorValueProperty(&PositionModifierConfig::m_rangeMinZ))
                ->Property("rangeMaxZ", BehaviorValueProperty(&PositionModifierConfig::m_rangeMaxZ))
                ->Property("gradientSamplerZ", BehaviorValueProperty(&PositionModifierConfig::m_gradientSamplerZ))
                ->Method("GetNumTags", &PositionModifierConfig::GetNumTags)
                ->Method("GetTag", &PositionModifierConfig::GetTag)
                ->Method("RemoveTag", &PositionModifierConfig::RemoveTag)
                ->Method("AddTag", &PositionModifierConfig::AddTag)
                ;
        }
    }

    size_t PositionModifierConfig::GetNumTags() const
    {
        return m_surfaceTagsToSnapTo.size();
    }

    AZ::Crc32 PositionModifierConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_surfaceTagsToSnapTo.size())
        {
            return m_surfaceTagsToSnapTo[tagIndex];
        }

        return AZ::Crc32();
    }

    void PositionModifierConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_surfaceTagsToSnapTo.size())
        {
            m_surfaceTagsToSnapTo.erase(m_surfaceTagsToSnapTo.begin() + tagIndex);
        }
    }

    void PositionModifierConfig::AddTag(AZStd::string tag)
    {
        m_surfaceTagsToSnapTo.push_back(SurfaceData::SurfaceTag(tag));
    }

    void PositionModifierComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationModifierService", 0xc551fca6));
        services.push_back(AZ_CRC("VegetationPositionModifierService", 0xe7d7293e));
    }

    void PositionModifierComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationPositionModifierService", 0xe7d7293e));
    }

    void PositionModifierComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaService", 0x6a859504));
    }

    void PositionModifierComponent::Reflect(AZ::ReflectContext* context)
    {
        PositionModifierConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<PositionModifierComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &PositionModifierComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("PositionModifierComponentTypeId", BehaviorConstant(PositionModifierComponentTypeId));

            behaviorContext->Class<PositionModifierComponent>()->RequestBus("PositionModifierRequestBus");

            behaviorContext->EBus<PositionModifierRequestBus>("PositionModifierRequestBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetAllowOverrides", &PositionModifierRequestBus::Events::GetAllowOverrides)
                ->Event("SetAllowOverrides", &PositionModifierRequestBus::Events::SetAllowOverrides)
                ->VirtualProperty("AllowOverrides", "GetAllowOverrides", "SetAllowOverrides")
                ->Event("GetRangeMin", &PositionModifierRequestBus::Events::GetRangeMin)
                ->Event("SetRangeMin", &PositionModifierRequestBus::Events::SetRangeMin)
                ->VirtualProperty("RangeMin", "GetRangeMin", "SetRangeMin")
                ->Event("GetRangeMax", &PositionModifierRequestBus::Events::GetRangeMax)
                ->Event("SetRangeMax", &PositionModifierRequestBus::Events::SetRangeMax)
                ->VirtualProperty("RangeMax", "GetRangeMax", "SetRangeMax")
                ->Event("GetGradientSamplerX", &PositionModifierRequestBus::Events::GetGradientSamplerX)
                ->Event("GetGradientSamplerY", &PositionModifierRequestBus::Events::GetGradientSamplerY)
                ->Event("GetGradientSamplerZ", &PositionModifierRequestBus::Events::GetGradientSamplerZ)
                ->Event("GetNumTags", &PositionModifierRequestBus::Events::GetNumTags)
                ->Event("GetTag", &PositionModifierRequestBus::Events::GetTag)
                ->Event("RemoveTag", &PositionModifierRequestBus::Events::RemoveTag)
                ->Event("AddTag", &PositionModifierRequestBus::Events::AddTag)
                ;
        }
    }

    PositionModifierComponent::PositionModifierComponent(const PositionModifierConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void PositionModifierComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependencies({
            m_configuration.m_gradientSamplerX.m_gradientId,
            m_configuration.m_gradientSamplerY.m_gradientId,
            m_configuration.m_gradientSamplerZ.m_gradientId });
        ModifierRequestBus::Handler::BusConnect(GetEntityId());
        PositionModifierRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PositionModifierComponent::Deactivate()
    {
        m_dependencyMonitor.Reset();
        ModifierRequestBus::Handler::BusDisconnect();
        PositionModifierRequestBus::Handler::BusDisconnect();
    }

    bool PositionModifierComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const PositionModifierConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool PositionModifierComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<PositionModifierConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void PositionModifierComponent::Execute(InstanceData& instanceData) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        const GradientSignal::GradientSampleParams sampleParams(instanceData.m_position);
        float factorX = m_configuration.m_gradientSamplerX.GetValue(sampleParams);
        float factorY = m_configuration.m_gradientSamplerY.GetValue(sampleParams);
        float factorZ = m_configuration.m_gradientSamplerZ.GetValue(sampleParams);

        const bool useOverrides = m_configuration.m_allowOverrides && instanceData.m_descriptorPtr && instanceData.m_descriptorPtr->m_positionOverrideEnabled;
        const AZ::Vector3& min = useOverrides ? instanceData.m_descriptorPtr->GetPositionMin() : GetRangeMin();
        const AZ::Vector3& max = useOverrides ? instanceData.m_descriptorPtr->GetPositionMax() : GetRangeMax();
        const AZ::Vector3 delta = min + AZ::Vector3(
            (max.GetX() - min.GetX()) * factorX,
            (max.GetY() - min.GetY()) * factorY,
            (max.GetZ() - min.GetZ()) * factorZ);
        const AZ::Vector3 deltaXY(delta.GetX(), delta.GetY(), 0.0f);

        instanceData.m_position += deltaXY;

        //auto snapping to surface if change occurred on XY axis
        if (m_configuration.m_autoSnapToSurface && !deltaXY.IsClose(AZ::Vector3::CreateZero()))
        {
            //get the combined set of masks to consider for snapping
            m_surfaceTagsToSnapToCombined.clear();
            m_surfaceTagsToSnapToCombined.reserve(
                m_configuration.m_surfaceTagsToSnapTo.size() +
                instanceData.m_masks.size());

            m_surfaceTagsToSnapToCombined.insert(m_surfaceTagsToSnapToCombined.end(),
                m_configuration.m_surfaceTagsToSnapTo.begin(), m_configuration.m_surfaceTagsToSnapTo.end());

            for (const auto& maskPair : instanceData.m_masks)
            {
                m_surfaceTagsToSnapToCombined.push_back(maskPair.first);
            }

            //get the intersection data at the new position
            m_points.clear();
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePoints, instanceData.m_position, m_surfaceTagsToSnapToCombined, m_points);
            if (!m_points.empty())
            {
                //sort the intersection data by distance from the new position in case there are multiple intersections at different or unrelated heights
                AZStd::sort(m_points.begin(), m_points.end(), [&instanceData](const SurfaceData::SurfacePoint& a, const SurfaceData::SurfacePoint& b)
                {
                    return a.m_position.GetDistanceSq(instanceData.m_position) < b.m_position.GetDistanceSq(instanceData.m_position);
                });

                instanceData.m_position = m_points[0].m_position;
                instanceData.m_normal = m_points[0].m_normal;
                instanceData.m_masks = m_points[0].m_masks;
            }
        }

        instanceData.m_position.SetZ(instanceData.m_position.GetZ() + delta.GetZ());
    }

    ModifierStage PositionModifierComponent::GetModifierStage() const
    {
        return ModifierStage::PreProcess;
    }

    bool PositionModifierComponent::GetAllowOverrides() const
    {
        return m_configuration.m_allowOverrides;
    }

    void PositionModifierComponent::SetAllowOverrides(bool value)
    {
        m_configuration.m_allowOverrides = value;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 PositionModifierComponent::GetRangeMin() const
    {
        return AZ::Vector3(m_configuration.m_rangeMinX, m_configuration.m_rangeMinY, m_configuration.m_rangeMinZ);
    }

    void PositionModifierComponent::SetRangeMin(AZ::Vector3 rangeMin)
    {
        m_configuration.m_rangeMinX = rangeMin.GetX();
        m_configuration.m_rangeMinY = rangeMin.GetY();
        m_configuration.m_rangeMinZ = rangeMin.GetZ();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::Vector3 PositionModifierComponent::GetRangeMax() const
    {
        return AZ::Vector3(m_configuration.m_rangeMaxX, m_configuration.m_rangeMaxY, m_configuration.m_rangeMaxZ);
    }

    void PositionModifierComponent::SetRangeMax(AZ::Vector3 rangeMax)
    {
        m_configuration.m_rangeMaxX = rangeMax.GetX();
        m_configuration.m_rangeMaxY = rangeMax.GetY();
        m_configuration.m_rangeMaxZ = rangeMax.GetZ();
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    GradientSignal::GradientSampler& PositionModifierComponent::GetGradientSamplerX()
    {
        return m_configuration.m_gradientSamplerX;
    }

    GradientSignal::GradientSampler& PositionModifierComponent::GetGradientSamplerY()
    {
        return m_configuration.m_gradientSamplerY;
    }

    GradientSignal::GradientSampler& PositionModifierComponent::GetGradientSamplerZ()
    {
        return m_configuration.m_gradientSamplerZ;
    }

    size_t PositionModifierComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 PositionModifierComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void PositionModifierComponent::RemoveTag(int tagIndex)
    {
        m_configuration.RemoveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void PositionModifierComponent::AddTag(AZStd::string tag)
    {
        m_configuration.AddTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}