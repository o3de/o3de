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
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Libraries/Spawning/SpawnTicketInstance.h>
#include <Include/ScriptCanvas/Libraries/Spawning/DespawnNodeable.generated.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    //! Node for despawning entities
    class DespawnNodeable
        : public ScriptCanvas::Nodeable
        , public AZ::TickBus::Handler
    {
        SCRIPTCANVAS_NODE(DespawnNodeable);
    public:
        DespawnNodeable() = default;
        DespawnNodeable(const DespawnNodeable& rhs);
        DespawnNodeable& operator=(const DespawnNodeable& rhs);

        // ScriptCanvas::Nodeable  overrides ...
        void OnInitializeExecutionState() override;
        void OnDeactivate() override;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
        
    private:
        AZStd::vector<SpawnTicketInstance> m_despawnedTicketList;
        AZStd::recursive_mutex m_mutex;
    };
}
