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
#include <ScriptCanvas/Core/GraphBus.h>

#include <Include/ScriptCanvas/Libraries/Logic/While.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Provides a node that represents a While loop (cycles until the provided condition is met)
            class While
                : public Node
            {
                SCRIPTCANVAS_NODE(While);

            public:

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                SlotId GetLoopFinishSlotId() const override;
                
                SlotId GetLoopSlotId() const override;
                
                bool IsFormalLoop() const override;

                bool IsSupportedByNewBackend() const override { return true; }
                
            protected:
                SlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& /*executionSlot*/, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }

            };
        }
    }
}
