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
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorInsert.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            class OperatorInsert : public OperatorBase
            {
            public:

                ScriptCanvas_Node(OperatorInsert,
                    ScriptCanvas_Node::Name("Insert")
                    ScriptCanvas_Node::Uuid("{122B30C9-30A6-4CAF-B4E3-327A6C1A1E38}")
                    ScriptCanvas_Node::Description("Inserts an element into the container at the specified Index or Key")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Category("Containers")
                );



                OperatorInsert()
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