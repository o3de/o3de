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
#include <ScriptCanvas/Internal/Nodes/StringFormatted.h>

#include <Include/ScriptCanvas/Libraries/String/Utilities.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! Deprecated: see class String's reflection of method "Starts With"
            class StartsWith
                : public Node
            {
            public:
                SCRIPTCANVAS_NODE(StartsWith);
            protected:

                void OnInputSignal(const SlotId& slotId) override;

            };

            //! Deprecated: see class String's reflection of method "Ends With"
            class EndsWith
                : public Node
            {
            public:
                SCRIPTCANVAS_NODE(EndsWith);
            protected:

                void OnInputSignal(const SlotId& slotId) override;

            };

            //! Deprecated: see class String's reflection of method "Ends With"
            class Split
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(Split);

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

            protected:

                static const char* k_defaultDelimiter;

                void OnInputSignal(const SlotId& slotId) override;

            };

            //! Deprecated: see class String's reflection of method "Join"
            class Join
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(Join);

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

            protected:

                static const char* k_defaultSeparator;

                void OnInputSignal(const SlotId& slotId) override;

            };

        }
    }
}
