/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Profiler.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace LmbrCentral
{
    /**
    * The DependencyMonitor is a convenience class to track multiple types of changes in entities and assets and distill the
    * changes down to a single propagated notification that downstream listeners can handle. Specifically, it listens for the
    * following:
    *  - Entity activated / deactivated
    *  - Transform changed
    *  - Shape changed
    *  - Asset ready / reloaded / unloaded / moved
    *  - Entity's dependencies changed
    * All of those get distilled into a single notification that by default will trigger an OnCompositionChanged() message
    * on the DependencyNotificationBus.
    * However, this is sometimes a little *too* distilled, so an entity can override the notification functions to perform custom logic.
    * For example, if the dependent entity has provided a dirty region via OnCompositionRegionChanged(), a function can be installed
    * to examine the region and determine whether or not it should be propagated, changed, or ignored.
    */
    class DependencyMonitor
        : private AZ::Data::AssetBus::MultiHandler
        , private AZ::EntityBus::MultiHandler
        , private AZ::TransformNotificationBus::MultiHandler
        , private ShapeComponentNotificationsBus::MultiHandler
        , private DependencyNotificationBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(DependencyMonitor, AZ::SystemAllocator);
        AZ_RTTI(DependencyMonitor, "{C7756A84-58D2-4171-A448-F8D3B84DF2F0}");

        DependencyMonitor() = default;
        ~DependencyMonitor();

        void Reset();
        void ConnectOwner(const AZ::EntityId& entityId);

        void ConnectDependency(const AZ::EntityId& entityId);
        void ConnectDependencies(const AZStd::vector<AZ::EntityId>& entityIds);

        void ConnectDependency(const AZ::Data::AssetId& assetId);
        void ConnectDependencies(const AZStd::vector<AZ::Data::AssetId>& assetIds);

        using EntityNotificationFunction =
            AZStd::function<void(const AZ::EntityId& ownerId, const AZ::EntityId& dependentId, const AZ::Aabb& dirtyRegion)>;
        using AssetNotificationFunction =
            AZStd::function<void(const AZ::EntityId& ownerId, const AZ::Data::AssetId& assetId)>;

        void SetEntityNotificationFunction(EntityNotificationFunction entityNotificationFn);

        void SetAssetNotificationFunction(AssetNotificationFunction assetNotificationFn);

        // Common notification functions that can be set.

        // The default notification function - always send OnCompositionChanged() on any change.
        void SetDefaultNotificationFunctions();

        // Notification function that passes through a dirty region to OnCompositionRegionChanged() if a dirty region is available.
        void SetRegionChangedEntityNotificationFunction();

    private:
        DependencyMonitor(const DependencyMonitor&) = delete;
        DependencyMonitor(const DependencyMonitor&&) = delete;
        DependencyMonitor& operator=(const DependencyMonitor&) = delete;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;
        void OnCompositionRegionChanged(const AZ::Aabb& dirtyRegion) override;

        ////////////////////////////////////////////////////////////////////////
        // EntityEvents
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;

        //////////////////////////////////////////////////////////////////////////
        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        ////////////////////////////////////////////////////////////////////////
        // ShapeComponentNotificationsBus
        void OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons reasons) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData> asset, void* oldDataPointer) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;

        void ResetOwnerId(const AZ::EntityId& ownerId);

        void SendAssetChangedNotification(const AZ::Data::AssetId& assetId);
        void SendEntityChangedNotification(const AZ::EntityId& entityId, const AZ::Aabb& dirtyRegion = AZ::Aabb::CreateNull());

        AZ::EntityId m_ownerId;
        AZStd::atomic_bool m_notificationInProgress{false};
        EntityNotificationFunction m_entityNotificationFn;
        AssetNotificationFunction m_assetNotificationFn;
    };
}

#include <LmbrCentral/Dependency/DependencyMonitor.inl>
