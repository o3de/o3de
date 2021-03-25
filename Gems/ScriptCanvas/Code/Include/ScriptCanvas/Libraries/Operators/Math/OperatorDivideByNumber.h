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
