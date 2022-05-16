/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "UnitTesting.h"

#include "UnitTestBus.h"

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    class ExecutionState;

    namespace UnitTesting
    {
        class EventSender
        {
        public:
            AZ_TYPE_INFO(EventSender, "{B7463F12-C981-4A0B-ACEF-4B26D431D797}");

            static void Reflect(AZ::ReflectContext* reflectContext);

            static void AddFailure(const ExecutionState* executionState, const Report& report);

            static void AddSuccess(const ExecutionState* executionState, const Report& report);

            static void Checkpoint(const ExecutionState* executionState, const Report& report);

            static void ExpectFalse(const ExecutionState* executionState, const bool value, const Report& report);

            static void ExpectTrue(const ExecutionState* executionState, const bool value, const Report& report);

            static void MarkComplete(const ExecutionState* executionState, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::AABBType candidate, const Data::AABBType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::BooleanType candidate, const Data::BooleanType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::ColorType candidate, const Data::ColorType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::CRCType candidate, const Data::CRCType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::EntityIDType candidate, const Data::EntityIDType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::Matrix3x3Type candidate, const Data::Matrix3x3Type reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::Matrix4x4Type candidate, const Data::Matrix4x4Type reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::OBBType candidate, const Data::OBBType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::PlaneType candidate, const Data::PlaneType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::QuaternionType candidate, const Data::QuaternionType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::TransformType candidate, const Data::TransformType reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report);

            static void ExpectEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::AABBType candidate, const Data::AABBType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::BooleanType candidate, const Data::BooleanType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::ColorType candidate, const Data::ColorType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::CRCType candidate, const Data::CRCType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::EntityIDType candidate, const Data::EntityIDType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::Matrix3x3Type candidate, const Data::Matrix3x3Type reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::Matrix4x4Type candidate, const Data::Matrix4x4Type reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::OBBType candidate, const Data::OBBType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::PlaneType candidate, const Data::PlaneType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::QuaternionType candidate, const Data::QuaternionType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::TransformType candidate, const Data::TransformType reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report);

            static void ExpectNotEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report);

            static void ExpectGreaterThan(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report);

            static void ExpectGreaterThan(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report);

            static void ExpectGreaterThan(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report);

            static void ExpectGreaterThan(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report);

            static void ExpectGreaterThan(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report);

            static void ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report);

            static void ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report);

            static void ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report);

            static void ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report);

            static void ExpectGreaterThanEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report);

            static void ExpectLessThan(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report);

            static void ExpectLessThan(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report);

            static void ExpectLessThan(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report);

            static void ExpectLessThan(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report);

            static void ExpectLessThan(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report);

            static void ExpectLessThanEqual(const ExecutionState* executionState, const Data::NumberType candidate, const Data::NumberType reference, const Report& report);

            static void ExpectLessThanEqual(const ExecutionState* executionState, const Data::StringType candidate, const Data::StringType reference, const Report& report);

            static void ExpectLessThanEqual(const ExecutionState* executionState, const Data::Vector2Type candidate, const Data::Vector2Type reference, const Report& report);

            static void ExpectLessThanEqual(const ExecutionState* executionState, const Data::Vector3Type candidate, const Data::Vector3Type reference, const Report& report);

            static void ExpectLessThanEqual(const ExecutionState* executionState, const Data::Vector4Type candidate, const Data::Vector4Type reference, const Report& report);
        };
        
    }
} 
