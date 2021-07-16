/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>

#include <Include/ScriptCanvas/Libraries/Logic/Gate.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Represents an "If" execution condition, continues True if the Condition is True, otherwise False
            class Gate
                : public Node
            {
            public:
                SCRIPTCANVAS_NODE(Gate);

                Gate();

                //////////////////////////////////////////////////////////////////////////
                // Translation

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                bool IsIfBranch() const override { return true; }

            protected:
                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& /*executionSlot*/, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }
            };
        }
    }
}
