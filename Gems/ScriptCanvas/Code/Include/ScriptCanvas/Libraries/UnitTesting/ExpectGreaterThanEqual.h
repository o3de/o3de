/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/UnitTesting/ExpectGreaterThanEqual.generated.h>
#include <Include/ScriptCanvas/Libraries/UnitTesting/UnitTesting.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            class ExpectGreaterThanEqual
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(ExpectGreaterThanEqual);

                void OnInit() override;
            };
        }
    }
}
