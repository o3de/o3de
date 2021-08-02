/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/CodeGen/NodeableCodegen.h>

#include <Source/Nodes/Nodeables/SharedDataSlotExample.generated.h>

namespace ScriptCanvasTesting
{
    namespace Nodeables
    {
        class InputMethodSharedDataSlotExample
            : public ScriptCanvas::Nodeable
        {
            SCRIPTCANVAS_NODE(InputMethodSharedDataSlotExample);
        public:

        };

        class BranchMethodSharedDataSlotExample
            : public ScriptCanvas::Nodeable
        {
            SCRIPTCANVAS_NODE(BranchMethodSharedDataSlotExample);
        public:

            void StringMagicbox(int);

        };
    }
}
