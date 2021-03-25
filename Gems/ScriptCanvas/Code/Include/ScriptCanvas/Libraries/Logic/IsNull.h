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

#include <Include/ScriptCanvas/Libraries/Logic/IsNull.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Evaluates a reference types for null
            class IsNull
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(IsNull);

                IsNull();

                AZ::Outcome<DependencyReport, void> GetDependencies() const;

                bool IsIfBranch() const override;

                bool IsIfBranchPrefacedWithBooleanExpression() const override;

                bool IsSupportedByNewBackend() const override { return true; }

            protected:
                SlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }

                void OnInit() override;

                void OnInputSignal(const SlotId&) override;

            };
        }
    }
}
