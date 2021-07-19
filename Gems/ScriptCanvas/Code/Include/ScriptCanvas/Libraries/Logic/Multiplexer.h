/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

#include <Include/ScriptCanvas/Libraries/Logic/Multiplexer.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Deprecated: see Targeted Sequencer
            class Multiplexer
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Multiplexer);

            };
        }
    }
}
