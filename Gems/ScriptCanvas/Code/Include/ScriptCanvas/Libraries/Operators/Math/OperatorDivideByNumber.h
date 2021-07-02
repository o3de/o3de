/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "OperatorArithmetic.h"
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorDivideByNumber.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
#define DIVIDABLE_TYPES { Data::Type::Number(), Data::Type::Vector2(), Data::Type::Vector3(), Data::Type::Vector4() }

            //! Deprecated: see MethodOverloaded for "Divide by Number (/)"
            class OperatorDivideByNumber 
                : public Node
            {                
            public:

                SCRIPTCANVAS_NODE(OperatorDivideByNumber);

                void CustomizeReplacementNode(Node* replacementNode, AZStd::unordered_map<SlotId, AZStd::vector<SlotId>>& outSlotIdMap) const override;

                enum Version
                {
                    InitialVersion = 0,
                    RemoveOperatorBase,

                    Current
                };

                static bool OperatorDivideByNumberVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                OperatorDivideByNumber() = default;

                // Nodes...
                void OnInit() override;
                void OnInputSignal(const SlotId& slotId) override;
                ////

            protected:

                AZ::Crc32 GetDynamicGroupId() const { return AZ_CRC("DivideGroup", 0x66473fe4); }

                SlotId m_operandId;
            };
        }
#undef DIVIDABLE_TYPES
    }
}
