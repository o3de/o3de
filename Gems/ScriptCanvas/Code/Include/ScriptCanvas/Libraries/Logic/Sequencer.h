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

                NodeConfiguration GetReplacementNodeConfiguration() const override;
                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

            protected:

                int m_order;
                int m_selectedIndex;

            protected:

                void OnInputSignal(const SlotId& slot) override;

            private:
                int m_currentIndex;
                bool m_outputIsValid;

                SlotId GetCurrentSlotId() const;
            };
        }
    }
}
