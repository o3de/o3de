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

#include "OperatorEmpty.h"
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Core/Contracts/SupportsMethodContract.h>

#include <ScriptCanvas/Utils/SerializationUtils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //////////////////
            // OperatorEmpty
            //////////////////
            bool OperatorEmpty::OperatorEmptyVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() < 1)
                {
                    // Remove OperatorBase
                    if (!SerializationUtils::RemoveBaseClass(context, classElement))
                    {
                        return false;
                    }
                }

                return true;
            }

            void OperatorEmpty::OnInit()
            {
                // Version Conversion away from Operator Base
                if (HasSlots())
                {
                    const Slot* slot = GetSlot(OperatorEmptyProperty::GetIsEmptySlotId(this));
                    if (slot == nullptr)
                    {
                        ConfigureSlots();
                    }
                }
                ////
            }

            void OperatorEmpty::OnInputSignal(const SlotId& slotId)
            {
                const SlotId inSlotId = OperatorEmptyProperty::GetInSlotId(this);
                if (slotId == inSlotId)
                {
                    SlotId sourceSlotId = OperatorEmptyProperty::GetSourceSlotId(this);

                    const Datum* containerDatum = FindDatum(sourceSlotId);

                    if (Datum::IsValidDatum(containerDatum))
                    {
                        // Is the container empty?
                        auto emptyOutcome = BehaviorContextMethodHelper::CallMethodOnDatum(*containerDatum, "Empty");
                        if (!emptyOutcome)
                        {
                            SCRIPTCANVAS_REPORT_ERROR((*this), "Failed to call Empty on container: %s", emptyOutcome.GetError().c_str());
                            return;
                        }

                        Datum emptyResult = emptyOutcome.TakeValue();
                        bool isEmpty = *emptyResult.GetAs<bool>();

                        PushOutput(Datum(isEmpty), *GetSlot(OperatorEmptyProperty::GetIsEmptySlotId(this)));

                        if (isEmpty)
                        {
                            SignalOutput(OperatorEmptyProperty::GetTrueSlotId(this));
                        }
                        else
                        {
                            SignalOutput(OperatorEmptyProperty::GetFalseSlotId(this));
                        }
                    }
                    else
                    {
                        PushOutput(Datum(true), *GetSlot(OperatorEmptyProperty::GetIsEmptySlotId(this)));
                        SignalOutput(OperatorEmptyProperty::GetFalseSlotId(this));
                    }

                    SignalOutput(OperatorEmptyProperty::GetOutSlotId(this));
                }
            }
        }
    }
}
