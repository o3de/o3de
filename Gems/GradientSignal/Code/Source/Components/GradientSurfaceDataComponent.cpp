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
#include <SurfaceData/MixedStackHeapAllocator.h>

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
                    ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("ShapeService"))
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
        services.push_back(AZ_CRC_CE("SurfaceDataModifierService"));
    }

    void GradientSurfaceDataComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("SurfaceDataModifierService"));
    }

    void GradientSurfaceDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC_CE("GradientService"));
    }

    void GradientSurfaceDataComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        // If there's a shape on this entity, start it before this component just in case it's the shape that we're using as our bounds.
        services.push_back(AZ_CRC_CE("ShapeService"));
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

        if (m_configuration.m_shapeConstraintEntityId.IsValid())
        {
            LmbrCentral::ShapeComponentNotificationsBus::Handler::BusConnect(m_configuration.m_shapeConstraintEntityId);
        }

        GradientSurfaceDataRequestBus::Handler::BusConnect(GetEntityId());

        // Register with the SurfaceData system and update our cached shape information if necessary.
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
        UpdateRegistryAndCache(m_modifierHandle);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusConnect(m_modifierHandle);
    }

    void GradientSurfaceDataComponent::Deactivate()
    {
        GradientSurfaceDataRequestBus::Handler::BusDisconnect();

        LmbrCentral::ShapeComponentNotificationsBus::Handler::BusDisconnect();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataModifier(m_modifierHandle);
        SurfaceData::SurfaceDataModifierRequestBus::Handler::BusDisconnect();
        m_modifierHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
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

    void GradientSurfaceDataComponent::ModifySurfacePoints(
            AZStd::span<const AZ::Vector3> positions,
            [[maybe_unused]] AZStd::span<const AZ::EntityId> creatorEntityIds,
            AZStd::span<SurfaceData::SurfaceTagWeights> weights) const
    {
        AZ_Assert(
            (positions.size() == creatorEntityIds.size()) && (positions.size() == weights.size()),
            "Sizes of the passed-in spans don't match");

        // If we don't have any modifier tags, there's nothing to modify.
        if (m_configuration.m_modifierTags.empty())
        {
            return;
        }

        // This method can be called from any thread, but our shape bounds can get updated from the main thread.
        // If we have an optional constraining shape bounds, grab a copy of it with minimized mutex lock times.  Avoid mutex
        // locking entirely if we aren't using the shape bounds option at all.
        // (m_validShapeBounds is an atomic bool, so it can be queried outside of the mutex)
        AZ::Aabb shapeConstraintBounds = AZ::Aabb::CreateNull();
        if (m_validShapeBounds)
        {
            AZStd::lock_guard<decltype(m_cacheMutex)> lock(m_cacheMutex);
            shapeConstraintBounds = m_cachedShapeConstraintBounds;
        }

        // Optimization: For our temporary vectors, if the input is below a certain size, allocate the temporary data off the stack.
        // Otherwise, allocate from the heap.
        constexpr size_t SmallQuerySize = 16;

        // Start by assuming an unbounded surface modifier and default to allowing *all* points through the shape check.
        AZStd::vector<bool, SurfaceData::mixed_stack_heap_allocator<bool, SmallQuerySize>> inBounds;

        // If we have an optional shape bounds, adjust the inBounds flags based on whether or not each point is inside the bounds.
        if (shapeConstraintBounds.IsValid())
        {
            LmbrCentral::ShapeComponentRequestsBus::Event(
                m_configuration.m_shapeConstraintEntityId,
                [positions, shapeConstraintBounds, &inBounds](LmbrCentral::ShapeComponentRequestsBus::Events* shape)
                {
                    inBounds.resize(positions.size(), false);

                    for (size_t index = 0; index < positions.size(); index++)
                    {
                        // Check the AABB first.
                        if (shapeConstraintBounds.Contains(positions[index]))
                        {
                            // The point is in the AABB, so check against the actual shape geometry.
                            inBounds[index] = shape->IsPointInside(positions[index]);
                        }
                    }
                });
        }

        // Get all of the potential gradient values in one bulk call.
        AZStd::vector<float, SurfaceData::mixed_stack_heap_allocator<float, SmallQuerySize>> gradientValues(positions.size());
        m_gradientSampler.GetValues(positions, gradientValues);

        for (size_t index = 0; index < positions.size(); index++)
        {
            // If the point is within our allowed shape bounds, verify that it meets the gradient thresholds.
            // If so, then add the value to the surface tags.
            if (inBounds.empty() || inBounds[index])
            {
                if (gradientValues[index] >= m_configuration.m_thresholdMin && gradientValues[index] <= m_configuration.m_thresholdMax)
                {
                    weights[index].AddSurfaceTagWeights(m_configuration.m_modifierTags, gradientValues[index]);
                }
            }
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
            registryHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataModifier(registryEntry);
        }
        else
        {
            // Update the registry entry with the SurfaceData system using the existing registryHandle.
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UpdateSurfaceDataModifier(registryHandle, registryEntry);
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
