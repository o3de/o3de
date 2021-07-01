/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <Include/ScriptCanvas/Libraries/Core/Start.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            //! Starts executing the graph when the entity that owns the graph is fully activated.
            class Start 
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(Start);

                

                void OnInputSignal(const SlotId&) override;

            };
        }
    }
}
