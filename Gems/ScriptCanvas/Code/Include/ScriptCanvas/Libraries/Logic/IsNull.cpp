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

#include <Libraries/Logic/IsNull.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <ScriptCanvas/Core/Contracts/IsReferenceTypeContract.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            IsNull::IsNull()
                : Node()
            {}

            void IsNull::OnInit()
            {

                {
                    auto func = []() { return aznew IsReferenceTypeContract(); };
                    ContractDescriptor descriptor{ AZStd::move(func) };                    

                    DynamicDataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = "Reference";
                    slotConfiguration.m_contractDescs.emplace_back(descriptor);
                    slotConfiguration.m_dynamicDataType = DynamicDataType::Any;
                    slotConfiguration.SetConnectionType(ConnectionType::Input);

                    AddSlot(slotConfiguration);
                }

                {
                    DataSlotConfiguration slotConfiguration;

                    slotConfiguration.m_name = "Is Null";
                    slotConfiguration.SetType(Data::Type::Boolean());
                    slotConfiguration.SetConnectionType(ConnectionType::Output);

                    AddSlot(slotConfiguration);
                }
            }

            void IsNull::OnInputSignal(const SlotId&)
            {
                const bool isNull = FindDatum(GetSlotId("Reference"))->Empty();
                PushOutput(Datum(isNull), *GetSlot(GetSlotId("Is Null")));
                SignalOutput(isNull ? IsNullProperty::GetTrueSlotId(this) : IsNullProperty::GetFalseSlotId(this));
            }
        }
    }
}
