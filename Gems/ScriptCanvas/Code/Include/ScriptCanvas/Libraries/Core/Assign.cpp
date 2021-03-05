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

#include "Assign.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void Assign::OnInit()
            {
                AddSlot("In", "", SlotType::ExecutionIn);
                AddSlot("Out", "", SlotType::ExecutionOut);
                AddInputDatumDynamicTypedSlot("Source");
                AddSlot("Target", "", SlotType::DataOut);
            }
            
            Data::Type Assign::GetSlotDataType(const SlotId& slotId) const
            {
                if (Slot* sourceSlot = GetSlot(GetSlotId("Source")))
                {
                    auto inputs = GetConnectedNodes(*sourceSlot);
                    // contracts should enforce this, but in case it breaks...
                    if (inputs.size() == 1)
                    {
                        auto input = *inputs.begin();
                        return input.first->GetSlotDataType(input.second);
                    }
                    else
                    {
                        AZ_Error("ScriptCanvas", (inputs.size() == 0), "Multiple inputs to 'Assign' are forbidden");
                    }
                }

                return Data::Type::Invalid();
            }

            void Assign::OnInputSignal(const SlotId&)
            {
                if (auto input = GetDatumByIndex(k_sourceInputIndex))
                {
                    PushOutput(*input, *GetSlotByIndex(k_targetSlotIndex));
                }

                SignalOutput(GetSlotId("Out"));
            }

            bool Assign::SlotAcceptsType(const SlotId& slotID, const Data::Type& type) const 
            {
                Slot* sourceSlot = GetSlot(GetSlotId("Source"));
                Slot* targetSlot = GetSlot(GetSlotId("Target"));
                return sourceSlot 
                    && targetSlot
                    && DynamicSlotAcceptsType(slotID, type, Node::DynamicTypeArity::Single, *targetSlot, AZStd::vector<Slot*>{sourceSlot});
            }

            void Assign::Reflect(AZ::ReflectContext* reflectContext)
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
                {
                    serializeContext->Class<Assign, Node>()
                        ->Version(0)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<Assign>("Assign", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Placeholder.png")
                            ;
                    }
                }
            }
        }
    }
}
