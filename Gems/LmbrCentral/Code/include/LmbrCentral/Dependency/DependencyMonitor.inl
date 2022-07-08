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
        AZ_PROFILE_FUNCTION(Entity);

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        AZ::EntityBus::MultiHandler::BusDisconnect();
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        ShapeComponentNotificationsBus::MultiHandler::BusDisconnect();
        DependencyNotificationBus::MultiHandler::BusDisconnect();
        m_ownerId = AZ::EntityId();

        m_notificationInProgress = false;
    }

    inline void DependencyMonitor::ConnectOwner(const AZ::EntityId& entityId)
    {
        Reset();
        m_ownerId = entityId;
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
        SendNotification();
    }

    inline void DependencyMonitor::OnEntityActivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        SendNotification();
    }

    inline void DependencyMonitor::OnEntityDeactivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        SendNotification();
    }

    inline void DependencyMonitor::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        SendNotification();
    }

    inline void DependencyMonitor::OnShapeChanged([[maybe_unused]] ShapeComponentNotifications::ShapeChangeReasons reasons)
    {
        SendNotification();
    }

    inline void DependencyMonitor::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SendNotification();
    }

    inline void DependencyMonitor::OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData> asset, [[maybe_unused]] void* oldDataPointer)
    {
        SendNotification();
    }

    inline void DependencyMonitor::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        SendNotification();
    }

    inline void DependencyMonitor::OnAssetUnloaded([[maybe_unused]] const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetType assetType)
    {
        SendNotification();
    }

    inline void DependencyMonitor::SendNotification()
    {
        AZ_PROFILE_FUNCTION(Entity);

        //test if notification is in progress to prevent recursion in case of nested dependencies
        if (!m_notificationInProgress)
        {
            m_notificationInProgress = true;
            DependencyNotificationBus::Event(m_ownerId, &DependencyNotificationBus::Events::OnCompositionChanged);
            m_notificationInProgress = false;
        }
    }
}
