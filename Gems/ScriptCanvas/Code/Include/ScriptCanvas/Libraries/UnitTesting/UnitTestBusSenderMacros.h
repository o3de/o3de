/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#define SCRIPT_CANVAS_UNIT_TEST_EQUALITY_TYPES(OVERLOAD, NAME, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, AABB, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Boolean, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, CRC, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Color, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, EntityID, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Matrix3x3, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Matrix4x4, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Number, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, OBB, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Plane, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Quaternion, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, String, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Transform, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Vector2, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Vector3, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Vector4, PARAM0, PARAM1, PARAM2)

#define SCRIPT_CANVAS_UNIT_TEST_COMPARE_TYPES(OVERLOAD, NAME, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, Number, PARAM0, PARAM1, PARAM2)\
    OVERLOAD(NAME, String, PARAM0, PARAM1, PARAM2)

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_DECLARATION(NAME, TYPE, PARAM0, PARAM1, PARAM2)\
    static void NAME(const AZ::EntityId& graphUniqueId, const Data::TYPE##Type candidate, const Data::TYPE##Type reference, const Report& report);

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_IMPLEMENTATION(NAME, TYPE, PARAM0, PARAM1, PARAM2)\
    void EventSender::NAME(const AZ::EntityId& graphUniqueId, const Data::TYPE##Type candidate, const Data::TYPE##Type reference, const Report& report)\
    {\
        void(BusTraits::* f)(const Data::##TYPE##Type, const Data::##TYPE##Type, const Report&) = &BusTraits::##NAME;\
        Bus::Event(graphUniqueId, f, candidate, reference, report);\
    }

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_REFLECTION(NAME, TYPE, LOOK_UP, OPERATOR, PARAM2)\
    builder->Method(LOOK_UP, static_cast<void(*)(const AZ::EntityId&, const Data::##TYPE##Type, const Data::##TYPE##Type, const Report&)>(&EventSender::##NAME), { { {"", "", behaviorContext->MakeDefaultValue(UniqueId)}, {"Candidate", "left of " OPERATOR}, {"Reference", "right of " OPERATOR}, {"Report", "additional notes for the test report"} } }) ;\
    builder->Attribute(AZ::ScriptCanvasAttributes::HiddenParameterIndex, uniqueIdIndex);

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_EQUALITY_OVERLOAD_DECLARATIONS(NAME)\
    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_TYPES(SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_DECLARATION, NAME, unused, unused, unused)

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_EQUALITY_OVERLOAD_IMPLEMENTATIONS(NAME)\
    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_TYPES(SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_IMPLEMENTATION, NAME, unused, unused, unused)

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_EQUALITY_OVERLOAD_REFLECTIONS(NAME, LOOK_UP, OPERATOR)\
    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_TYPES(SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_REFLECTION, NAME, LOOK_UP, OPERATOR, unused)

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_COMPARE_OVERLOAD_DECLARATIONS(NAME)\
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_TYPES(SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_DECLARATION, NAME, unused, unused, unused)

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_COMPARE_OVERLOAD_IMPLEMENTATIONS(NAME)\
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_TYPES(SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_IMPLEMENTATION, NAME, unused, unused, unused)

#define SCRIPT_CANVAS_UNIT_TEST_SENDER_COMPARE_OVERLOAD_REFLECTIONS(NAME, LOOK_UP, OPERATOR)\
    SCRIPT_CANVAS_UNIT_TEST_COMPARE_TYPES(SCRIPT_CANVAS_UNIT_TEST_SENDER_OVERLOAD_REFLECTION, NAME, LOOK_UP, OPERATOR, unused)

#define SCRIPT_CANVAS_UNIT_TEST_LEGACY_NODE_IMPLEMENTATION(NAME, TYPE, PARAM0, PARAM1, PARAM2)\
    case Data::eType::##TYPE##:\
    {\
        void(ScriptCanvas::UnitTesting::BusTraits::* f)(const Data::##TYPE##Type, const Data::##TYPE##Type, const ScriptCanvas::UnitTesting::Report&) = &ScriptCanvas::UnitTesting::BusTraits::##NAME##;\
        ScriptCanvas::UnitTesting::Bus::Event(GetOwningScriptCanvasId(), f, *lhs->GetAs<Data::##TYPE##Type>(), *rhs->GetAs<Data::##TYPE##Type>(), *report);\
    }\
    break;

#define SCRIPT_CANVAS_UNIT_TEST_LEGACY_NODE_EQUALITY_IMPLEMENTATIONS(NAME) case Data::eType::Number: break; default: break;

#define SCRIPT_CANVAS_UNIT_TEST_LEGACY_NODE_COMPARE_IMPLEMENTATIONS(NAME) case Data::eType::Number: break; default: break;
