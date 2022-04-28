/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UnitTestBusSender.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Core/Attributes.h>

namespace ScriptCanvas
{
    namespace UnitTesting
    {        
        void EventSender::AddFailure(const ExecutionState* executionState, const Report& report)
        {
            Bus::Event(executionState->GetAssetId(), &BusTraits::AddFailure, report);
        }

        void EventSender::AddSuccess(const ExecutionState* executionState, const Report& report)
        {
            Bus::Event(executionState->GetAssetId(), &BusTraits::AddSuccess, report);
        }

        void EventSender::Checkpoint(const ExecutionState* executionState, const Report& report)
        {
            Bus::Event(executionState->GetAssetId(), &BusTraits::Checkpoint, report);
        }

        void EventSender::ExpectFalse(const ExecutionState* executionState, const bool value, const Report& report)
        {
            Bus::Event(executionState->GetAssetId(), &BusTraits::ExpectFalse, value, report);
        }

        void EventSender::ExpectTrue(const ExecutionState* executionState, const bool value, const Report& report)
        {
            Bus::Event(executionState->GetAssetId(), &BusTraits::ExpectTrue, value, report);
        }

        void EventSender::MarkComplete(const ExecutionState* executionState, const Report& report)
        {
            Bus::Event(executionState->GetAssetId(), &BusTraits::MarkComplete, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::AABBType candidate, const Data::AABBType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::AABBType, const Data::AABBType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::BooleanType candidate, const Data::BooleanType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::BooleanType, const Data::BooleanType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::ColorType candidate, const Data::ColorType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::ColorType, const Data::ColorType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::CRCType candidate, const Data::CRCType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::CRCType, const Data::CRCType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::EntityIDType candidate, const Data::EntityIDType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::EntityIDType, const Data::EntityIDType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::Matrix3x3Type candidate, const Data::Matrix3x3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Matrix3x3Type, const Data::Matrix3x3Type, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::Matrix4x4Type candidate, const Data::Matrix4x4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Matrix4x4Type, const Data::Matrix4x4Type, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::NumberType, const Data::NumberType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::OBBType candidate, const Data::OBBType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::OBBType, const Data::OBBType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::PlaneType candidate, const Data::PlaneType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::PlaneType, const Data::PlaneType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::QuaternionType candidate, const Data::QuaternionType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::QuaternionType, const Data::QuaternionType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::StringType, const Data::StringType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::TransformType candidate, const Data::TransformType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::TransformType, const Data::TransformType, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector2Type, const Data::Vector2Type, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector3Type, const Data::Vector3Type, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector4Type, const Data::Vector4Type, const Report&) = &BusTraits::ExpectEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::AABBType candidate, const Data::AABBType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::AABBType, const Data::AABBType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::BooleanType candidate, const Data::BooleanType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::BooleanType, const Data::BooleanType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::ColorType candidate, const Data::ColorType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::ColorType, const Data::ColorType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::CRCType candidate, const Data::CRCType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::CRCType, const Data::CRCType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::EntityIDType candidate, const Data::EntityIDType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::EntityIDType, const Data::EntityIDType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::Matrix3x3Type candidate, const Data::Matrix3x3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Matrix3x3Type, const Data::Matrix3x3Type, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::Matrix4x4Type candidate, const Data::Matrix4x4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Matrix4x4Type, const Data::Matrix4x4Type, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::NumberType, const Data::NumberType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::OBBType candidate, const Data::OBBType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::OBBType, const Data::OBBType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::PlaneType candidate, const Data::PlaneType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::PlaneType, const Data::PlaneType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::QuaternionType candidate, const Data::QuaternionType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::QuaternionType, const Data::QuaternionType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::StringType, const Data::StringType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::TransformType candidate, const Data::TransformType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::TransformType, const Data::TransformType, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector2Type, const Data::Vector2Type, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector3Type, const Data::Vector3Type, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectNotEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector4Type, const Data::Vector4Type, const Report&) = &BusTraits::ExpectNotEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThan(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::NumberType, const Data::NumberType, const Report&) = &BusTraits::ExpectGreaterThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThan(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::StringType, const Data::StringType, const Report&) = &BusTraits::ExpectGreaterThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThan(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector2Type, const Data::Vector2Type, const Report&) = &BusTraits::ExpectGreaterThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThan(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector3Type, const Data::Vector3Type, const Report&) = &BusTraits::ExpectGreaterThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThan(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector4Type, const Data::Vector4Type, const Report&) = &BusTraits::ExpectGreaterThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::NumberType, const Data::NumberType, const Report&) = &BusTraits::ExpectGreaterThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::StringType, const Data::StringType, const Report&) = &BusTraits::ExpectGreaterThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector2Type, const Data::Vector2Type, const Report&) = &BusTraits::ExpectGreaterThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector3Type, const Data::Vector3Type, const Report&) = &BusTraits::ExpectGreaterThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector4Type, const Data::Vector4Type, const Report&) = &BusTraits::ExpectGreaterThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThan(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::NumberType, const Data::NumberType, const Report&) = &BusTraits::ExpectLessThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThan(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::StringType, const Data::StringType, const Report&) = &BusTraits::ExpectLessThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThan(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector2Type, const Data::Vector2Type, const Report&) = &BusTraits::ExpectLessThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThan(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector3Type, const Data::Vector3Type, const Report&) = &BusTraits::ExpectLessThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThan(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector4Type, const Data::Vector4Type, const Report&) = &BusTraits::ExpectLessThan;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThanEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::NumberType, const Data::NumberType, const Report&) = &BusTraits::ExpectLessThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThanEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::StringType, const Data::StringType, const Report&) = &BusTraits::ExpectLessThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThanEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector2Type, const Data::Vector2Type, const Report&) = &BusTraits::ExpectLessThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThanEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector3Type, const Data::Vector3Type, const Report&) = &BusTraits::ExpectLessThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::ExpectLessThanEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report)
        {
            void(BusTraits:: * f)(const Data::Vector4Type, const Data::Vector4Type, const Report&) = &BusTraits::ExpectLessThanEqual;
            Bus::Event(executionState->GetAssetId(), f, candidate, reference, report);
        }

        void EventSender::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<EventSender>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EventSender>("Unit Testing", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                            Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/UnitTesting.png")->
                            Attribute(AZ::Edit::Attributes::CategoryStyle, ".method")->
                            Attribute(AZ::Edit::Attributes::Category, "Utilities/Unit Testing")->
                            Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TestingNodeTitlePalette")
                        ;
                }
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
            {
                AZ::ScriptCanvasAttributes::HiddenIndices uniqueIdIndex = { 0 };

                auto builder = behaviorContext->Class<EventSender>("Unit Testing");
                builder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common);

                builder->Method("Add Failure", &EventSender::AddFailure, { { {"", "", nullptr}, {"Report", "additional notes for the test report"} } })
                        ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Method("Add Success", &EventSender::AddSuccess, { { {"", "", nullptr}, {"Report", "additional notes for the test report"}  } })
                        ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Method("Checkpoint", &EventSender::Checkpoint, { { {"", "", nullptr}, {"Report", "additional notes for the test report"}  } })
                        ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Method("Expect False", &EventSender::ExpectFalse, { { {"", "", nullptr}, {"Candidate", "a value that must be false"}, {"Report", "additional notes for the test report"}  } })
                        ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Method("Expect True", &EventSender::ExpectTrue, { { {"", "", nullptr}, {"Candidate", "a value that must be true"}, {"Report", "additional notes for the test report"} } })
                        ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Method("Mark Complete", &EventSender::MarkComplete, { { {"", "", nullptr}, {"Report", "additional notes for the test report"} } })
                        ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ;

                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::AABBType, const Data::AABBType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::BooleanType, const Data::BooleanType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::CRCType, const Data::CRCType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::ColorType, const Data::ColorType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::EntityIDType, const Data::EntityIDType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::Matrix3x3Type, const Data::Matrix3x3Type, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::Matrix4x4Type, const Data::Matrix4x4Type, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::NumberType, const Data::NumberType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::OBBType, const Data::OBBType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::PlaneType, const Data::PlaneType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::QuaternionType, const Data::QuaternionType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::StringType, const Data::StringType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::TransformType, const Data::TransformType, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector2Type, const Data::Vector2Type, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector3Type, const Data::Vector3Type, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector4Type, const Data::Vector4Type, const Report&)>(&EventSender::ExpectEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of =="}, {"Reference", "right of =="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));


                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::AABBType, const Data::AABBType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::BooleanType, const Data::BooleanType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::CRCType, const Data::CRCType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::ColorType, const Data::ColorType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::EntityIDType, const Data::EntityIDType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::Matrix3x3Type, const Data::Matrix3x3Type, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::Matrix4x4Type, const Data::Matrix4x4Type, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::NumberType, const Data::NumberType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::OBBType, const Data::OBBType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::PlaneType, const Data::PlaneType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::QuaternionType, const Data::QuaternionType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::StringType, const Data::StringType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::TransformType, const Data::TransformType, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector2Type, const Data::Vector2Type, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector3Type, const Data::Vector3Type, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Not Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector4Type, const Data::Vector4Type, const Report&)>(&EventSender::ExpectNotEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of !="}, {"Reference", "right of !="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));

                builder->Method("Expect Greater Than", static_cast<void(*)(const ExecutionState*, const Data::NumberType, const Data::NumberType, const Report&)>(&EventSender::ExpectGreaterThan)
                    , { { {"", "", nullptr}, {"Candidate", "left of >"}, {"Reference", "right of >"}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Greater Than", static_cast<void(*)(const ExecutionState*, const Data::StringType, const Data::StringType, const Report&)>(&EventSender::ExpectGreaterThan)
                    , { { {"", "", nullptr}, {"Candidate", "left of >"}, {"Reference", "right of >"}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));

                builder->Method("Expect Greater Than Equal", static_cast<void(*)(const ExecutionState*, const Data::NumberType, const Data::NumberType, const Report&)>(&EventSender::ExpectGreaterThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of >="}, {"Reference", "right of >="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Greater Than Equal", static_cast<void(*)(const ExecutionState*, const Data::StringType, const Data::StringType, const Report&)>(&EventSender::ExpectGreaterThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of >="}, {"Reference", "right of >="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Greater Than Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector2Type, const Data::Vector2Type, const Report&)>(&EventSender::ExpectGreaterThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of >="}, {"Reference", "right of >="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Greater Than Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector3Type, const Data::Vector3Type, const Report&)>(&EventSender::ExpectGreaterThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of >="}, {"Reference", "right of >="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Greater Than Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector4Type, const Data::Vector4Type, const Report&)>(&EventSender::ExpectGreaterThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of >="}, {"Reference", "right of >="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));

                builder->Method("Expect Less Than", static_cast<void(*)(const ExecutionState*, const Data::NumberType, const Data::NumberType, const Report&)>(&EventSender::ExpectLessThan)
                    , { { {"", "", nullptr}, {"Candidate", "left of <"}, {"Reference", "right of <"}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than", static_cast<void(*)(const ExecutionState*, const Data::StringType, const Data::StringType, const Report&)>(&EventSender::ExpectLessThan)
                    , { { {"", "", nullptr}, {"Candidate", "left of <"}, {"Reference", "right of <"}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than", static_cast<void(*)(const ExecutionState*, const Data::Vector2Type, const Data::Vector2Type, const Report&)>(&EventSender::ExpectLessThan)
                    , { { {"", "", nullptr}, {"Candidate", "left of <"}, {"Reference", "right of <"}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than", static_cast<void(*)(const ExecutionState*, const Data::Vector3Type, const Data::Vector3Type, const Report&)>(&EventSender::ExpectLessThan)
                    , { { {"", "", nullptr}, {"Candidate", "left of <"}, {"Reference", "right of <"}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than", static_cast<void(*)(const ExecutionState*, const Data::Vector4Type, const Data::Vector4Type, const Report&)>(&EventSender::ExpectLessThan)
                    , { { {"", "", nullptr}, {"Candidate", "left of <"}, {"Reference", "right of <"}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));

                builder->Method("Expect Less Than Equal", static_cast<void(*)(const ExecutionState*, const Data::NumberType, const Data::NumberType, const Report&)>(&EventSender::ExpectLessThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of <="}, {"Reference", "right of <="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than Equal", static_cast<void(*)(const ExecutionState*, const Data::StringType, const Data::StringType, const Report&)>(&EventSender::ExpectLessThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of <="}, {"Reference", "right of <="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector2Type, const Data::Vector2Type, const Report&)>(&EventSender::ExpectLessThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of <="}, {"Reference", "right of <="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector3Type, const Data::Vector3Type, const Report&)>(&EventSender::ExpectLessThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of <="}, {"Reference", "right of <="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                builder->Method("Expect Less Than Equal", static_cast<void(*)(const ExecutionState*, const Data::Vector4Type, const Data::Vector4Type, const Report&)>(&EventSender::ExpectLessThanEqual)
                    , { { {"", "", nullptr}, {"Candidate", "left of <="}, {"Reference", "right of <="}, {"Report", "additional notes for the test report"} } })
                    ->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex)
                    ->Attribute(AZ::ScriptCanvasAttributes::OverloadArgumentGroup, AZ::OverloadArgumentGroupInfo({ "", "CandidateAndReference", "CandidateAndReference", "" }, {}));
                ;
            }
        }
    } // namespace UnitTesting
} // namespace ScriptCanvas 
