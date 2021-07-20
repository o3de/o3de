/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

#include "BinaryOperation.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            void Sum::Reflect(AZ::ReflectContext* reflection)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
                if (serializeContext)
                {
                    serializeContext->Class<Sum, Number>()
                        ->Version(2)
                        ->Field("A", &Sum::m_a)
                        ->Field("B", &Sum::m_b)
                        ;

                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<Sum>("Sum", "Performs the sum between two numbers.")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Sum.png")
                            ;
                    }

                }

                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
                if (behaviorContext)
                {
                    behaviorContext->Class<Sum>("Sum")
                        ->Method("In", &Sum::OnInputSignal)
                        ->Attribute(ScriptCanvas::Attributes::Input, true)
                        ->Method("Out", &Sum::SignalOutput)
                        ->Attribute(ScriptCanvas::Attributes::Output, true)
                        ->Property("A", BehaviorValueProperty(&Sum::m_a))
                        ->Property("B", BehaviorValueProperty(&Sum::m_b))
                        ->Property("This", BehaviorValueProperty(&Sum::m_sum))
                        ;
                }
            }

            Sum::Sum()
                : Number()
            {}

            Sum::~Sum()
            {}

            void Sum::OnEntry()
            {
                m_status = ExecutionStatus::Immediate;
                m_executionMode = ExecutionStatus::Immediate;
            }

            void Sum::OnInputSignal(const SlotID& slot)
            {
                //static const SlotID& inSlot = SlotID("In");

                //if (slot == inSlot)
                //{
                //    Types::Value* result = nullptr;

                //    if (auto value = azdynamic_cast<Types::ValueInt*>(Evaluate(SlotID("This"))))
                //    {
                //        m_value = value->Get();
                //    }
                //}
            }

            void Sum::OnExecute(double deltaTime)
            {
                if (m_status != ExecutionStatus::NotStarted)
                {
                    EvaluateSlot(SlotID("GetThis"));
                    SignalOutput(SlotID("Out"));
                }
            }

            AZ::BehaviorValueParameter Sum::EvaluateSlot(const ScriptCanvas::SlotID& slotId)
            {
                ScriptCanvas::Slot* slot = GetSlot(slotId);
                if (slot)
                {
                    if (slot->GetType() == ScriptCanvas::SlotType::Setter)
                    {
                        if (!slot->GetConnectionList().empty())
                        {
                            auto connection = slot->GetConnectionList()[0];

                            // Evaluate the node connected to this slot, we'll set our value according to it.
                            AZ::BehaviorValueParameter parameter;
                            ScriptCanvas::NodeServiceRequestBus::EventResult(parameter, connection.GetNodeId(), &NodeServiceRequests::EvaluateSlot, connection.GetSlotId());
                            return slot->GetProperty() ? ScriptCanvas::SafeSet<Types::ValueFloat>(parameter, slot->GetProperty()->m_setter, this) : parameter;
                        }
                    }
                    else if (slot->GetType() == ScriptCanvas::SlotType::Getter)
                    {
                        static const ScriptCanvas::SlotID aSlot("SetA");
                        static const ScriptCanvas::SlotID bSlot("SetB");

                        // Evaluating each slot will invoke its setter which, if there is a connection it will get evaluate and 
                        // return the connected value, otherwise we'll use the default value of the property.
                        EvaluateSlot(aSlot);
                        EvaluateSlot(bSlot);

                        // Both m_a and m_b have been resolved so we'll do the sum and return it.
                        m_sum = m_a.Get() + m_b.Get();

                        return AZ::BehaviorValueParameter(&m_sum);
                    }
                }

                // There was no connection to invoke a setter
                return AZ::BehaviorValueParameter();
            }

            Types::Value* Sum::Evaluate(const SlotID& slot)
            {
                AZ_Assert(false, "Deprecated");
                return nullptr;
            }
        }
    }
}
