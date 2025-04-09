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
#include <SurfaceDataProfiler.h>

AZ_DEFINE_BUDGET(SurfaceData);

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
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Event(
                    "GetSurfacePoints",
                    [](SurfaceData::SurfaceDataSystem* handler, const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags) -> AZStd::vector<AzFramework::SurfaceData::SurfacePoint>
                    {
                        AZStd::vector<AzFramework::SurfaceData::SurfacePoint> result;
                        SurfaceData::SurfacePointList surfacePointList;
                        handler->GetSurfacePoints(inPosition, desiredTags, surfacePointList);
                        surfacePointList.EnumeratePoints(
                        [&result](
                            [[maybe_unused]] size_t inPositionIndex, const AZ::Vector3& position, const AZ::Vector3& normal, const SurfaceData::SurfaceTagWeights& masks)-> bool
                        {
                            AzFramework::SurfaceData::SurfacePoint point;
                            point.m_position = position;
                            point.m_normal = normal;
                            point.m_surfaceTags = masks.GetSurfaceTagWeightList();

                            result.emplace_back(point);
                            return true;
                        });
                        return result;
                    })
                ->Event("RefreshSurfaceData", &SurfaceDataSystemRequestBus::Events::RefreshSurfaceData)
                ->Event("GetSurfaceDataProviderHandle", &SurfaceDataSystemRequestBus::Events::GetSurfaceDataProviderHandle)
                ->Event("GetSurfaceDataModifierHandle", &SurfaceDataSystemRequestBus::Events::GetSurfaceDataModifierHandle)
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
        provided.push_back(AZ_CRC_CE("SurfaceDataSystemService"));
    }

    void SurfaceDataSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("SurfaceDataSystemService"));
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
            // Get the set of surface tags that can be affected by adding the surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because new surface points have the potential of getting the modifier tags applied as well.
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(entry.m_bounds, entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds,
                affectedSurfaceTags);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataProvider(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataProviderInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            // Get the set of surface tags that can be affected by adding the surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because the removed surface points have the potential of getting the modifier tags applied as well.
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(entry.m_bounds, entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataProvider(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZ::Aabb oldBounds = AZ::Aabb::CreateNull();

        if (UpdateSurfaceDataProviderInternal(handle, entry, oldBounds))
        {
            // Get the set of surface tags that can be affected by adding the surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because the affected surface points have the potential of getting the modifier tags applied as well.
            // For now, we'll just merge the old and new bounds into a larger region. If this causes too much refreshing to occur,
            // this could eventually be improved by getting the tags from both sets of bounds separately and combining them.
            AZ::Aabb surfaceTagBounds = oldBounds;
            surfaceTagBounds.AddAabb(entry.m_bounds);
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(surfaceTagBounds, entry.m_tags);

            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, oldBounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataModifier(const SurfaceDataRegistryEntry& entry)
    {
        const SurfaceDataRegistryHandle handle = RegisterSurfaceDataModifierInternal(entry);
        if (handle != InvalidSurfaceDataRegistryHandle)
        {
            // Get the set of surface tags that can be affected by adding a surface data modifier. Since this doesn't create
            // any new surface points, we only need to broadcast the modifier tags themselves as the ones that changed.
            const SurfaceTagSet affectedSurfaceTags = ConvertTagVectorToSet(entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId,
                entry.m_bounds, entry.m_bounds, affectedSurfaceTags);
        }
        return handle;
    }

    void SurfaceDataSystemComponent::UnregisterSurfaceDataModifier(const SurfaceDataRegistryHandle& handle)
    {
        const SurfaceDataRegistryEntry entry = UnregisterSurfaceDataModifierInternal(handle);
        if (entry.m_entityId.IsValid())
        {
            // Get the set of surface tags that can be affected by removing a surface data modifier. Since this doesn't create
            // any new surface points, we only need to broadcast the modifier tags themselves as the ones that changed.
            const SurfaceTagSet affectedSurfaceTags = ConvertTagVectorToSet(entry.m_tags);

            // Send in the entry's bounds as both the old and new bounds, since a null Aabb for old bounds
            // would cause a full refresh for any system listening, instead of just a refresh within the bounds.
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, entry.m_bounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    void SurfaceDataSystemComponent::UpdateSurfaceDataModifier(const SurfaceDataRegistryHandle& handle, const SurfaceDataRegistryEntry& entry)
    {
        AZ::Aabb oldBounds = AZ::Aabb::CreateNull();

        // Get the previous set of surface tags for this modifier.
        SurfaceTagSet affectedSurfaceTags;
        {
            AZStd::shared_lock<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
            auto entryItr = m_registeredSurfaceDataModifiers.find(handle);
            if (entryItr != m_registeredSurfaceDataModifiers.end())
            {
                affectedSurfaceTags = ConvertTagVectorToSet(entryItr->second.m_tags);
            }
        }

        // Merge in the new set of surface tags for this modifier. Since modifiers don't create
        // any new surface points, we only need to broadcast the modifier tags themselves as the ones that changed.
        for (auto& tag : entry.m_tags)
        {
            affectedSurfaceTags.emplace(tag);
        }

        if (UpdateSurfaceDataModifierInternal(handle, entry, oldBounds))
        {
            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, entry.m_entityId, oldBounds, entry.m_bounds,
                affectedSurfaceTags);
        }
    }

    void SurfaceDataSystemComponent::RefreshSurfaceData(const SurfaceDataRegistryHandle& providerHandle, const AZ::Aabb& dirtyBounds)
    {
        auto entryItr = m_registeredSurfaceDataProviders.find(providerHandle);
        if (entryItr != m_registeredSurfaceDataProviders.end())
        {
            // Get the set of surface tags that can be affected by refreshing a surface data provider.
            // This includes all of the provider's tags, as well as any surface modifier tags that exist in the bounds,
            // because the affected surface points have the potential of getting the modifier tags applied as well.
            SurfaceTagSet affectedSurfaceTags = GetAffectedSurfaceTags(dirtyBounds, entryItr->second.m_tags);

            SurfaceDataSystemNotificationBus::Broadcast(
                &SurfaceDataSystemNotificationBus::Events::OnSurfaceChanged, AZ::EntityId(), dirtyBounds, dirtyBounds, affectedSurfaceTags);
        }

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
        SURFACE_DATA_PROFILE_FUNCTION_VERBOSE

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
            SURFACE_DATA_PROFILE_SCOPE_VERBOSE("GetSurfacePointsFromListInternal: StartListConstruction");
            surfacePointLists.StartListConstruction(inPositions, maxPointsCreatedPerInput, tagFilters);
        }

        // Loop through each data provider and generate surface points from the set of input positions.
        // Any generated points that have the same XY coordinates and extremely similar Z values will get combined together.
        {
            SURFACE_DATA_PROFILE_SCOPE_VERBOSE("GetSurfacePointsFromListInternal: GetSurfacePointsFromList");
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
            SURFACE_DATA_PROFILE_SCOPE_VERBOSE("GetSurfacePointsFromListInternal: ModifySurfaceWeights");
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

    SurfaceTagSet SurfaceDataSystemComponent::GetTagsFromBounds(
        const AZ::Aabb& bounds, const SurfaceDataRegistryMap& registeredEntries) const
    {
        SurfaceTagSet tags;

        const bool inputHasInfiniteBounds = !bounds.IsValid();

        for (const auto& [entryHandle, entry] : registeredEntries)
        {
            const bool entryHasInfiniteBounds = !entry.m_bounds.IsValid();

            if (inputHasInfiniteBounds || entryHasInfiniteBounds || AabbOverlaps2D(entry.m_bounds, bounds))
            {
                for (auto& entryTag : entry.m_tags)
                {
                    tags.emplace(entryTag);
                }
            }
        }

        return tags;
    }

    SurfaceTagSet SurfaceDataSystemComponent::GetProviderTagsFromBounds(const AZ::Aabb& bounds) const
    {
        return GetTagsFromBounds(bounds, m_registeredSurfaceDataProviders);
    }

    SurfaceTagSet SurfaceDataSystemComponent::GetModifierTagsFromBounds(const AZ::Aabb& bounds) const
    {
        return GetTagsFromBounds(bounds, m_registeredSurfaceDataModifiers);
    }

    SurfaceTagSet SurfaceDataSystemComponent::ConvertTagVectorToSet(const SurfaceTagVector& surfaceTags) const
    {
        SurfaceTagSet tags;
        for (auto& tag : surfaceTags)
        {
            tags.emplace(tag);
        }

        return tags;
    }

    SurfaceTagSet SurfaceDataSystemComponent::GetAffectedSurfaceTags(const AZ::Aabb& bounds, const SurfaceTagVector& providerTags) const
    {
        // Get all of the surface tags that can be affected by surface provider changes within the given bounds.
        // Because a change to a surface provider can cause changes to a surface modifier as well, we need to merge all of the
        // surface provider tags with all of the potential surface modifier tags in the given bounds.

        // Get all of the modifier tags that occur in the bounds.
        SurfaceTagSet tagSet = GetModifierTagsFromBounds(bounds);

        // Merge the provider tags with them.
        for (auto& tag : providerTags)
        {
            tagSet.emplace(tag);
        }

        return tagSet;
    }

} // namespace SurfaceData
