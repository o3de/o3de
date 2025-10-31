/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Data/DataType.h>
#include <Data/DataTypeUtils.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Data;

    using ScriptCanvasUnitTestDataType = ScriptCanvasUnitTestFixture;

    TEST_F(ScriptCanvasUnitTestDataType, AABB_GetExpectedType_WhileCreatingAABBType)
    {
        auto testType = Type::AABB();
        auto testOtherType = Type::AABB();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::AABB);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, AssetId_GetExpectedType_WhileCreatingAssetIdType)
    {
        auto testType = Type::AssetId();
        auto testOtherType = Type::AssetId();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::AssetId);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, BehaviorContextObject_GetExpectedType_WhileCreatingBehaviorContextObjectType)
    {
        AZ::Uuid testUuid = AZ::Uuid::CreateRandom();
        auto testType = Type::BehaviorContextObject(testUuid);
        auto testOtherType = Type::BehaviorContextObject(testUuid);
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), testUuid);
        EXPECT_EQ(testType.GetType(), eType::BehaviorContextObject);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(testUuid));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(testUuid));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Boolean_GetExpectedType_WhileCreatingBooleanType)
    {
        auto testType = Type::Boolean();
        auto testOtherType = Type::Boolean();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Boolean);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Number();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Color_GetExpectedType_WhileCreatingColorType)
    {
        auto testType = Type::Color();
        auto testOtherType = Type::Color();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Color);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Vector3();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Vector4();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, CRC_GetExpectedType_WhileCreatingCRCType)
    {
        auto testType = Type::CRC();
        auto testOtherType = Type::CRC();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::CRC);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, EntityID_GetExpectedType_WhileCreatingEntityIDType)
    {
        auto testType = Type::EntityID();
        auto testOtherType = Type::EntityID();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::EntityID);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, NamedEntityID_GetExpectedType_WhileCreatingNamedEntityIDType)
    {
        auto testType = Type::NamedEntityID();
        auto testOtherType = Type::NamedEntityID();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::NamedEntityID);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Invalid_GetExpectedType_WhileCreatingInvalidType)
    {
        auto testType = Type::Invalid();
        auto testOtherType = Type::Invalid();
        EXPECT_FALSE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Invalid);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_FALSE(testType);
        EXPECT_TRUE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Matrix3x3_GetExpectedType_WhileCreatingMatrix3x3Type)
    {
        auto testType = Type::Matrix3x3();
        auto testOtherType = Type::Matrix3x3();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Matrix3x3);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Quaternion();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Matrix4x4_GetExpectedType_WhileCreatingMatrix4x4Type)
    {
        auto testType = Type::Matrix4x4();
        auto testOtherType = Type::Matrix4x4();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Matrix4x4);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Transform();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Quaternion();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Number_GetExpectedType_WhileCreatingNumberType)
    {
        auto testType = Type::Number();
        auto testOtherType = Type::Number();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Number);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Boolean();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, OBB_GetExpectedType_WhileCreatingOBBType)
    {
        auto testType = Type::OBB();
        auto testOtherType = Type::OBB();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::OBB);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Plane_GetExpectedType_WhileCreatingPlaneType)
    {
        auto testType = Type::Plane();
        auto testOtherType = Type::Plane();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Plane);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Quaternion_GetExpectedType_WhileCreatingQuaternionType)
    {
        auto testType = Type::Quaternion();
        auto testOtherType = Type::Quaternion();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Quaternion);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Matrix3x3();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Matrix4x4();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Transform();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, String_GetExpectedType_WhileCreatingStringType)
    {
        auto testType = Type::String();
        auto testOtherType = Type::String();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::String);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(testType.IsConvertibleTo(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_TRUE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Transform_GetExpectedType_WhileCreatingTransformType)
    {
        auto testType = Type::Transform();
        auto testOtherType = Type::Transform();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Transform);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(testType.IsConvertibleTo(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_FALSE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_FALSE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Matrix4x4();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Vector2_GetExpectedType_WhileCreatingVector2Type)
    {
        auto testType = Type::Vector2();
        auto testOtherType = Type::Vector2();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Vector2);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(testType.IsConvertibleTo(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_TRUE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Vector3();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Vector4();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Vector3_GetExpectedType_WhileCreatingVector3Type)
    {
        auto testType = Type::Vector3();
        auto testOtherType = Type::Vector3();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Vector3);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(testType.IsConvertibleTo(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_TRUE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Vector2();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Vector4();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Color();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }

    TEST_F(ScriptCanvasUnitTestDataType, Vector4_GetExpectedType_WhileCreatingVector4Type)
    {
        auto testType = Type::Vector4();
        auto testOtherType = Type::Vector4();
        EXPECT_TRUE(testType.IsValid());
        EXPECT_EQ(testType.GetAZType(), ToAZType(testType));
        EXPECT_EQ(testType.GetType(), eType::Vector4);
        EXPECT_TRUE(testType.IS_A(testOtherType));
        EXPECT_TRUE(testType.IS_EXACTLY_A(testOtherType));
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(testType.IsConvertibleTo(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testOtherType)));
        EXPECT_TRUE(testType.IsConvertibleFrom(testOtherType));
        EXPECT_TRUE(testType.IsConvertibleFrom(ToAZType(testOtherType)));
        auto testConvertibleType = Type::Vector2();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Vector3();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::Color();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        testConvertibleType = Type::String();
        EXPECT_TRUE(testType.IsConvertibleTo(testConvertibleType));
        EXPECT_TRUE(testType.IsConvertibleTo(ToAZType(testConvertibleType)));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(testType));
        EXPECT_TRUE(testConvertibleType.IsConvertibleFrom(ToAZType(testType)));
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(testType);
        EXPECT_FALSE(!testType);
        EXPECT_TRUE(testType == testOtherType);
        EXPECT_FALSE(testType != testOtherType);
    }
} // namespace ScriptCanvasUnitTest
