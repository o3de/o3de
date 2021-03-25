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

#include "Replace.h"

#include <AzFramework/StringFunc/StringFunc.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            void Replace::OnInit()
            {
                // Version Conversion Code for a slot renaming.
                Slot* slot = GetSlotByName("Ignore Case");

                if (slot)
                {
                    RenameSlot((*slot), "Case Sensitive");

                    ModifiableDatumView datumView;
                    ModifyUnderlyingSlotDatum(slot->GetId(), datumView);

                    if (datumView.IsValid() && datumView.IsType(ScriptCanvas::Data::Type::Boolean()))
                    {
                        const bool* datumValue = datumView.GetAs<ScriptCanvas::Data::BooleanType>();

                        Datum newDatum = Datum(!(*datumValue));
                        datumView.AssignToDatum(newDatum);
                    }
                }
                ////
            }

            void Replace::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
            {
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

            void Replace::OnInputSignal(const SlotId&)
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Replace::OnInputSignal");

                AZStd::string sourceString = ReplaceProperty::GetSource(this);
                AZStd::string replaceString = ReplaceProperty::GetReplace(this);
                AZStd::string withString = ReplaceProperty::GetWith(this);

                bool caseSensitive = ReplaceProperty::GetCaseSensitive(this);
                
                AzFramework::StringFunc::Replace(sourceString, replaceString.c_str(), withString.c_str(), caseSensitive);

                const SlotId outputResultSlotId = ReplaceProperty::GetResultSlotId(this);

                ScriptCanvas::Datum output(sourceString);
                if (auto* outputSlot = GetSlot(outputResultSlotId))
                {
                    PushOutput(output, *outputSlot);
                }

                SignalOutput(GetSlotId("Out"));
            }
        }
    }
}
