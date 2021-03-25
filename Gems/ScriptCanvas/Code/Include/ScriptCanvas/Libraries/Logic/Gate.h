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

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Libraries/Logic/Boolean.h>

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

                bool IsSupportedByNewBackend() const override { return true; }

            protected:

                SlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& /*executionSlot*/, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }

                //////////////////////////////////////////////////////////////////////////

                void OnInputSignal(const SlotId&) override;

            private:

                bool m_condition;

            };
        }
    }
}
