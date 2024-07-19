/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/SurfaceMaskGradientComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <AzCore/Debug/Profiler.h>

namespace GradientSignal
{
    void SurfaceMaskGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("SurfaceTagList", &SurfaceMaskGradientConfig::m_surfaceTagList)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceMaskGradientConfig>(
                    "Surface Mask Gradient", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceMaskGradientConfig::m_surfaceTagList, "Surface Tag List", "Identifiers used to compare against underlying surfaces.")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceMaskGradientConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Method("GetNumTags", &SurfaceMaskGradientConfig::GetNumTags)
                ->Method("GetTag", &SurfaceMaskGradientConfig::GetTag)
                ->Method("RemoveTag", &SurfaceMaskGradientConfig::RemoveTag)
                ->Method("AddTag", &SurfaceMaskGradientConfig::AddTag)
                ;
        }
    }

    size_t SurfaceMaskGradientConfig::GetNumTags() const
    {
        return m_surfaceTagList.size();
    }

    AZ::Crc32 SurfaceMaskGradientConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_surfaceTagList.size())
        {
            return m_surfaceTagList[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceMaskGradientConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_surfaceTagList.size())
        {
            m_surfaceTagList.erase(m_surfaceTagList.begin() + tagIndex);
        }
    }

    void SurfaceMaskGradientConfig::AddTag(AZStd::string tag)
    {
        m_surfaceTagList.push_back(SurfaceData::SurfaceTag(tag));
    }

    void SurfaceMaskGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void SurfaceMaskGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void SurfaceMaskGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceMaskGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceMaskGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceMaskGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceMaskGradientComponentTypeId", BehaviorConstant(SurfaceMaskGradientComponentTypeId));

            behaviorContext->Class<SurfaceMaskGradientComponent>()->RequestBus("SurfaceMaskGradientRequestBus");

            behaviorContext->EBus<SurfaceMaskGradientRequestBus>("SurfaceMaskGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetNumTags", &SurfaceMaskGradientRequestBus::Events::GetNumTags)
                ->Event("GetTag", &SurfaceMaskGradientRequestBus::Events::GetTag)
                ->Event("RemoveTag", &SurfaceMaskGradientRequestBus::Events::RemoveTag)
                ->Event("AddTag", &SurfaceMaskGradientRequestBus::Events::AddTag)
                ;
        }
    }

    SurfaceMaskGradientComponent::SurfaceMaskGradientComponent(const SurfaceMaskGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceMaskGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.SetRegionChangedEntityNotificationFunction();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        SurfaceMaskGradientRequestBus::Handler::BusConnect(GetEntityId());

        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusConnect();

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceMaskGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        SurfaceMaskGradientRequestBus::Handler::BusDisconnect();
    }

    bool SurfaceMaskGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceMaskGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceMaskGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceMaskGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float SurfaceMaskGradientComponent::GetValue(const GradientSampleParams& params) const
    {
        float result = 0.0f;
        GetValues(AZStd::span<const AZ::Vector3>(&params.m_position, 1), AZStd::span<float>(&result, 1));
        return result;
    }

    void SurfaceMaskGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
    {
        if (positions.size() != outValues.size())
        {
            AZ_Assert(false, "input and output lists are different sizes (%zu vs %zu).", positions.size(), outValues.size());
            return;
        }

        if (GradientRequestBus::HasReentrantEBusUseThisThread())
        {
            AZ_ErrorOnce("GradientSignal", false, "Detected cyclic dependencies with surface tag references on entity '%s' (%s)",
                GetEntity()->GetName().c_str(), GetEntityId().ToString().c_str());
            return;
        }

        AZStd::shared_lock lock(m_queryMutex);

        // Initialize all our output values to 0.
        AZStd::fill(outValues.begin(), outValues.end(), 0.0f);

        if (!m_configuration.m_surfaceTagList.empty())
        {
            SurfaceData::SurfacePointList points;
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromList(
                positions, m_configuration.m_surfaceTagList, points);

            // For each position, get the max surface weight that matches our filter and that appears at that position.
            points.EnumeratePoints(
                [this, &outValues](
                    size_t inPositionIndex, [[maybe_unused]] const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& normal,
                    const SurfaceData::SurfaceTagWeights& masks) -> bool
                {
                    masks.EnumerateWeights(
                        [this, inPositionIndex, &outValues]([[maybe_unused]] AZ::Crc32 surfaceType, float weight) -> bool
                        {
                            if (AZStd::find(
                                    m_configuration.m_surfaceTagList.begin(), m_configuration.m_surfaceTagList.end(), surfaceType) !=
                                m_configuration.m_surfaceTagList.end())
                            {
                                outValues[inPositionIndex] = AZ::GetMax(AZ::GetClamp(weight, 0.0f, 1.0f), outValues[inPositionIndex]);
                            }
                            return true;
                        });
                    return true;
                });
        }
    }

    size_t SurfaceMaskGradientComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 SurfaceMaskGradientComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void SurfaceMaskGradientComponent::RemoveTag(int tagIndex)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.RemoveTag(tagIndex);
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceMaskGradientComponent::AddTag(AZStd::string tag)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.AddTag(tag);
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceMaskGradientComponent::OnSurfaceChanged(
        [[maybe_unused]] const AZ::EntityId& entityId,
        [[maybe_unused]] const AZ::Aabb& oldBounds,
        [[maybe_unused]] const AZ::Aabb& newBounds,
        const SurfaceData::SurfaceTagSet& changedSurfaceTags)
    {
        bool changedTagAffectsGradient = false;

        // Only hold the lock while we're comparing the surface tags. Don't hold onto it during the OnCompositionChanged call,
        // because that can execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::shared_lock lock(m_queryMutex);
            for (auto& tag : m_configuration.m_surfaceTagList)
            {
                if (changedSurfaceTags.contains(tag))
                {
                    changedTagAffectsGradient = true;
                    break;
                }
            }
        }

        if (changedTagAffectsGradient)
        {
            AZ::Aabb expandedBounds(oldBounds);
            expandedBounds.AddAabb(newBounds);

            LmbrCentral::DependencyNotificationBus::Event(
                GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionRegionChanged, expandedBounds);
        }
    }

}
