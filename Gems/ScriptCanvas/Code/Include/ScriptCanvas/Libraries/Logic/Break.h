/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <Include/ScriptCanvas/Libraries/Logic/Break.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Used to exit a looping structure
            class Break
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Break);

                Break() = default;                   

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                

            };
        }
    }
}
