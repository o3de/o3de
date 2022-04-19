/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/sort.h>

#include <SurfaceData/Components/SurfaceDataSystemComponent.h>
#include <SurfaceData/SurfaceDataConstants.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>


namespace SurfaceData
{
    void SurfaceDataSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        SurfaceTag::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SurfaceDataSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SurfaceDataSystemComponent>("Surface Data System", "Manages registration of surface data providers and forwards intersection data requests to them")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SurfacePointList>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ;

            behaviorContext->Class<SurfaceDataSystemComponent>()
                ->RequestBus("SurfaceDataSystemRequestBus")
                ;

            behaviorContext->EBus<SurfaceDataSystemRequestBus>("SurfaceDataSystemRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Event("GetSurfacePoints", &SurfaceDataSystemRequestBus::Events::GetSurfacePoints)
                ;

            behaviorContext->EBus<SurfaceDataSystemNotificationBus>("SurfaceDataSystemNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Event("OnSurfaceChanged", &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged)
                ;
        }
    }

    void SurfaceDataSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void SurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void SurfaceDataSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void SurfaceDataSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SurfaceDataSystemComponent::Init()
    {
    }

    void SurfaceDataSystemComponent::Activate()
    {
        AZ::Interface<SurfaceDataSystem>::Register(this);
        SurfaceDataSystemRequestBus::Handler::BusConnect();
    }

