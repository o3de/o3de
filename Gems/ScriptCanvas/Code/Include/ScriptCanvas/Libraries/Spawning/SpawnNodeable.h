/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptBus.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptMediator.h>
#include <Include/ScriptCanvas/Libraries/Spawning/SpawnNodeable.generated.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    //! Node for spawning entities
    class SpawnNodeable
        : public Nodeable
        , public AzFramework::Scripts::SpawnableScriptNotificationsBus::Handler
    {
        SCRIPTCANVAS_NODE(SpawnNodeable);
    public:
        SpawnNodeable() = default;
        SpawnNodeable(const SpawnNodeable& rhs);
        SpawnNodeable& operator=(const SpawnNodeable& rhs);

        // ScriptCanvas::Nodeable  overrides ...
        void OnDeactivate() override;

        // AzFramework::SpawnableScriptNotificationsBus::Handler overrides ...
        void OnSpawn(AzFramework::EntitySpawnTicket spawnTicket, AZStd::vector<AZ::EntityId> entityList) override;

    private:
        AzFramework::Scripts::SpawnableScriptMediator m_spawnableScriptMediator;
    };
}
