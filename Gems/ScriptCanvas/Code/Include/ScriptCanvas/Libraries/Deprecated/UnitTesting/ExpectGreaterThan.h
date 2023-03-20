/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <Libraries/Deprecated/UnitTesting/UnitTesting.h>
#include <Include/ScriptCanvas/Libraries/Deprecated/UnitTesting/ExpectGreaterThan.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            class ExpectGreaterThan
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(ExpectGreaterThan);

                void OnInit() override;
            };
        }
    }
}
