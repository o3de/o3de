/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

#include <Include/ScriptCanvas/Libraries/Logic/Sequencer.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Deprecated: see Ordered Sequencer
            class Sequencer
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(Sequencer);

                Sequencer();

                enum Order
                {
                    Forward = 0,
                    Backward
                };

                NodeReplacementConfiguration GetReplacementNodeConfiguration() const override;
                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

            protected:

                int m_order;
                int m_selectedIndex;

            private:
                int m_currentIndex;
                bool m_outputIsValid;

                SlotId GetCurrentSlotId() const;
            };
        }
    }
}
