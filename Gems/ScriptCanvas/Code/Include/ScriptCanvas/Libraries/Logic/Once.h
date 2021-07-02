/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Logic/Boolean.h>

#include <Include/ScriptCanvas/Libraries/Logic/Once.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! An execution flow gate that will only activate once. The gate will reopen if it receives a Reset pulse.
            class Once
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Once);

                Once();

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                

            protected:

                ExecutionNameMap GetExecutionNameMap() const override;
            };
        }
    }
}
