/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TerrainDataRequestBus.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AzFramework::Terrain
{
    // Create a handler that can be accessed from Python scripts to receive terrain change notifications.
    class TerrainDataNotificationHandler final
        : public AzFramework::Terrain::TerrainDataNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            TerrainDataNotificationHandler,
            "{A83EF103-295A-4653-8279-F30FBF3F9037}",
            AZ::SystemAllocator,
            OnTerrainDataCreateBegin,
            OnTerrainDataCreateEnd,
            OnTerrainDataDestroyBegin,
            OnTerrainDataDestroyEnd,
            OnTerrainDataChanged);

        void OnTerrainDataCreateBegin() override
        {
            Call(FN_OnTerrainDataCreateBegin);
        }

        void OnTerrainDataCreateEnd() override
        {
            Call(FN_OnTerrainDataCreateEnd);
        }

        void OnTerrainDataDestroyBegin() override
        {
            Call(FN_OnTerrainDataDestroyBegin);
        }

        void OnTerrainDataDestroyEnd() override
        {
            Call(FN_OnTerrainDataDestroyEnd);
        }

        void OnTerrainDataChanged(
            const AZ::Aabb& dirtyRegion, AzFramework::Terrain::TerrainDataNotifications::TerrainDataChangedMask dataChangedMask) override
        {
            Call(FN_OnTerrainDataChanged, dirtyRegion, dataChangedMask);
        }
    };

    void TerrainDataRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AzFramework::Terrain::TerrainDataRequestBus>("TerrainDataRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Event("GetNormal", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetNormal)
                ->Event("GetMaxSurfaceWeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetMaxSurfaceWeight)
                ->Event("GetMaxSurfaceWeightFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetMaxSurfaceWeightFromVector2)
                ->Event("GetSurfaceWeights", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfaceWeights)
                ->Event("GetSurfaceWeightsFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetSurfaceWeightsFromVector2)
                ->Event("GetIsHole", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHole)
                ->Event("GetIsHoleFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetIsHoleFromFloats)
                ->Event("GetSurfacePoint", &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetSurfacePoint)
                ->Event("GetSurfacePointFromVector2",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::BehaviorContextGetSurfacePointFromVector2)
                ->Event("GetTerrainAabb", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainAabb)
                ->Event("GetTerrainHeightQueryResolution",
                    &AzFramework::Terrain::TerrainDataRequestBus::Events::GetTerrainHeightQueryResolution)
                ->Event("GetHeight", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetHeightVal)
                ->Event("GetHeightFromVector2", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetHeightValFromVector2)
                ->Event("GetHeightFromFloats", &AzFramework::Terrain::TerrainDataRequestBus::Events::GetHeightValFromFloats)
                ;

            behaviorContext->EBus<AzFramework::Terrain::TerrainDataNotificationBus>("TerrainDataNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Event("OnTerrainDataCreateBegin", &AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateBegin)
                ->Event("OnTerrainDataCreateEnd", &AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataCreateEnd)
                ->Event("OnTerrainDataDestroyBegin", &AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyBegin)
                ->Event("OnTerrainDataDestroyEnd", &AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataDestroyEnd)
                ->Event("OnTerrainDataChanged", &AzFramework::Terrain::TerrainDataNotifications::OnTerrainDataChanged)
                ->Handler<AzFramework::Terrain::TerrainDataNotificationHandler>()
            ;
        }

        //TerrainDataNotificationHandler::Reflect(context);
    }
} // namespace AzFramework::Terrain
