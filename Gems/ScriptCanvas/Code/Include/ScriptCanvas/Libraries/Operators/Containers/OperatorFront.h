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
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorFront.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorFront : public OperatorBase
            {
            public:
                ScriptCanvas_Node(OperatorFront,
                    ScriptCanvas_Node::Name("Get First Element")
                    ScriptCanvas_Node::Uuid("{B0E10176-5DFE-4D34-BEF8-7EB575929CE7}")
                    ScriptCanvas_Node::Description("Retrieves the first element in the container")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Containers")
                );

                OperatorFront()
                    : OperatorBase(DefaultContainerInquiryOperatorConfiguration())
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