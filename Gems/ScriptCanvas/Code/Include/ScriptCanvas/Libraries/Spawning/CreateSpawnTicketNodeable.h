/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptAssetRef.h>
#include <AzFramework/Spawnable/Script/SpawnableScriptMediator.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

#include <Include/ScriptCanvas/Libraries/Spawning/CreateSpawnTicketNodeable.generated.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    class CreateSpawnTicketNodeable
        : public Nodeable
    {
        SCRIPTCANVAS_NODE(CreateSpawnTicketNodeable);
    public:
        AZ_CLASS_ALLOCATOR(CreateSpawnTicketNodeable, AZ::SystemAllocator)
        CreateSpawnTicketNodeable() = default;
        CreateSpawnTicketNodeable(const CreateSpawnTicketNodeable& rhs);
        CreateSpawnTicketNodeable& operator=(const CreateSpawnTicketNodeable& rhs);

    private:
        AzFramework::Scripts::SpawnableScriptMediator m_spawnableScriptMediator;
    };
}
