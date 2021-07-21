/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "MathExpression.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {

            void MathExpression::ConfigureSlotDisplayType(ScriptCanvas::Node* node, ScriptCanvas::Slot* slot, AZ::Uuid type, AZStd::string name)
            {
                if (slot)
                {
                    auto displayType = ScriptCanvas::Data::FromAZType(type);
                    if (displayType.IsValid())
                    {
                        AZ::Crc32 dynamicGroup = AZ_CRC("ExpressionDisplayGroup", 0x770de38e);
                        //AZ::Crc32 dynamicGroup = slot->GetDynamicGroup();

                        if (dynamicGroup != AZ::Crc32())
                        {
                            node->SetDisplayType(dynamicGroup, displayType);
                            Slot* resultSlot = node->GetSlot(node->GetSlotId("Result"));
                            resultSlot->SetDynamicDataType(DynamicDataType::Any);
                            resultSlot->SetDisplayType(displayType);
                            
                        }
                        else
                        {
                            slot->SetDisplayType(displayType);
                        }
                    }

                    //slot->Rename(name);
                }
            }


            ExpressionEvaluation::ParseOutcome MathExpression::ParseExpression(const AZStd::string& formatString)
            {
                const AZStd::unordered_set<ExpressionEvaluation::ExpressionParserId> mathInterfaceSet = {
                    ExpressionEvaluation::Interfaces::NumericPrimitives,
                    ExpressionEvaluation::Interfaces::MathOperators
                };

                ExpressionEvaluation::ParseOutcome outcome;
                ExpressionEvaluation::ExpressionEvaluationRequestBus::BroadcastResult(outcome, &ExpressionEvaluation::ExpressionEvaluationRequests::ParseRestrictedExpression, mathInterfaceSet, formatString);

                return outcome;
            }

            AZStd::string MathExpression::GetExpressionSeparator() const
            {
                return " + ";
            }

            AZStd::optional<NodeRequests::SlotSelectionInfo> MathExpression::CreateSlotTypeSelectorInfo([[maybe_unused]] const SlotId& slotID)
            {
                ScriptCanvas::NodeRequests::SlotSelectionInfo info;
                info.m_restrictedTypes= { ToAZType(ScriptCanvas::Data::Type::Number()), ToAZType(ScriptCanvas::Data::Type::Vector3()) };
                return info;
            }
        }
    }
}
