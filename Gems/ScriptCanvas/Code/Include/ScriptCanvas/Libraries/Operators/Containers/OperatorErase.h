/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Libraries/Operators/Operator.h>

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorErase.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //! Deprecated: see MethodOverloaded for "Erase"
            class OperatorErase : public OperatorBase
            {
            public:

                SCRIPTCANVAS_NODE(OperatorErase);


                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

                OperatorErase()
                    : OperatorBase(DefaultContainerManipulationOperatorConfiguration())
                {
                }

            protected:

                void OnInit() override;

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;
                void OnSourceTypeChanged() override;

            private:
                bool m_missedElementNotFound = false;
            };

        }
    }
}
