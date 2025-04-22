/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Spawnable/Script/SpawnableScriptBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptMediator.h>
#include <Include/ScriptCanvas/Libraries/Spawning/DespawnNodeable.generated.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    //! Node for despawning entities
    class DespawnNodeable
        : public Nodeable
        , public AzFramework::Scripts::SpawnableScriptNotificationsBus::MultiHandler
    {
        SCRIPTCANVAS_NODE(DespawnNodeable);
    public:
        AZ_CLASS_ALLOCATOR(DespawnNodeable, AZ::SystemAllocator)
        DespawnNodeable() = default;
        DespawnNodeable(const DespawnNodeable& rhs);
        DespawnNodeable& operator=(const DespawnNodeable& rhs);

        // ScriptCanvas::Nodeable  overrides ...
        void OnDeactivate() override;

        // AzFramework::SpawnableScriptNotificationsBus::Handler overrides ...
        void OnDespawn(AzFramework::EntitySpawnTicket spawnTicket) override;

    private:
        AzFramework::Scripts::SpawnableScriptMediator m_spawnableScriptMediator;
    };
}
