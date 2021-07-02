/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OperatorLength.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>
#include <AzCore/Math/MathUtils.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            void OperatorLength::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
                auto newDataInSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                auto oldDataInSlots = this->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataIn);
                if (newDataInSlots.size() == oldDataInSlots.size())
                {
                    for (size_t index = 0; index < newDataInSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataInSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataInSlots[index]->GetId() });
                    }
                }

                auto newDataOutSlots = replacementNode->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                auto oldDataOutSlots = this->GetSlotsByType(ScriptCanvas::CombinedSlotType::DataOut);
                if (newDataOutSlots.size() == oldDataOutSlots.size())
                {
                    for (size_t index = 0; index < newDataOutSlots.size(); index++)
                    {
                        outSlotIdMap.emplace(oldDataOutSlots[index]->GetId(), AZStd::vector<SlotId>{ newDataOutSlots[index]->GetId() });
                    }
                }
            }

            bool OperatorLength::OperatorLengthConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() < Version::RemoveOperatorBase)
                {
                    if (!SerializationUtils::RemoveBaseClass(serializeContext, rootElement))
                    {
                        return false;
                    }

                    if (!SerializationUtils::RemoveBaseClass(serializeContext, rootElement))
                    {
                        return false;
                    }

                    rootElement.RemoveElementByName(AZ::Crc32("BaseClass2"));
                }

                return true;
            }

            void OperatorLength::OnInit()
            {
                Slot* slot = GetSlotByName("Length");

                if (slot == nullptr)
                {
                    ConfigureSlots();
                }
            }

            void OperatorLength::OnInputSignal(const SlotId& slotId)
            {
                if (slotId != OperatorLengthProperty::GetInSlotId(this))
                {
                    return;
                }

                Data::Type type = GetDisplayType(AZ::Crc32("SourceGroup"));

                if (!type.IsValid())
                {
                    return;
                }

                Datum result;
                const Datum* operand = FindDatum(OperatorLengthProperty::GetSourceSlotId(this));

                switch (type.GetType())
                {
                case Data::eType::Vector2:
                {
                    const AZ::Vector2* vector = operand->GetAs<AZ::Vector2>();
                    result = Datum(vector->GetLength());
                }
                break;
                case Data::eType::Vector3:
                {
                    const AZ::Vector3* vector = operand->GetAs<AZ::Vector3>();
                    result = Datum(vector->GetLength());
                }
                break;
                case Data::eType::Vector4:
                {
                    const AZ::Vector4* vector = operand->GetAs<AZ::Vector4>();
                    result = Datum(vector->GetLength());
                }
                break;
                case Data::eType::Quaternion:
                {
                    const AZ::Quaternion* vector = operand->GetAs<AZ::Quaternion>();
                    result = Datum(vector->GetLength());
                }
                break;
                default:
                    AZ_Assert(false, "Length operator not defined for type: %s", Data::ToAZType(type).ToString<AZStd::string>().c_str());
                    break;
                }

                PushOutput(result, (*OperatorLengthProperty::GetLengthSlot(this)));
                SignalOutput(OperatorLengthProperty::GetOutSlotId(this));
            }
        }
    }
}
