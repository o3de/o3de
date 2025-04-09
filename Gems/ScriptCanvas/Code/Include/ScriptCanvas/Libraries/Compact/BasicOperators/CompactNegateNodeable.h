/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/CodeGen/NodeableCodegen.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>

#include <Include/ScriptCanvas/Libraries/Compact/BasicOperators/CompactNegateNodeable.generated.h>

namespace ScriptCanvas::Nodeables
{
    class CompactNegateNodeable : public Nodeable
    {
        SCRIPTCANVAS_NODE(CompactNegateNodeable);

    public:
        AZ_CLASS_ALLOCATOR(CompactNegateNodeable, AZ::SystemAllocator)
    };
}
