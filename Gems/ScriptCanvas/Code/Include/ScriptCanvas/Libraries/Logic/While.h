/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            protected:
                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& /*executionSlot*/, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }
            };
        }
    }
}
