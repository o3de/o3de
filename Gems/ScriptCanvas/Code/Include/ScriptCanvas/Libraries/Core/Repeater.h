/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
