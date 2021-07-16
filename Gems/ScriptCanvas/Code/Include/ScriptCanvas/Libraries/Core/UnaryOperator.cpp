/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

