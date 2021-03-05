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

#include <ScriptCanvas/Libraries/Operators/Operator.h>

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorErase.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorErase : public OperatorBase
            {
            public:
                ScriptCanvas_Node(OperatorErase,
                    ScriptCanvas_Node::Name("Erase")
                    ScriptCanvas_Node::Uuid("{F1A891C9-81D4-4675-A57A-F11AB415F95F}")
                    ScriptCanvas_Node::Description("Erase the element at the specified Index or with the specified Key")
                    ScriptCanvas_Node::Version(1)
                    ScriptCanvas_Node::Category("Containers")
                );

                OperatorErase()
                    : OperatorBase(DefaultContainerManipulationOperatorConfiguration())
                {
                }

                ScriptCanvas_Out(ScriptCanvas_Out::Name("Element Not Found", "Triggered if the specified element was not found"));

            protected:

                void OnInit() override;

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;
                void OnSourceTypeChanged() override;
                void OnInputSignal(const SlotId& slotId) override;
                void InvokeOperator();

            };

        }
    }
}
