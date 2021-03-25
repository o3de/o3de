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

#include <Include/ScriptCanvas/Libraries/Time/HeartBeat.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //! Deprecated: see HeartBeatNodeableNode
            class HeartBeat
                : public ScriptCanvas::Nodes::Internal::BaseTimerNode
            {
            public:

                SCRIPTCANVAS_NODE(HeartBeat);

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

                void OnInputSignal(const SlotId&) override;

                const char* GetBaseTimeSlotName() const override { return "Interval"; }
                const char* GetBaseTimeSlotToolTip() const override { return "The amount of time between pulses."; }

            protected:

                void OnTimeElapsed() override;

             };
        }
    }
}
