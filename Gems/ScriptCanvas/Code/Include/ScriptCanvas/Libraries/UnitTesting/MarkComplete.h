/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/UnitTesting/MarkComplete.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            //! Reports that the graph completed to the unit testing framework, it is required in all unit test graphs
            class MarkComplete
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(MarkComplete);
            }; 
        }
    }
}
