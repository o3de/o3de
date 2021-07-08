/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorEmpty.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //! Deprecated: see MethodOverloaded for "Is Empty"
            class OperatorEmpty : public Node
            {
            public:
                SCRIPTCANVAS_NODE(OperatorEmpty);

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

                static bool OperatorEmptyVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

                OperatorEmpty() = default;

                void OnInit() override;
            };
        }
    }
}
