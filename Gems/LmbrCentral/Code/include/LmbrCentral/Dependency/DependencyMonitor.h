/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Dependency/DependencyNotificationBus.h>
#include <AzCore/Asset/AssetCommon.h>

namespace LmbrCentral
{
    /**
    * Tracks changes in entity dependencies to refresh vegetation
    */
    class DependencyMonitor
        : private AZ::Data::AssetBus::MultiHandler
        , private AZ::EntityBus::MultiHandler
        , private AZ::TransformNotificationBus::MultiHandler
        , private ShapeComponentNotificationsBus::MultiHandler
        , private DependencyNotificationBus::MultiHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(DependencyMonitor, AZ::SystemAllocator, 0);
        AZ_RTTI(DependencyMonitor, "{C7756A84-58D2-4171-A448-F8D3B84DF2F0}");

        DependencyMonitor() = default;
        ~DependencyMonitor();

        void Reset();
        void ConnectOwner(const AZ::EntityId& entityId);

        void ConnectDependency(const AZ::EntityId& entityId);
        void ConnectDependencies(const AZStd::vector<AZ::EntityId>& entityIds);

        void ConnectDependency(const AZ::Data::AssetId& assetId);
        void ConnectDependencies(const AZStd::vector<AZ::Data::AssetId>& assetIds);

    private:
        DependencyMonitor(const DependencyMonitor&) = delete;
        DependencyMonitor(const DependencyMonitor&&) = delete;
        DependencyMonitor& operator=(const DependencyMonitor&) = delete;

        //////////////////////////////////////////////////////////////////////////
        // DependencyNotificationBus
        void OnCompositionChanged() override;

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

        void SendNotification();

        AZ::EntityId m_ownerId;
        AZStd::atomic_bool m_notificationInProgress{false};
    };
}

#include <LmbrCentral/Dependency/DependencyMonitor.inl>