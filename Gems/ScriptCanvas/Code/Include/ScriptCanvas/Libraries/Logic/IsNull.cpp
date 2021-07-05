/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            AZ::Outcome<DependencyReport, void> IsNull::GetDependencies() const
            {
                return AZ::Success(DependencyReport());
            }

            bool IsNull::IsIfBranch() const
            {
                return true;
            }

            bool IsNull::IsIfBranchPrefacedWithBooleanExpression() const
            {
                return true;
            }

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
