/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include "TerrainMacroMaterialBus.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Terrain
{
    // Create a handler that can be accessed from Python scripts to receive terrain change notifications.
    class TerrainMacroMaterialNotificationHandler final
        : public Terrain::TerrainMacroMaterialNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            TerrainMacroMaterialNotificationHandler,
            "{B0ED8B29-0E0D-4567-BEAF-C842C4DB2700}",
            AZ::SystemAllocator,
            OnTerrainMacroMaterialCreated,
            OnTerrainMacroMaterialChanged,
            OnTerrainMacroMaterialRegionChanged,
            OnTerrainMacroMaterialDestroyed);

        void OnTerrainMacroMaterialCreated(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const Terrain::MacroMaterialData& macroMaterial) override
        {
            Call(FN_OnTerrainMacroMaterialCreated);
        }

        void OnTerrainMacroMaterialChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const Terrain::MacroMaterialData& macroMaterial) override
        {
            Call(FN_OnTerrainMacroMaterialChanged);
        }

        void OnTerrainMacroMaterialRegionChanged(
            [[maybe_unused]] AZ::EntityId macroMaterialEntity,
            [[maybe_unused]] const AZ::Aabb& oldRegion,
            [[maybe_unused]] const AZ::Aabb& newRegion) override
        {
            Call(FN_OnTerrainMacroMaterialRegionChanged);
        }

        void OnTerrainMacroMaterialDestroyed([[maybe_unused]] AZ::EntityId macroMaterialEntity) override
        {
            Call(FN_OnTerrainMacroMaterialDestroyed);
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<Terrain::TerrainMacroMaterialNotificationBus>("TerrainMacroMaterialAutomationBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Module, "terrain")
                    ->Handler<TerrainMacroMaterialNotificationHandler>();
            }
        }
    };

    void TerrainMacroMaterialRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<Terrain::TerrainMacroMaterialRequestBus>("TerrainMacroMaterialRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Event("GetTerrainMacroMaterialData", &Terrain::TerrainMacroMaterialRequestBus::Events::GetTerrainMacroMaterialData)
            ;

            behaviorContext->EBus<Terrain::TerrainMacroMaterialNotificationBus>("TerrainMacroMaterialNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Terrain")
                ->Attribute(AZ::Script::Attributes::Module, "terrain")
                ->Event("OnTerrainMacroMaterialCreated", &Terrain::TerrainMacroMaterialNotifications::OnTerrainMacroMaterialCreated)
                ->Event("OnTerrainMacroMaterialChanged", &Terrain::TerrainMacroMaterialNotifications::OnTerrainMacroMaterialChanged)
                ->Event("OnTerrainMacroMaterialRegionChanged", &Terrain::TerrainMacroMaterialNotifications::OnTerrainMacroMaterialRegionChanged)
                ->Event("OnTerrainMacroMaterialDestroyed", &Terrain::TerrainMacroMaterialNotifications::OnTerrainMacroMaterialDestroyed)
            ;

            Terrain::TerrainMacroMaterialNotificationHandler::Reflect(context);
        }
    }
} // namespace Terrain
