/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/SurfaceAltitudeGradientComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <GradientSignal/Util.h>
#include <LmbrCentral/Dependency/DependencyMonitor.h>

namespace GradientSignal
{
    void SurfaceAltitudeGradientConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceAltitudeGradientConfig, AZ::ComponentConfig>()
                ->Version(0)
                ->Field("ShapeEntityId", &SurfaceAltitudeGradientConfig::m_shapeEntityId)
                ->Field("AltitudeMin", &SurfaceAltitudeGradientConfig::m_altitudeMin)
                ->Field("AltitudeMax", &SurfaceAltitudeGradientConfig::m_altitudeMax)
                ->Field("SurfaceTagsToSample", &SurfaceAltitudeGradientConfig::m_surfaceTagsToSample)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SurfaceAltitudeGradientConfig>(
                    "Altitude Gradient", "altitude Gradient")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_shapeEntityId, "Pin To Shape Entity Id", "Shape bounds override min/max altitude if specified.")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("ShapeService"))
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_altitudeMin, "Altitude Min", "Minimum acceptable surface altitude.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &SurfaceAltitudeGradientConfig::IsShapeValid)
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_altitudeMax, "Altitude Max", "Maximum acceptable surface altitude.")
                    ->Attribute(AZ::Edit::Attributes::ReadOnly, &SurfaceAltitudeGradientConfig::IsShapeValid)
                    ->DataElement(0, &SurfaceAltitudeGradientConfig::m_surfaceTagsToSample, "Surface Tags to track", "")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfaceAltitudeGradientConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("shapeEntityId", BehaviorValueProperty(&SurfaceAltitudeGradientConfig::m_shapeEntityId))
                ->Property("altitudeMin", BehaviorValueProperty(&SurfaceAltitudeGradientConfig::m_altitudeMin))
                ->Property("altitudeMax", BehaviorValueProperty(&SurfaceAltitudeGradientConfig::m_altitudeMax))
                ->Method("GetNumTags", &SurfaceAltitudeGradientConfig::GetNumTags)
                ->Method("GetTag", &SurfaceAltitudeGradientConfig::GetTag)
                ->Method("RemoveTag", &SurfaceAltitudeGradientConfig::RemoveTag)
                ->Method("AddTag", &SurfaceAltitudeGradientConfig::AddTag)
                ;
        }
    }

    bool SurfaceAltitudeGradientConfig::IsShapeValid() const
    {
        return m_shapeEntityId.IsValid();
    }

    size_t SurfaceAltitudeGradientConfig::GetNumTags() const
    {
        return m_surfaceTagsToSample.size();
    }

    AZ::Crc32 SurfaceAltitudeGradientConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_surfaceTagsToSample.size() && tagIndex >= 0)
        {
            return m_surfaceTagsToSample[tagIndex];
        }

        return AZ::Crc32();
    }

    void SurfaceAltitudeGradientConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_surfaceTagsToSample.size() && tagIndex >= 0)
        {
            m_surfaceTagsToSample.erase(m_surfaceTagsToSample.begin() + tagIndex);
        }
    }

    void SurfaceAltitudeGradientConfig::AddTag(AZStd::string tag)
    {
        m_surfaceTagsToSample.push_back(SurfaceData::SurfaceTag(tag));
    }

    void SurfaceAltitudeGradientComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void SurfaceAltitudeGradientComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
        services.push_back(AZ_CRC_CE("GradientTransformService"));
    }

    void SurfaceAltitudeGradientComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& services)
    {
    }

    void SurfaceAltitudeGradientComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceAltitudeGradientConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SurfaceAltitudeGradientComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &SurfaceAltitudeGradientComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SurfaceAltitudeGradientComponentTypeId", BehaviorConstant(SurfaceAltitudeGradientComponentTypeId));

            behaviorContext->Class<SurfaceAltitudeGradientComponent>()->RequestBus("SurfaceAltitudeGradientRequestBus");

            behaviorContext->EBus<SurfaceAltitudeGradientRequestBus>("SurfaceAltitudeGradientRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetShapeEntityId", &SurfaceAltitudeGradientRequestBus::Events::GetShapeEntityId)
                ->Event("SetShapeEntityId", &SurfaceAltitudeGradientRequestBus::Events::SetShapeEntityId)
                ->VirtualProperty("ShapeEntityId", "GetShapeEntityId", "SetShapeEntityId")
                ->Event("GetAltitudeMin", &SurfaceAltitudeGradientRequestBus::Events::GetAltitudeMin)
                ->Event("SetAltitudeMin", &SurfaceAltitudeGradientRequestBus::Events::SetAltitudeMin)
                ->VirtualProperty("AltitudeMin", "GetAltitudeMin", "SetAltitudeMin")
                ->Event("GetAltitudeMax", &SurfaceAltitudeGradientRequestBus::Events::GetAltitudeMax)
                ->Event("SetAltitudeMax", &SurfaceAltitudeGradientRequestBus::Events::SetAltitudeMax)
                ->VirtualProperty("AltitudeMax", "GetAltitudeMax", "SetAltitudeMax")
                ->Event("GetNumTags", &SurfaceAltitudeGradientRequestBus::Events::GetNumTags)
                ->Event("GetTag", &SurfaceAltitudeGradientRequestBus::Events::GetTag)
                ->Event("RemoveTag", &SurfaceAltitudeGradientRequestBus::Events::RemoveTag)
                ->Event("AddTag", &SurfaceAltitudeGradientRequestBus::Events::AddTag)
                ;
        }
    }

    SurfaceAltitudeGradientComponent::SurfaceAltitudeGradientComponent(const SurfaceAltitudeGradientConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void SurfaceAltitudeGradientComponent::Activate()
    {
        m_dependencyMonitor.Reset();
        m_dependencyMonitor.ConnectOwner(GetEntityId());
        m_dependencyMonitor.ConnectDependency(m_configuration.m_shapeEntityId);
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::TickBus::Handler::BusConnect();
        SurfaceAltitudeGradientRequestBus::Handler::BusConnect(GetEntityId());
        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusConnect();
        UpdateFromShape();
        m_dirty = false;

        // Connect to GradientRequestBus last so that everything is initialized before listening for gradient queries.
        GradientRequestBus::Handler::BusConnect(GetEntityId());
    }

    void SurfaceAltitudeGradientComponent::Deactivate()
    {
        // Disconnect from GradientRequestBus first to ensure no queries are in process when deactivating.
        GradientRequestBus::Handler::BusDisconnect();

        m_dependencyMonitor.Reset();
        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        SurfaceAltitudeGradientRequestBus::Handler::BusDisconnect();
        m_dirty = false;
    }

    bool SurfaceAltitudeGradientComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SurfaceAltitudeGradientConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SurfaceAltitudeGradientComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<SurfaceAltitudeGradientConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    float SurfaceAltitudeGradientComponent::GetValue(const GradientSampleParams& sampleParams) const
    {
        float result = 0.0f;
        GetValues(AZStd::span<const AZ::Vector3>(&sampleParams.m_position, 1), AZStd::span<float>(&result, 1));
        return result;
    }

    void SurfaceAltitudeGradientComponent::GetValues(AZStd::span<const AZ::Vector3> positions, AZStd::span<float> outValues) const
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

        SurfaceData::SurfacePointList points;
        AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->GetSurfacePointsFromList(
            positions, m_configuration.m_surfaceTagsToSample, points);

        // For each position, turn the height into a 0-1 value based on our min/max altitudes.
        for (size_t index = 0; index < positions.size(); index++)
        {
            if (!points.IsEmpty(index))
            {
                // Get the point with the highest Z value and use that for the altitude.
                const float highestAltitude = points.GetHighestSurfacePoint(index).m_position.GetZ();

                // Turn the absolute altitude value into a 0-1 value by returning the % of the given altitude range that it falls at.
                outValues[index] = GetRatio(m_configuration.m_altitudeMin, m_configuration.m_altitudeMax, highestAltitude);
            }
            else
            {
                outValues[index] = 0.0f;
            }
        }
    }

    void SurfaceAltitudeGradientComponent::OnCompositionChanged()
    {
        m_dirty = true;
    }

    void SurfaceAltitudeGradientComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (m_dirty)
        {
            const auto altitudeMinOld = m_configuration.m_altitudeMin;
            const auto altitudeMaxOld = m_configuration.m_altitudeMax;

            //updating on tick to query shape bus on main thread
            UpdateFromShape();

            //notify observers if content has changed
            if (altitudeMinOld != m_configuration.m_altitudeMin ||
                altitudeMaxOld != m_configuration.m_altitudeMax ||
                m_surfaceDirty)
            {
                LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
            }
            m_dirty = false;
            m_surfaceDirty = false;
        }
    }

    void SurfaceAltitudeGradientComponent::OnSurfaceChanged(
        [[maybe_unused]] const AZ::EntityId& entityId,
        [[maybe_unused]] const AZ::Aabb& oldBounds,
        [[maybe_unused]] const AZ::Aabb& newBounds,
        [[maybe_unused]] const SurfaceData::SurfaceTagSet& changedSurfaceTags)
    {
        /* The following logic is currently disabled until we can find a safer way to do this.
           The intent of the logic is to make the SurfaceAltitudeGradient refresh its data if the surface(s) that it depends on changes.
           However, it's currently possible to get into a refresh feedback loop if a surface provider (like terrain) uses one of these
           gradients. The loop looks like this:
           - Surface that the gradient depends on changes, which triggers this OnSurfaceChanged message
           - Gradient marks itself as dirty, which triggers an OnCompositionChanged message to anything depending on the gradient
           - Terrain receives message and triggers an OnSurfaceChanged message
           - OnSurfaceChanged message makes it back to this gradient. Even if this gradient doesn't depend on that specific surface,
             it doesn't have enough information here to know that, so if the AABB overlaps, it will mark itself as dirty again, even
             though the actual surfaces we're listening to in that AABB didn't change.

           We can't just query the surface provider itself to see what surfaces it provides, because if there are any surface modifiers,
           it's *possible* for them to modify the points of the surface provider to add the surface types we're listening for.

           By disabling this code, we end up with stale data on the gradient, but enabling it can cause refreshes on every frame which
           destroys the framerate.
        */ 

        /*
        // Create a box that's infinite in the XY direction, but contains our altitude range, so that we can compare against the dirty
        // surface region.
        const AZ::Aabb altitudeBox = AZ::Aabb::CreateFromMinMaxValues(
            AZStd::numeric_limits<float>::lowest(), AZStd::numeric_limits<float>::lowest(), m_configuration.m_altitudeMin,
            AZStd::numeric_limits<float>::max(), AZStd::numeric_limits<float>::max(), m_configuration.m_altitudeMax);

        if (oldBounds.Overlaps(altitudeBox) || newBounds.Overlaps(altitudeBox))
        {
            m_dirty = true;
            m_surfaceDirty = true;
        }
        */
    }

    void SurfaceAltitudeGradientComponent::UpdateFromShape()
    {
        AZStd::unique_lock lock(m_queryMutex);

        if (m_configuration.m_shapeEntityId.IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(
                AZ::Vector3(-AZ::Constants::FloatMax, -AZ::Constants::FloatMax, AZ::GetMin(m_configuration.m_altitudeMin, m_configuration.m_altitudeMax)),
                AZ::Vector3(AZ::Constants::FloatMax, AZ::Constants::FloatMax, AZ::GetMax(m_configuration.m_altitudeMin, m_configuration.m_altitudeMax)));

            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_configuration.m_shapeEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

            if (bounds.IsValid())
            {
                m_configuration.m_altitudeMin = bounds.GetMin().GetZ();
                m_configuration.m_altitudeMax = bounds.GetMax().GetZ();
            }
        }
    }

    AZ::EntityId SurfaceAltitudeGradientComponent::GetShapeEntityId() const
    {
        return m_configuration.m_shapeEntityId;
    }
    void SurfaceAltitudeGradientComponent::SetShapeEntityId(AZ::EntityId entityId)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_shapeEntityId = entityId;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceAltitudeGradientComponent::GetAltitudeMin() const
    {
        return m_configuration.m_altitudeMin;
    }

    void SurfaceAltitudeGradientComponent::SetAltitudeMin(float altitudeMin)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_altitudeMin = altitudeMin;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float SurfaceAltitudeGradientComponent::GetAltitudeMax() const
    {
        return m_configuration.m_altitudeMax;
    }

    void SurfaceAltitudeGradientComponent::SetAltitudeMax(float altitudeMax)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.m_altitudeMax = altitudeMax;
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    size_t SurfaceAltitudeGradientComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 SurfaceAltitudeGradientComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void SurfaceAltitudeGradientComponent::RemoveTag(int tagIndex)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.RemoveTag(tagIndex);
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void SurfaceAltitudeGradientComponent::AddTag(AZStd::string tag)
    {
        // Only hold the lock while we're changing the data. Don't hold onto it during the OnCompositionChanged call, because that can
        // execute an arbitrary amount of logic, including calls back to this component.
        {
            AZStd::unique_lock lock(m_queryMutex);
            m_configuration.AddTag(tag);
        }

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }
}
