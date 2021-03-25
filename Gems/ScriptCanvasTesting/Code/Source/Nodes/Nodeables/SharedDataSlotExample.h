/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
