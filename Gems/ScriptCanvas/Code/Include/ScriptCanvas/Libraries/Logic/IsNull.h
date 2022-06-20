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

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                bool IsIfBranch() const override;

                bool IsIfBranchPrefacedWithBooleanExpression() const override;

            protected:
                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }

                void OnInit() override;
            };
        }
    }
}
