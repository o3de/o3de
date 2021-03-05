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

#include <Core/Node.h>

namespace ScriptCanvas
{
    class NodeVisitor;

    namespace Nodes
    {
        class BinaryOperation
            : public Number
        {
        public:

            AZ_COMPONENT(BinaryOperation, "{04798FF9-50EE-487E-9433-B2C4F0FE4D37}", Node);

            static void Reflect(AZ::ReflectContext* reflection);

            BinaryOperation();
            ~BinaryOperation() override;

            void OnInputSignal(const SlotID& slot) override;
            Types::Value* Evaluate(const SlotID& slot) override;

            AZ::BehaviorValueParameter EvaluateSlot(const ScriptCanvas::SlotID&) override;

            void Visit(NodeVisitor& visitor) const override { visitor.Visit(*this); }

        protected:

            Types::ValueFloat m_a;
            Types::ValueFloat m_b;
            Types::ValueFloat m_sum;

        };
    }
}