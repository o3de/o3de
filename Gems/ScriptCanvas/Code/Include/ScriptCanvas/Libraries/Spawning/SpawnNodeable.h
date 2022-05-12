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
#include <Include/ScriptCanvas/Libraries/Spawning/SpawnNodeable.generated.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    //! Node for spawning entities
    class SpawnNodeable
        : public ScriptCanvas::Nodeable
        , public AZ::TickBus::Handler
    {
        SCRIPTCANVAS_NODE(SpawnNodeable);
    public:
        SpawnNodeable() = default;
        SpawnNodeable(const SpawnNodeable& rhs);
        SpawnNodeable& operator=(const SpawnNodeable& rhs);

        // ScriptCanvas::Nodeable  overrides ...
        void OnInitializeExecutionState() override;
        void OnDeactivate() override;

        // AZ::TickBus::Handler overrides ...
        void OnTick(float delta, AZ::ScriptTimePoint timePoint) override;
        
    private:
        struct SpawnableResult
        {
            AZStd::vector<Data::EntityIDType> m_entityList;
            SpawnTicketInstance m_spawnTicket;
        };
        
        AZStd::vector<SpawnableResult> m_completionResults;
        AZStd::recursive_mutex m_mutex;
    };
}
