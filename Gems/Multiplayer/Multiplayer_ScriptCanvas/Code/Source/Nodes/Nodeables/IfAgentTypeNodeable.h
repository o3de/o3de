/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <Source/Nodes/Nodeables/IfAgentTypeNodeable.generated.h>

namespace ScriptCanvasMultiplayer
{
    class IfAgentTypeNodeable
        : public ScriptCanvas::Nodeable
    {
        SCRIPTCANVAS_NODE(IfAgentTypeNodeable);
    };
} // namespace ScriptCanvasMultiplayer
