/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/UnitTesting/AddSuccess.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            //! Used to represent that a unit test graph's operation has succeeded
            class AddSuccess
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(AddSuccess);
            };
        }
    }
}
