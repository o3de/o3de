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

#include "Sequencer.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            static const int NUMBER_OF_OUTPUTS = 8;

            Sequencer::Sequencer()
                : Node()
                , m_selectedIndex(0)
                , m_currentIndex(0)
                , m_order(0)
                , m_outputIsValid(true)
            {}

            void  Sequencer::OnInputSignal(const SlotId& slot)
            {
                m_selectedIndex = SequencerProperty::GetIndex(this);
                m_order = SequencerProperty::GetOrder(this);

                const SlotId inSlot = SequencerProperty::GetInSlotId(this);
                const SlotId nextSlot = SequencerProperty::GetNextSlotId(this);
                
                if (slot == inSlot)
                {
                    m_currentIndex = m_selectedIndex;
                } 
                else if (slot == nextSlot)
                {
                    int step = m_order == Order::Forward ? 1 : -1;

                    m_outputIsValid = false;
                    int startIndex = m_currentIndex;
                    while (!m_outputIsValid)
                    {
                        m_currentIndex = (m_currentIndex + step + NUMBER_OF_OUTPUTS) % NUMBER_OF_OUTPUTS;
                        SlotId outSlotId = GetCurrentSlotId();
                        Slot* outSlot = GetSlot(outSlotId);
                        if (outSlot)
                        {
                            m_outputIsValid = !GetConnectedNodes(*outSlot).empty();
                        }

                        //Avoid infinite loop when none of the outputs or only the current output connects to other nodes.
                        if (m_currentIndex == startIndex)
                        {
                            m_outputIsValid = false;
                            break;
                        }
                    }
                }

                if (m_outputIsValid)
                {
                    SlotId outSlotId = GetCurrentSlotId();
                    SignalOutput(outSlotId);
                }
            }

            SlotId Sequencer::GetCurrentSlotId() const
            {
                AZStd::string slotName = "Out" + AZStd::to_string(m_currentIndex);
                SlotId outSlotId = GetSlotId(slotName.c_str());
                return outSlotId;
            }
        }
    }
}
