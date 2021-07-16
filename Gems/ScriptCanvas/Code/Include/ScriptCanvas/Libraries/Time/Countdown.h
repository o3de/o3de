/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>

#include <Include/ScriptCanvas/Libraries/Time/Countdown.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //! Deprecated: see TimeDelayNodeableNode
            class TimeDelay
                : public ScriptCanvas::Nodes::Internal::BaseTimerNode
            {

            public:

                SCRIPTCANVAS_NODE(TimeDelay);

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

                TimeDelay() = default;


                bool AllowInstantResponse() const override;

                const char* GetBaseTimeSlotToolTip() const override { return "The amount of time to delay before the Out is signalled."; }

            };

            // Deprecated: see TimeDelayNodeableNode
            class TickDelay
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(TickDelay);

                TickDelay();

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

            protected:

                int m_tickCounter;
                int m_tickOrder;
            };

            //! Deprecated: see DelayNodeableNode
            class Countdown
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Countdown);

                Countdown();

            protected:

                float m_countdownSeconds;
                bool m_looping;
                float m_holdTime;

                //! Internal, used to determine if the node is holding before looping
                bool m_holding;

                //! Internal counter to track time elapsed
                float m_currentTime;

                bool ShowHoldTime() const
                {
                    // TODO: This only works on the property grid. If a true value is connected to the "SetLoop" slot,
                    // We need to show the "Hold Before Loop" slot, otherwise we need to hide it.
                    return m_looping;
                }

                bool IsOutOfDate(const VersionData& graphVersion) const override;

            protected:

                UpdateResult OnUpdateNode() override;

            };
        }
    }
}
