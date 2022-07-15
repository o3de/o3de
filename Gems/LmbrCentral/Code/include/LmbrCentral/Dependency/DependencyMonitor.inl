/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace LmbrCentral
{
    inline DependencyMonitor::~DependencyMonitor()
    {
        Reset();
    }

    inline void DependencyMonitor::Reset()
    {
        ResetOwnerId(AZ::EntityId());
        SetDefaultNotificationFunctions();
    }

    inline void DependencyMonitor::ResetOwnerId(const AZ::EntityId& ownerId)
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AZ::EntityBus::MultiHandler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        ShapeComponentNotificationsBus::MultiHandler::BusDisconnect();
        DependencyNotificationBus::MultiHandler::BusDisconnect();
        m_ownerId = ownerId;

        m_notificationInProgress = false;
    }

    inline void DependencyMonitor::SetEntityNotificationFunction(EntityNotificationFunction entityNotificationFn)
    {
        m_entityNotificationFn = entityNotificationFn;
    }

    inline void DependencyMonitor::SetAssetNotificationFunction(AssetNotificationFunction assetNotificationFn)
    {
        m_assetNotificationFn = assetNotificationFn;
    }

    inline void DependencyMonitor::SetDefaultNotificationFunctions()
    {
        m_entityNotificationFn =
            [](const AZ::EntityId& ownerId, [[maybe_unused]] const AZ::EntityId& dependentId, [[maybe_unused]] const AZ::Aabb& dirtyRegion)
        {
            DependencyNotificationBus::Event(ownerId, &DependencyNotificationBus::Events::OnCompositionChanged);
        };

        m_assetNotificationFn =
            [](const AZ::EntityId& ownerId, [[maybe_unused]] const AZ::Data::AssetId& assetId)
        {
            DependencyNotificationBus::Event(ownerId, &DependencyNotificationBus::Events::OnCompositionChanged);
        };
    }

    inline void DependencyMonitor::SetRegionChangedEntityNotificationFunction()
    {
        m_entityNotificationFn =
            [](const AZ::EntityId& ownerId, [[maybe_unused]] const AZ::EntityId& dependentId, const AZ::Aabb& dirtyRegion)
        {
            if (dirtyRegion.IsValid())
            {
                DependencyNotificationBus::Event(ownerId, &DependencyNotificationBus::Events::OnCompositionRegionChanged, dirtyRegion);
            }
            else
            {
                DependencyNotificationBus::Event(ownerId, &DependencyNotificationBus::Events::OnCompositionChanged);
            }
        };
    }

    inline void DependencyMonitor::ConnectOwner(const AZ::EntityId& entityId)
    {
        ResetOwnerId(entityId);
    }

    inline void DependencyMonitor::ConnectDependency(const AZ::EntityId& entityId)
    {
        AZ_PROFILE_FUNCTION(Entity);
        if (entityId.IsValid())
        {
            AZ::EntityBus::MultiHandler::BusConnect(entityId);
            AZ::TransformNotificationBus::MultiHandler::BusConnect(entityId);
            ShapeComponentNotificationsBus::MultiHandler::BusConnect(entityId);
            DependencyNotificationBus::MultiHandler::BusConnect(entityId);
        }
    }

    inline void DependencyMonitor::ConnectDependencies(const AZStd::vector<AZ::EntityId>& entityIds)
    {
        AZ_PROFILE_FUNCTION(Entity);

        for (const auto& entityId : entityIds)
        {
            ConnectDependency(entityId);
        }
    }

    inline void DependencyMonitor::ConnectDependency(const AZ::Data::AssetId& assetId)
    {
        AZ_PROFILE_FUNCTION(Entity);

        if (assetId.IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
        }
    }

    inline void DependencyMonitor::ConnectDependencies(const AZStd::vector<AZ::Data::AssetId>& assetIds)
    {
        for (const auto& assetId : assetIds)
        {
            ConnectDependency(assetId);
        }
    }

    inline void DependencyMonitor::OnCompositionChanged()
    {
        AZ::EntityId entityId = *DependencyNotificationBus::GetCurrentBusId();
        SendEntityChangedNotification(entityId);
    }

    inline void DependencyMonitor::OnCompositionRegionChanged(const AZ::Aabb& dirtyRegion)
    {
        AZ::EntityId entityId = *DependencyNotificationBus::GetCurrentBusId();
        SendEntityChangedNotification(entityId, dirtyRegion);
    }

    inline void DependencyMonitor::OnEntityActivated(const AZ::EntityId& entityId)
    {
        SendEntityChangedNotification(entityId);
    }

    inline void DependencyMonitor::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        SendEntityChangedNotification(entityId);
    }

    inline void DependencyMonitor::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        AZ::EntityId entityId = *AZ::TransformNotificationBus::GetCurrentBusId();
        SendEntityChangedNotification(entityId);
    }

    inline void DependencyMonitor::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        AZ::EntityId entityId = *ShapeComponentNotificationsBus::GetCurrentBusId();
        SendEntityChangedNotification(entityId);
    }

    inline void DependencyMonitor::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SendAssetChangedNotification(asset.GetId());
    }

    inline void DependencyMonitor::OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData> asset, [[maybe_unused]] void* oldDataPointer)
    {
        SendAssetChangedNotification(asset.GetId());
    }

    inline void DependencyMonitor::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SendAssetChangedNotification(asset.GetId());
    }

    inline void DependencyMonitor::OnAssetUnloaded(const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetType assetType)
    {
        SendAssetChangedNotification(assetId);
    }

    inline void DependencyMonitor::SendEntityChangedNotification(const AZ::EntityId& entityId, const AZ::Aabb& dirtyRegion)
    {
        AZ_PROFILE_FUNCTION(Entity);

        // test if notification is in progress to prevent recursion in case of nested dependencies
        if (!m_notificationInProgress)
        {
            m_notificationInProgress = true;
            m_entityNotificationFn(m_ownerId, entityId, dirtyRegion);
            m_notificationInProgress = false;
        }
    }

    inline void DependencyMonitor::SendAssetChangedNotification(const AZ::Data::AssetId& assetId)
    {
        //test if notification is in progress to prevent recursion in case of nested dependencies
        if (!m_notificationInProgress)
        {
            m_notificationInProgress = true;
            m_assetNotificationFn(m_ownerId, assetId);
            m_notificationInProgress = false;
        }
    }
}
