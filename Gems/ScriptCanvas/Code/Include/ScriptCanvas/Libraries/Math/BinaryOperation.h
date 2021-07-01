/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
