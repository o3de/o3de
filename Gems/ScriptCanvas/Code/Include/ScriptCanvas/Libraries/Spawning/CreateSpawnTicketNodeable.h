/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Spawnable/SpawnableAssetRef.h>
#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>
#include <Include/ScriptCanvas/Libraries/Spawning/CreateSpawnTicketNodeable.generated.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

namespace ScriptCanvas::Nodeables::Spawning
{
    class CreateSpawnTicketNodeable
        : public Nodeable
    {
        SCRIPTCANVAS_NODE(CreateSpawnTicketNodeable);
    };
}
