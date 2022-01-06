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

#include "SurfaceDataSystemComponent.h"
#include <SurfaceData/SurfaceDataConstants.h>
#include <SurfaceData/SurfaceTag.h>
#include <SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataModifierRequestBus.h>
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
            behaviorContext->Class<SurfacePoint>()
                ->Constructor()
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Attribute(AZ::Script::Attributes::Module, "surface_data")
                ->Property("entityId", BehaviorValueProperty(&SurfacePoint::m_entityId))
                ->Property("position", BehaviorValueProperty(&SurfacePoint::m_position))
                ->Property("normal", BehaviorValueProperty(&SurfacePoint::m_normal))
                ->Property("masks", BehaviorValueProperty(&SurfacePoint::m_masks))
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
        SurfaceDataSystemRequestBus::Handler::BusConnect();
    }

    void SurfaceDataSystemComponent::Deactivate()
    {
        SurfaceDataSystemRequestBus::Handler::BusDisconnect();
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

    void SurfaceDataSystemComponent::GetSurfacePoints(const AZ::Vector3& inPosition, const SurfaceTagVector& desiredTags, SurfacePointList& surfacePointList) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        const bool hasDesiredTags = HasValidTags(desiredTags);
        const bool hasModifierTags = hasDesiredTags && HasMatchingTags(desiredTags, m_registeredModifierTags);

        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        surfacePointList.clear();

        //gather all intersecting points
        for (const auto& entryPair : m_registeredSurfaceDataProviders)
        {
            const AZ::u32 entryAddress = entryPair.first;
            const SurfaceDataRegistryEntry& entry = entryPair.second;
            AZ::Vector3 point2d(inPosition.GetX(), inPosition.GetY(), entry.m_bounds.GetMax().GetZ());
            if (!entry.m_bounds.IsValid() || entry.m_bounds.Contains(point2d))
            {
                if (!hasDesiredTags || hasModifierTags || HasMatchingTags(desiredTags, entry.m_tags))
                {
                    SurfaceDataProviderRequestBus::Event(entryAddress, &SurfaceDataProviderRequestBus::Events::GetSurfacePoints, point2d, surfacePointList);
                }
            }
        }

        if (!surfacePointList.empty())
        {
            //modify or annotate reported points
            for (const auto& entryPair : m_registeredSurfaceDataModifiers)
            {
                const AZ::u32 entryAddress = entryPair.first;
                const SurfaceDataRegistryEntry& entry = entryPair.second;
                AZ::Vector3 point2d(inPosition.GetX(), inPosition.GetY(), entry.m_bounds.GetMax().GetZ());
                if (!entry.m_bounds.IsValid() || entry.m_bounds.Contains(point2d))
                {
                    SurfaceDataModifierRequestBus::Event(entryAddress, &SurfaceDataModifierRequestBus::Events::ModifySurfacePoints, surfacePointList);
                }
            }
            
            // After we've finished creating and annotating all the surface points, combine any points together that have effectively the
            // same XY coordinates and extremely similar Z values.  This produces results that are sorted in decreasing Z order.
            // Also, this filters out any remaining points that don't match the desired tag list.  This can happen when a surface provider
            // doesn't add a desired tag, and a surface modifier has the *potential* to add it, but then doesn't.
            CombineSortAndFilterNeighboringPoints(surfacePointList, hasDesiredTags, desiredTags);
        }
    }

    void SurfaceDataSystemComponent::GetSurfacePointsFromRegion(const AZ::Aabb& inRegion, const AZ::Vector2 stepSize, const SurfaceTagVector& desiredTags, SurfacePointListPerPosition& surfacePointListPerPosition) const
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);

        surfacePointListPerPosition.clear();
        surfacePointListPerPosition.reserve(aznumeric_cast<uint32_t>(ceil(inRegion.GetXExtent() / stepSize.GetX())) * aznumeric_cast<uint32_t>(ceil(inRegion.GetYExtent() / stepSize.GetY())));

        // Initialize our list-per-position list with every input position to query from the region.
        // This is inclusive on the min sides of inRegion, and exclusive on the max sides.
        for (float y = inRegion.GetMin().GetY(); y < inRegion.GetMax().GetY(); y += stepSize.GetY())
        {
            for (float x = inRegion.GetMin().GetX(); x < inRegion.GetMax().GetX(); x += stepSize.GetX())
            {
                surfacePointListPerPosition.emplace_back(AZ::Vector3(x, y, AZ::Constants::FloatMax), SurfaceData::SurfacePointList{});
            }
        }

        const bool hasDesiredTags = HasValidTags(desiredTags);
        const bool hasModifierTags = hasDesiredTags && HasMatchingTags(desiredTags, m_registeredModifierTags);

        // Loop through each data provider, and query all the points for each one.  This allows us to check the tags and the overall
        // AABB bounds just once per provider, instead of once per point.  It also allows for an eventual optimization in which we could send
        // the list of points directly into each SurfaceDataProvider.
        for (const auto& entryPair : m_registeredSurfaceDataProviders)
        {
            const SurfaceDataRegistryEntry& entry = entryPair.second;
            bool alwaysApplies = !entry.m_bounds.IsValid();

            if ((!hasDesiredTags || hasModifierTags || HasMatchingTags(desiredTags, entry.m_tags)) &&
                ( alwaysApplies || AabbOverlaps2D(entry.m_bounds, inRegion) )
                )
            {
                for (auto& surfacePointListAndPoint : surfacePointListPerPosition)
                {
                    const auto& point2d = surfacePointListAndPoint.first;
                    SurfacePointList& surfacePointList = surfacePointListAndPoint.second;
                    AZ::Vector3 point3d(point2d.GetX(), point2d.GetY(), entry.m_bounds.GetMax().GetZ());
                    if (alwaysApplies || entry.m_bounds.Contains(point3d))
                    {
                        SurfaceDataProviderRequestBus::Event(entryPair.first, &SurfaceDataProviderRequestBus::Events::GetSurfacePoints, point3d, surfacePointList);
                    }
                }
            }
        }

        // Once we have our list of surface points created, run through the list of surface data modifiers to potentially add
        // surface tags / values onto each point.  The difference between this and the above loop is that surface data *providers*
        // create new surface points, but surface data *modifiers* simply annotate points that have already been created.  The modifiers
        // are used to annotate points that occur within a volume.  A common example is marking points as "underwater" for points that occur
        // within a water volume.
        for (const auto& entryPair : m_registeredSurfaceDataModifiers)
        {
            const SurfaceDataRegistryEntry& entry = entryPair.second;
            bool alwaysApplies = !entry.m_bounds.IsValid();

            if (alwaysApplies || AabbOverlaps2D(entry.m_bounds, inRegion))
            {
                for (auto& surfacePointListAndPoint : surfacePointListPerPosition)
                {
                    const auto& point2d = surfacePointListAndPoint.first;
                    SurfacePointList& surfacePointList = surfacePointListAndPoint.second;
                    if (!surfacePointList.empty())
                    {
                        AZ::Vector3 point3d(point2d.GetX(), point2d.GetY(), entry.m_bounds.GetMax().GetZ());
                        if (alwaysApplies || entry.m_bounds.Contains(point3d))
                        {
                            SurfaceDataModifierRequestBus::Event(entryPair.first, &SurfaceDataModifierRequestBus::Events::ModifySurfacePoints, surfacePointList);
                        }
                    }
                }
            }
        }

        // After we've finished creating and annotating all the surface points, combine any points together that have effectively the
        // same XY coordinates and extremely similar Z values.  This produces results that are sorted in decreasing Z order.
        // Also, this filters out any remaining points that don't match the desired tag list.  This can happen when a surface provider
        // doesn't add a desired tag, and a surface modifier has the *potential* to add it, but then doesn't.
        for (auto& surfacePointListAndPoint : surfacePointListPerPosition)
        {
            auto& surfacePointList = surfacePointListAndPoint.second;
            if (!surfacePointList.empty())
            {
                CombineSortAndFilterNeighboringPoints(surfacePointList, hasDesiredTags, desiredTags);
            }
        }
    }

    void SurfaceDataSystemComponent::CombineSortAndFilterNeighboringPoints(SurfacePointList& sourcePointList, bool hasDesiredTags, const SurfaceTagVector& desiredTags) const
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (sourcePointList.empty())
        {
            return;
        }

        // Sorting only makes sense if we have two or more points
        if (sourcePointList.size() > 1)
        {
            //sort by depth/distance before combining points
            AZStd::sort(sourcePointList.begin(), sourcePointList.end(), [](const SurfacePoint& a, const SurfacePoint& b)
            {
                return a.m_position.GetZ() > b.m_position.GetZ();
            });
        }


        //efficient point consolidation requires the points to be pre-sorted so we are only comparing/combining neighbors
        const size_t sourcePointCount = sourcePointList.size();
        size_t targetPointIndex = 0;
        size_t sourcePointIndex = 0;

        m_targetPointList.clear();
        m_targetPointList.reserve(sourcePointCount);

        // Locate the first point that matches our desired tags, if one exists.
        for (sourcePointIndex = 0; sourcePointIndex < sourcePointCount; sourcePointIndex++)
        {
            if (!hasDesiredTags || (HasMatchingTags(sourcePointList[sourcePointIndex].m_masks, desiredTags)))
            {
                break;
            }
        }

        if (sourcePointIndex < sourcePointCount)
        {
            // We found a point that matches our tags, so add it to our target list as the first point.
            m_targetPointList.push_back(sourcePointList[sourcePointIndex++]);

            //iterate over subsequent source points for comparison and consolidation with the last added target/unique point
            for (; sourcePointIndex < sourcePointCount; ++sourcePointIndex)
            {
                const auto& sourcePoint = sourcePointList[sourcePointIndex];

                if (!hasDesiredTags || (HasMatchingTags(sourcePoint.m_masks, desiredTags)))
                {
                    auto& targetPoint = m_targetPointList[targetPointIndex];

                    // [LY-90907] need to add a configurable tolerance for comparison
                    if (targetPoint.m_position.IsClose(sourcePoint.m_position) &&
                        targetPoint.m_normal.IsClose(sourcePoint.m_normal))
                    {
                        //consolidate points with similar attributes by adding masks to the target point and ignoring the source
                        AddMaxValueForMasks(targetPoint.m_masks, sourcePoint.m_masks);
                        continue;
                    }

                    //if the points were too different, we have to add a new target point to compare against
                    m_targetPointList.push_back(sourcePoint);
                    ++targetPointIndex;
                }
            }

            AZStd::swap(sourcePointList, m_targetPointList);
        }
    }

    SurfaceDataRegistryHandle SurfaceDataSystemComponent::RegisterSurfaceDataProviderInternal(const SurfaceDataRegistryEntry& entry)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataProviderHandleCounter;
        m_registeredSurfaceDataProviders[handle] = entry;
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataProviderInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
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
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
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
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
        SurfaceDataRegistryHandle handle = ++m_registeredSurfaceDataModifierHandleCounter;
        m_registeredSurfaceDataModifiers[handle] = entry;
        m_registeredModifierTags.insert(entry.m_tags.begin(), entry.m_tags.end());
        return handle;
    }

    SurfaceDataRegistryEntry SurfaceDataSystemComponent::UnregisterSurfaceDataModifierInternal(const SurfaceDataRegistryHandle& handle)
    {
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
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
        AZStd::lock_guard<decltype(m_registrationMutex)> registrationLock(m_registrationMutex);
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
