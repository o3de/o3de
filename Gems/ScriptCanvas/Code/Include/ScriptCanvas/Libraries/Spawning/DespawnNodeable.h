/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Include/ScriptCanvas/Libraries/Spawning/DespawnNodeable.generated.h>

#include <AzCore/Component/TickBus.h>

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Libraries/Spawning/SpawnableData.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    class DespawnNodeable
        : public ScriptCanvas::Nodeable
        , public AZ::TickBus::Handler
    {
        SCRIPTCANVAS_NODE(DespawnNodeable);
    public:
        DespawnNodeable() = default;
        DespawnNodeable(const DespawnNodeable& rhs);
        DespawnNodeable& operator=(DespawnNodeable& rhs);

        void OnInitializeExecutionState() override;
        void OnDeactivate() override;

        //TickBus
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
        
    private:
        AZStd::vector<AzFramework::EntitySpawnTicket::Id> m_despawnedTicketIdList;
        AZStd::recursive_mutex m_idBatchMutex;
    };
}
