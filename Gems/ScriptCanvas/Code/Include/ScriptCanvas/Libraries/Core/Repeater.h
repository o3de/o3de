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
#pragma once

#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>

#include <Include/ScriptCanvas/Libraries/Core/Repeater.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            //! A node that repeats an execution signal over the specified time
            class Repeater
                : public ScriptCanvas::Nodes::Internal::BaseTimerNode
            {
                SCRIPTCANVAS_NODE(Repeater);

            public:
                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

                void OnInit() override;

                void OnInputSignal(const SlotId& slotId) override;
                void OnTimeElapsed();

                const char* GetTimeSlotFormat() const override { return "Delay (%s)"; }

                const char* GetBaseTimeSlotName() const override { return "Interval"; }
                const char* GetBaseTimeSlotToolTip() const override { return "The Interval between repetitions"; }
                
            protected:

                bool AllowInstantResponse() const override { return true; }

                int m_repetionCount;
            };
        }
    }
}
