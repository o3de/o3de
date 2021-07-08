/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Contains.h"

#include <AzFramework/StringFunc/StringFunc.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            void Contains::OnInit()
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

            void Contains::CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const
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

        }
    }
}
