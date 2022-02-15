/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/Components/GradientSurfaceDataComponent.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

namespace GradientSignal
{
    void GradientSurfaceDataConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientSurfaceDataConfig, AZ::ComponentConfig>()
                ->Version(2)
                ->Field("ShapeConstraintEntityId", &GradientSurfaceDataConfig::m_shapeConstraintEntityId)
                ->Field("ThresholdMin", &GradientSurfaceDataConfig::m_thresholdMin)
                ->Field("ThresholdMax", &GradientSurfaceDataConfig::m_thresholdMax)
                ->Field("ModifierTags", &GradientSurfaceDataConfig::m_modifierTags)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<GradientSurfaceDataConfig>(
                    "Gradient Surface Tag Emitter", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GradientSurfaceDataConfig::m_shapeConstraintEntityId, "Surface Bounds", "Optionally constrain surface data to the shape on the selected entity")
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC("ShapeService", 0xe86aa5fe))
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSurfaceDataConfig::m_thresholdMin, "Threshold Min", "Minimum value accepted from input gradient that allows tags to be applied.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &GradientSurfaceDataConfig::m_thresholdMax, "Threshold Max", "Maximum value accepted from input gradient that allows tags to be applied.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(0, &GradientSurfaceDataConfig::m_modifierTags, "Extended Tags", "Surface tags to add to contained points")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<GradientSurfaceDataConfig>()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Method("GetNumTags", &GradientSurfaceDataConfig::GetNumTags)
                ->Method("GetTag", &GradientSurfaceDataConfig::GetTag)
                ->Method("RemoveTag", &GradientSurfaceDataConfig::RemoveTag)
                ->Method("AddTag", &GradientSurfaceDataConfig::AddTag)
                ->Property("ShapeConstraintEntityId", BehaviorValueProperty(&GradientSurfaceDataConfig::m_shapeConstraintEntityId))
                ;
        }
    }

    size_t GradientSurfaceDataConfig::GetNumTags() const
    {
        return m_modifierTags.size();
    }

    AZ::Crc32 GradientSurfaceDataConfig::GetTag(int tagIndex) const
    {
        if (tagIndex < m_modifierTags.size() && tagIndex >= 0)
        {
            return m_modifierTags[tagIndex];
        }

        return AZ::Crc32();
    }

    void GradientSurfaceDataConfig::RemoveTag(int tagIndex)
    {
        if (tagIndex < m_modifierTags.size() && tagIndex >= 0)
        {
            m_modifierTags.erase(m_modifierTags.begin() + tagIndex);
        }
    }

    void GradientSurfaceDataConfig::AddTag(AZStd::string tag)
    {
        m_modifierTags.push_back(SurfaceData::SurfaceTag(tag));
    }

    void GradientSurfaceDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void GradientSurfaceDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("SurfaceDataModifierService", 0x68f8aa72));
    }

    void GradientSurfaceDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("GradientService", 0x21c18d23));
    }

    void GradientSurfaceDataComponent::Reflect(AZ::ReflectContext* context)
    {
        GradientSurfaceDataConfig::Reflect(context);

        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<GradientSurfaceDataComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &GradientSurfaceDataComponent::m_configuration)
                ;
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->ConstantProperty("GradientSurfaceDataComponentTypeId", BehaviorConstant(GradientSurfaceDataComponentTypeId))
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ;

            behaviorContext->Class<GradientSurfaceDataComponent>()->RequestBus("GradientSurfaceDataRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ;

            behaviorContext->EBus<GradientSurfaceDataRequestBus>("GradientSurfaceDataRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "vegetation")
                ->Event("GetShapeConstraintEntityId", &GradientSurfaceDataRequestBus::Events::GetShapeConstraintEntityId)
                ->Event("SetShapeConstraintEntityId", &GradientSurfaceDataRequestBus::Events::SetShapeConstraintEntityId)
                ->VirtualProperty("ShapeConstraintEntityId", "GetShapeConstraintEntityId", "SetShapeConstraintEntityId")
                ->Event("SetThresholdMin", &GradientSurfaceDataRequestBus::Events::SetThresholdMin)
                ->Event("GetThresholdMin", &GradientSurfaceDataRequestBus::Events::GetThresholdMin)
                ->VirtualProperty("ThresholdMin", "GetThresholdMin", "SetThresholdMin")
                ->Event("SetThresholdMax", &GradientSurfaceDataRequestBus::Events::SetThresholdMax)
                ->Event("GetThresholdMax", &GradientSurfaceDataRequestBus::Events::GetThresholdMax)
                ->VirtualProperty("ThresholdMax", "GetThresholdMax", "SetThresholdMax")
                ->Event("GetNumTags", &GradientSurfaceDataRequestBus::Events::GetNumTags)
                ->Event("GetTag", &GradientSurfaceDataRequestBus::Events::GetTag)
                ->Event("RemoveTag", &GradientSurfaceDataRequestBus::Events::RemoveTag)
                ->Event("AddTag", &GradientSurfaceDataRequestBus::Events::AddTag)
                ;
        }
    }

    GradientSurfaceDataComponent::GradientSurfaceDataComponent(const GradientSurfaceDataConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void GradientSurfaceDataComponent::Activate()
    {
        m_gradientSampler.m_gradientId = GetEntityId();
        m_gradientSampler.m_ownerEntityId = GetEntityId();

        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        GradientSurfaceDataRequestBus::Handler::BusConnect(GetEntityId());

        if (m_configuration.m_shapeConstraintEntityId.IsValid())
        {
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_shapeConstraintEntityId);
        }

        // Register with the SurfaceData system and update our cached shape information if necessary.
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        UpdateRegistryAndCache(m_modifierHandle);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);
    }

    void GradientSurfaceDataComponent::Deactivate()
    {
        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataModifier, m_modifierHandle);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        GradientSurfaceDataRequestBus::Handler::BusDisconnect();
    }

    bool GradientSurfaceDataComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const GradientSurfaceDataConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool GradientSurfaceDataComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<GradientSurfaceDataConfig*>(outBaseConfig))
        {
            *config = m_configuration;
            return true;
        }
        return false;
    }

    void GradientSurfaceDataComponent::ModifySurfacePoints(SurfaceData::SurfacePointList& surfacePointList) const
    {
        if (!m_configuration.m_modifierTags.empty())
        {
            // This method can be called from the vegetation thread, but our shape bounds can get updated from the main thread.
            // If we have an optional constraining shape bounds, grab a copy of it with minimized mutex lock times.  Avoid mutex
            // locking entirely if we aren't using the shape bounds option at all.
            // (m_validShapeBounds is an atomic bool, so it can be queried outside of the mutex)
            bool validShapeBounds = false;
            AZ::Aabb shapeConstraintBounds;
            if (m_validShapeBounds)
            {
                AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);
                shapeConstraintBounds = m_cachedShapeConstraintBounds;
                validShapeBounds = m_cachedShapeConstraintBounds.IsValid();
            }

            surfacePointList.ModifySurfaceWeights(
                GetEntityId(), 
                [this, validShapeBounds, shapeConstraintBounds](const AZ::Vector3& position, SurfaceData::SurfaceTagWeights& weights)
                {
                    bool inBounds = true;

                    // If we have an optional shape bounds, verify the point exists inside of it before querying the gradient value.
                    // Otherwise, assume an unbounded surface modifier and allow *all* points through the shape check.
                    if (validShapeBounds)
                    {
                        inBounds = false;
                        if (shapeConstraintBounds.Contains(position))
                        {
                            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                                inBounds, m_configuration.m_shapeConstraintEntityId,
                                &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, position);
                        }
                    }

                    // If the point is within our allowed shape bounds, verify that it meets the gradient thresholds.
                    // If so, then return the value to add to the surface tags.
                    if (inBounds)
                    {
                        const GradientSampleParams sampleParams = { position };
                        const float value = m_gradientSampler.GetValue(sampleParams);
                        if (value >= m_configuration.m_thresholdMin && value <= m_configuration.m_thresholdMax)
                        {
                            weights.AddSurfaceTagWeights(m_configuration.m_modifierTags, value);
                        }
                    }
                });
        }
    }

    void GradientSurfaceDataComponent::OnCompositionChanged()
    {
        AZ_PROFILE_FUNCTION(Entity);
        UpdateRegistryAndCache(m_modifierHandle);
    }

    void GradientSurfaceDataComponent::SetThresholdMin(float thresholdMin)
    {
        m_configuration.m_thresholdMin = thresholdMin;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float GradientSurfaceDataComponent::GetThresholdMin() const
    {
        return m_configuration.m_thresholdMin;
    }

    void GradientSurfaceDataComponent::SetThresholdMax(float thresholdMax)
    {
        m_configuration.m_thresholdMax = thresholdMax;
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    float GradientSurfaceDataComponent::GetThresholdMax() const
    {
        return m_configuration.m_thresholdMax;
    }

    size_t GradientSurfaceDataComponent::GetNumTags() const
    {
        return m_configuration.GetNumTags();
    }

    AZ::Crc32 GradientSurfaceDataComponent::GetTag(int tagIndex) const
    {
        return m_configuration.GetTag(tagIndex);
    }

    void GradientSurfaceDataComponent::RemoveTag(int tagIndex)
    {
        m_configuration.RemoveTag(tagIndex);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void GradientSurfaceDataComponent::AddTag(AZStd::string tag)
    {
        m_configuration.AddTag(tag);
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    void GradientSurfaceDataComponent::UpdateRegistryAndCache(SurfaceData::SurfaceDataRegistryHandle& registryHandle)
    {
        // Set up the registry information for this component.
        SurfaceData::SurfaceDataRegistryEntry registryEntry;
        registryEntry.m_entityId = GetEntityId();
        registryEntry.m_tags = m_configuration.m_modifierTags;
        registryEntry.m_bounds = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(registryEntry.m_bounds, m_configuration.m_shapeConstraintEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        // Update our cached shape bounds within a mutex lock so that we don't have data contention
        // with ModifySurfacePoints() on the vegetation thread.
        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);

            // Cache our new shape bounds so that we don't have to look it up for every surface point.
            m_cachedShapeConstraintBounds = registryEntry.m_bounds;

            // Separately keep track of whether or not the bounds are valid in an atomic bool so that we can easily check
            // validity without requiring the mutex.
            m_validShapeBounds = m_cachedShapeConstraintBounds.IsValid();
        }

        // If this is our first time calling this, we need to register with the SurfaceData system.  On subsequent
        // calls, just update the entry that already exists.
        if (registryHandle == SurfaceData::InvalidSurfaceDataRegistryHandle)
        {
            // Register with the SurfaceData system and get a valid registryHandle.
            SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(registryHandle,
                &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataModifier, registryEntry);
        }
        else
        {
            // Update the registry entry with the SurfaceData system using the existing registryHandle.
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
                &SurfaceData::SurfaceDataSystemRequestBus::Events::UpdateSurfaceDataModifier,
                registryHandle, registryEntry);

        }
    }

    void GradientSurfaceDataComponent::OnShapeChanged([[maybe_unused]] LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
    }

    AZ::EntityId GradientSurfaceDataComponent::GetShapeConstraintEntityId() const
    {
        return m_configuration.m_shapeConstraintEntityId;
    }

    void GradientSurfaceDataComponent::SetShapeConstraintEntityId(AZ::EntityId entityId)
    {
        if (m_configuration.m_shapeConstraintEntityId != entityId)
        {
            m_configuration.m_shapeConstraintEntityId = entityId;

            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
            if (m_configuration.m_shapeConstraintEntityId.IsValid())
            {
                LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_shapeConstraintEntityId);
            }

            // If our shape constraint entity has changed, trigger a notification that our component's composition has changed.
            // This will lead to a refresh of any surface data that this component intersects with.
            LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        }
    }
}
