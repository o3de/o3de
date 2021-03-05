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
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorPushBack.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorPushBack : public OperatorBase
            {
            public:

                ScriptCanvas_Node(OperatorPushBack,
                    ScriptCanvas_Node::Name("Add Element at End")
                    ScriptCanvas_Node::Uuid("{11DDEEC1-7111-4655-B47F-8F0372B1A1A9}")
                    ScriptCanvas_Node::Description("Adds the provided element at the end of the container")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Containers")
                );

                OperatorPushBack()
                    : OperatorBase(DefaultContainerManipulationOperatorConfiguration())
                {
                }

            protected:

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;
                void OnSourceTypeChanged() override;
                void OnInputSignal(const SlotId& slotId) override;
                void InvokeOperator();

            };
        }
    }
}