    void SurfaceDataSystemComponent::Deactivate()
    {
        SurfaceDataSystemRequestBus::Handler::BusDisconnect();
        AZ::Interface<SurfaceDataSystem>::Unregister(this);
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataProvider(const SurfaceDataRegistryEntry& entry)
    {
        const SurfaceDataRegistryHandle handle = RegisterSurfaceDataProviderInternal(entry);
        if (handle != InvalidSurfaceDataRegistryHandle)
        {
            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause *all* vegetation sectors to get marked as dirty.
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataProviderInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for new bounds
            // would cause *all* vegetation sectors to get marked as dirty.
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZ::Aabb oldBounds = AZ::Aabb::CreateNull();

        if (UpdateSurfaceDataProviderInternal(handle, entry, oldBounds))
        {
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, oldBounds, entry.m_bounds);
        }
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry)
    {
        const SurfaceDataRegistryHandle handle = RegisterSurfaceDataModifierInternal(entry);
        if (handle != InvalidSurfaceDataRegistryHandle)
        {
            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause *all* vegetation sectors to get marked as dirty.
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataModifierInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for new bounds
            // would cause *all* vegetation sectors to get marked as dirty.
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZ::Aabb oldBounds = AZ::Aabb::CreateNull();

        if (UpdateSurfaceDataModifierInternal(handle, entry, oldBounds))
        {
            SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, oldBounds, entry.m_bounds);
        }
    }

    void SurfaceDataSystemComponent::RefreshSurfaceData(const AZ::Aabb& dirtyBounds)
    {
        SurfaceDataSystemNotificationBus::Broadcast(&SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, AZ::EntityId(), dirtyBounds, dirtyBounds);
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::GetSurfaceDataProviderHandle(const AZ::EntityId& providerEntityId)
    {
        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        for (auto& [providerHandle, providerEntry] : m_registeredSurfaceDataProviders)
        {
            if (providerEntry.m_entityId == providerEntityId)
            {
                return providerHandle;
            }
        }
        return {};
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::GetSurfaceDataModifierHandle(const AZ::EntityId& modifierEntityId)
    {
        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        for (auto& [modifierHandle, modifierEntry] : m_registeredSurfaceDataModifiers)
        {
            if (modifierEntry.m_entityId == modifierEntityId)
            {
                return modifierHandle;
            }
        }
        return {};
    }

    void SurfaceDataSystemComponent::GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointList) const
    {
        GetSurfacePointsFromListInternal(
            AZStd::span<const AZ::Vector3>(&inPosition, 1), AZ::Aabb::CreateFromPoint(inPosition), desiredTags, surfacePointList);
    }

    void SurfaceDataSystemComponent::GetSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize,
        const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointLists) const
    {
        const size_t totalQueryPositions = aznumeric_cast<size_t>(ceil(inRegion.GetXExtent() / stepSize.GetX())) *
            aznumeric_cast<size_t>(ceil(inRegion.GetYExtent() / stepSize.GetY()));

        AZStd::vector<AZ::Vector3> inPositions;
        inPositions.reserve(totalQueryPositions);

        // Initialize our list-per-position list with every input position to query from the region.
        // This is inclusive on the min sides of inRegion, and exclusive on the max sides.
        for (float y = inRegion.GetMin().GetY(); y < inRegion.GetMax().GetY(); y += stepSize.GetY())
        {
            for (float x = inRegion.GetMin().GetX(); x < inRegion.GetMax().GetX(); x += stepSize.GetX())
            {
                inPositions.emplace_back(x, y, AZ::Constants::FloatMax);
            }
        }

        GetSurfacePointsFromListInternal(inPositions, inRegion, desiredTags, surfacePointLists);
    }

    void SurfaceDataSystemComponent::GetSurfacePointsFromList(
        AZStd::span<const AZ::Vector3> inPositions,
        const SurfaceTagVector& desiredTags,
        SurfacePointList& surfacePointLists) const
    {
        AZ::Aabb inBounds = AZ::Aabb::CreateNull();
        for (auto& position : inPositions)
        {
            inBounds.AddPoint(position);
        }

        GetSurfacePointsFromListInternal(inPositions, inBounds, desiredTags, surfacePointLists);
    }

    void SurfaceDataSystemComponent::GetSurfacePointsFromListInternal(
        AZStd::span<const AZ::Vector3> inPositions, const AZ::Aabb& inPositionBounds,
        const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointLists) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        const bool useTagFilters = HasValidTags(desiredTags);
        const bool hasModifierTags = useTagFilters && HasAnyMatchingTags(desiredTags, m_registeredModifierTags);

        // Clear our output structure.
        surfacePointLists.Clear();

        auto ProviderIsApplicable = [useTagFilters, hasModifierTags, &desiredTags, &inPositionBounds]
            (const SurfaceDataRegistryEntry& provider) -> bool
        {
            bool hasInfiniteBounds = !provider.m_bounds.IsValid();

            // Only allow surface providers that match our tag filters. However, if we aren't using tag filters,
            // or if there's at least one surface modifier that can *add* a filtered tag to a created point, then
            // allow all the surface providers.
            if (!useTagFilters || hasModifierTags || HasAnyMatchingTags(desiredTags, provider.m_tags))
            {
                // Only allow surface providers that overlap the input position area.
                if (hasInfiniteBounds || AabbOverlaps2D(provider.m_bounds, inPositionBounds))
                {
                    return true;
                }
            }

            return false;
        };

        // Gather up the subset of surface providers that overlap the input positions.
        size_t maxPointsCreatedPerInput = 0;
        for (const auto& [providerHandle, provider] : m_registeredSurfaceDataProviders)
        {
            if (ProviderIsApplicable(provider))
            {
                maxPointsCreatedPerInput += provider.m_maxPointsCreatedPerInput;
            }
        }

        // If we don't have any surface providers that will create any new surface points, then there's nothing more to do.
        if (maxPointsCreatedPerInput == 0)
        {
            return;
        }

        // Notify our output structure that we're starting to build up the list of output points.
        // This will reserve memory and allocate temporary structures to help build up the list efficiently.
        AZStd::span<const SurfaceTag> tagFilters;
        if (useTagFilters)
        {
            tagFilters = desiredTags;
        }

        {
            AZ_PROFILE_SCOPE(Entity, "GetSurfacePointsFromListInternal: StartListConstruction");
            surfacePointLists.StartListConstruction(inPositions, maxPointsCreatedPerInput, tagFilters);
        }

        // Loop through each data provider and generate surface points from the set of input positions.
        // Any generated points that have the same XY coordinates and extremely similar Z values will get combined together.
        {
            AZ_PROFILE_SCOPE(Entity, "GetSurfacePointsFromListInternal: GetSurfacePointsFromList");
            for (const auto& [providerHandle, provider] : m_registeredSurfaceDataProviders)
            {
                if (ProviderIsApplicable(provider))
                {
                    SurfaceDataProviderRequestBus::Event(
                        providerHandle, &SurfaceDataProviderRequestBus::Events::GetSurfacePointsFromList, inPositions, surfacePointLists);
                }
            }
        }

        // Once we have our list of surface points created, run through the list of surface data modifiers to potentially add
        // surface tags / values onto each point.  The difference between this and the above loop is that surface data *providers*
        // create new surface points, but surface data *modifiers* simply annotate points that have already been created.  The modifiers
        // are used to annotate points that occur within a volume.  A common example is marking points as "underwater" for points that occur
        // within a water volume.
        {
            AZ_PROFILE_SCOPE(Entity, "GetSurfacePointsFromListInternal: ModifySurfaceWeights");
            for (const auto& [modifierHandle, modifier] : m_registeredSurfaceDataModifiers)
            {
                bool hasInfiniteBounds = !modifier.m_bounds.IsValid();

                if (hasInfiniteBounds || AabbOverlaps2D(modifier.m_bounds, surfacePointLists.GetSurfacePointAabb()))
                {
                    surfacePointLists.ModifySurfaceWeights(modifierHandle);
                }
            }
        }

        // Notify the output structure that we're done building up the list.
        // This will filter out any remaining points that don't match the desired tag list. This can happen when a surface provider
        // doesn't add a desired tag, and a surface modifier has the *potential* to add it, but then doesn't.
        // It may also compact the memory and free any temporary structures.
        surfacePointLists.EndListConstruction();
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataProviderInternal(const SurfaceDataRegistryEntry& entry)
    {
        AZ_Assert(entry.m_maxPointsCreatedPerInput > 0, "Surface data providers should always create at least 1 point.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataProviderHandleCounter;
        m_registeredSurfaceDataProviders[handle] = entry;
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryEntry entry;
        auto entryItr = m_registeredSurfaceDataProviders.find(handle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            entry = entryItr->second;
            m_registeredSurfaceDataProviders.erase(entryItr);
        }
        return entry;
    }

    bool SurfaceDataSystemComponent::UpdateSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds)
    {
        AZ_Assert(entry.m_maxPointsCreatedPerInput > 0, "Surface data providers should always create at least 1 point.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        auto entryItr = m_registeredSurfaceDataProviders.find(handle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            oldBounds = entryItr->second.m_bounds;
            entryItr->second = entry;
            return true;
        }
        return false;
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataModifierInternal(const SurfaceDataRegistryEntry& entry)
    {
        AZ_Assert(entry.m_maxPointsCreatedPerInput == 0, "Surface data modifiers cannot create any points.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataModifierHandleCounter;
        m_registeredSurfaceDataModifiers[handle] = entry;
        m_registeredModifierTags.insert(entry.m_tags.begin(), entry.m_tags.end());
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryEntry entry;
        auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
        if (entryItr != m_registeredSurfaceDataModifiers.end())
        {
            entry = entryItr->second;
            m_registeredSurfaceDataModifiers.erase(entryItr);
        }
        return entry;
    }

    bool SurfaceDataSystemComponent::UpdateSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry, AZ::Aabb& oldBounds)
    {
        AZ_Assert(entry.m_maxPointsCreatedPerInput == 0, "Surface data modifiers cannot create any points.");
        AZStd::unique_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
        if (entryItr != m_registeredSurfaceDataModifiers.end())
        {
            oldBounds = entryItr->second.m_bounds;
            entryItr->second = entry;
            m_registeredModifierTags.insert(entry.m_tags.begin(), entry.m_tags.end());
            return true;
        }
        return false;
    }

}
