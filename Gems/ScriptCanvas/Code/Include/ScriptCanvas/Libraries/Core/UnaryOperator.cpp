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

#include "UnaryOperator.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        const char* UnaryOperator::k_evaluateName = "In";
        const char* UnaryOperator::k_onTrue = "True";
        const char* UnaryOperator::k_onFalse = "False";

        const char* UnaryOperator::k_valueName = "Value";
        const char* UnaryOperator::k_resultName = "Result";

        Datum UnaryOperator::Evaluate([[maybe_unused]] const Datum& value)
        {
            AZ_Assert(false, "Evaluate must be overridden");
            return Datum();
        };

        void UnaryOperator::OnInputSignal([[maybe_unused]] const SlotId& slot)
        {
            AZ_Assert(false, "OnInputSignal must be overridden");
        }

        void UnaryOperator::ConfigureSlots()
        {
            {
                ExecutionSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = UnaryOperator::k_evaluateName;
                slotConfiguration.m_toolTip = "Signal to perform the evaluation when desired.";
                slotConfiguration.SetConnectionType(ConnectionType::Input);

                AddSlot(slotConfiguration);
            }
        }

        SlotId UnaryOperator::GetOutputSlotId() const
        {
            return GetSlotId(k_resultName);
        }

        void UnaryOperator::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<UnaryOperator, Node>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<UnaryOperator>("UnaryOperator", "UnaryOperator")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }

        void UnaryExpression::InitializeUnaryExpression()
        {
        }

        void UnaryExpression::OnInputSignal(const SlotId&)
        {
            const Datum output = Evaluate(*FindDatumByIndex(k_datumIndex));
            if (auto slot = GetSlot(GetOutputSlotId()))
            {
                PushOutput(output, *slot);
            }

            const bool* value = output.GetAs<bool>();
            if (value && *value)
            {
                SignalOutput(GetSlotId(k_onTrue));
            }
            else
            {
                SignalOutput(GetSlotId(k_onFalse));
            }
        }

        void UnaryExpression::ConfigureSlots()
        {
            {
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = UnaryExpression::k_valueName;
                slotConfiguration.SetType(Data::Type::Boolean());
                slotConfiguration.SetConnectionType(ConnectionType::Input);

                AddSlot(slotConfiguration);
            }

            {
                DataSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = UnaryExpression::k_resultName;
                slotConfiguration.SetType(Data::Type::Boolean());
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                AddSlot(slotConfiguration);
            }

            UnaryOperator::ConfigureSlots();

            {
                ExecutionSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = k_onTrue;
                slotConfiguration.m_toolTip = "Signaled if the result of the operation is true.";
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                AddSlot(slotConfiguration);
            }

            {
                ExecutionSlotConfiguration slotConfiguration;

                slotConfiguration.m_name = k_onFalse;
                slotConfiguration.m_toolTip = "Signaled if the result of the operation is false.";
                slotConfiguration.SetConnectionType(ConnectionType::Output);

                AddSlot(slotConfiguration);
            }

            InitializeUnaryExpression();

        }

        void UnaryExpression::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<UnaryExpression, UnaryOperator>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<UnaryExpression>("UnaryExpression", "UnaryExpression")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }
    }
}

