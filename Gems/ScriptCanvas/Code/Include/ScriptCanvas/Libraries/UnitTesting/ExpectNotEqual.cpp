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

#include "ExpectNotEqual.h"

#include "UnitTestBus.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            void ExpectNotEqual::OnInit()
            {
                // DYNAMIC_SLOT_VERSION_CONVERTER
                auto candidateSlot = GetSlot(GetSlotId("Candidate"));

                if (candidateSlot)
                {
                    if (!candidateSlot->IsDynamicSlot())
                    {
                        candidateSlot->SetDynamicDataType(DynamicDataType::Any);
                    }

                    if (candidateSlot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(candidateSlot->GetId(), AZ_CRC("DynamicGroup", 0x219a2e3a));
                    }
                }

                auto referenceSlot = GetSlot(GetSlotId("Reference"));

                if (referenceSlot)
                {
                    if (!referenceSlot->IsDynamicSlot())
                    {
                        referenceSlot->SetDynamicDataType(DynamicDataType::Any);
                    }

                    if (referenceSlot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(referenceSlot->GetId(), AZ_CRC("DynamicGroup", 0x219a2e3a));
                    }
                }
                ////
            }

            void ExpectNotEqual::OnInputSignal([[maybe_unused]] const SlotId& slotId)
            {
                auto lhs = FindDatum(GetSlotId("Candidate"));
                if (!lhs)
                {
                    return;
                }

                auto rhs = FindDatum(GetSlotId("Reference"));
                if (!rhs)
                {
                    return;
                }

                if (lhs->GetType() != rhs->GetType())
                {
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::AddFailure
                        , "Type mismatch in comparison operator");
                    
                    SignalOutput(GetSlotId("Out"));
                    return; 
                }

                const auto report = FindDatum(GetSlotId("Report"))->GetAs<Data::StringType>();

                switch (lhs->GetType().GetType())
                {

                case Data::eType::AABB:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualAABB
                        , *lhs->GetAs<Data::AABBType>()
                        , *rhs->GetAs<Data::AABBType>()
                        , *report);
                    break;

                case Data::eType::Boolean:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualBoolean
                        , *lhs->GetAs<Data::BooleanType>()
                        , *rhs->GetAs<Data::BooleanType>()
                        , *report);
                    break;

                case Data::eType::CRC:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualCRC
                        , *lhs->GetAs<Data::CRCType>()
                        , *rhs->GetAs<Data::CRCType>()
                        , *report);
                    break;

                case Data::eType::Color:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualColor
                        , *lhs->GetAs<Data::ColorType>()
                        , *rhs->GetAs<Data::ColorType>()
                        , *report);
                    break;

                case Data::eType::EntityID:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualEntityID
                        , *lhs->GetAs<Data::EntityIDType>()
                        , *rhs->GetAs<Data::EntityIDType>()
                        , *report);
                    break;

                case Data::eType::Matrix3x3:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualMatrix3x3
                        , *lhs->GetAs<Data::Matrix3x3Type>()
                        , *rhs->GetAs<Data::Matrix3x3Type>()
                        , *report);
                    break;

                case Data::eType::Matrix4x4:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualMatrix4x4
                        , *lhs->GetAs<Data::Matrix4x4Type>()
                        , *rhs->GetAs<Data::Matrix4x4Type>()
                        , *report);
                    break;

                case Data::eType::Number:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualNumber
                        , *lhs->GetAs<Data::NumberType>()
                        , *rhs->GetAs<Data::NumberType>()
                        , *report);
                    break;

                case Data::eType::OBB:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualOBB
                        , *lhs->GetAs<Data::OBBType>()
                        , *rhs->GetAs<Data::OBBType>()
                        , *report);
                    break;

                case Data::eType::Plane:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualPlane
                        , *lhs->GetAs<Data::PlaneType>()
                        , *rhs->GetAs<Data::PlaneType>()
                        , *report);
                    break;

                case Data::eType::Quaternion:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualQuaternion
                        , *lhs->GetAs<Data::QuaternionType>()
                        , *rhs->GetAs<Data::QuaternionType>()
                        , *report);
                    break;

                case Data::eType::String:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualString
                        , *lhs->GetAs<Data::StringType>()
                        , *rhs->GetAs<Data::StringType>()
                        , *report);
                    break;

                case Data::eType::Transform:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualTransform
                        , *lhs->GetAs<Data::TransformType>()
                        , *rhs->GetAs<Data::TransformType>()
                        , *report);
                    break;

                case Data::eType::Vector2:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualVector2
                        , *lhs->GetAs<Data::Vector2Type>()
                        , *rhs->GetAs<Data::Vector2Type>()
                        , *report);
                    break;

                case Data::eType::Vector3:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualVector3
                        , *lhs->GetAs<Data::Vector3Type>()
                        , *rhs->GetAs<Data::Vector3Type>()
                        , *report);
                    break;

                case Data::eType::Vector4:
                    ScriptCanvas::UnitTesting::Bus::Event
                        ( GetOwningScriptCanvasId()
                        , &ScriptCanvas::UnitTesting::BusTraits::ExpectNotEqualVector4
                        , *lhs->GetAs<Data::Vector4Type>()
                        , *rhs->GetAs<Data::Vector4Type>()
                        , *report);
                    break;

                }

                SignalOutput(GetSlotId("Out"));
            }
        } // namespace UnitTesting
    } // namespace Nodes
} // namespace ScriptCanvas
