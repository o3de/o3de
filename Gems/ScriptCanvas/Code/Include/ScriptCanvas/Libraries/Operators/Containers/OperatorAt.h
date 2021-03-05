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
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorAt.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorAt : public OperatorBase
            {
            public:
                ScriptCanvas_Node(OperatorAt,
                    ScriptCanvas_Node::Name("Get Element")
                    ScriptCanvas_Node::Uuid("{6A4FE29F-3BAA-40D6-8BEE-CDE9CCBD7885}")
                    ScriptCanvas_Node::Description("Returns the element at the specified Index or Key")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Containers")
                );

                OperatorAt()
                    : OperatorBase(DefaultContainerInquiryOperatorConfiguration())
                {
                }

                ScriptCanvas_Out(ScriptCanvas_Out::Name("Key Not Found", "Triggered if the specified key was not found"));

            protected:

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;
                void OnSourceTypeChanged() override;
                void OnInputSignal(const SlotId& slotId) override;
                void InvokeOperator();
                void KeyNotFound(const Datum* containerDatum);
            };

        }
    }
}
