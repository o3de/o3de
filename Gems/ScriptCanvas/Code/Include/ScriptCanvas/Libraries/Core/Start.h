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
