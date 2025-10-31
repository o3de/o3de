// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#pragma once

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>

#include <Source/${Name}.generated.h>

namespace ScriptCanvas::Nodes
{
    class ${SanitizedCppName}
        : public Nodeable
    {
        SCRIPTCANVAS_NODE(${SanitizedCppName});
    public:
        ${SanitizedCppName}() = default;

    };
}
