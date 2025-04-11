/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/MockComponentApplication.h>

#include <Data/DataType.h>
#include <Data/DataTypeUtils.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Data;

    using ScriptCanvasUnitTestDataTypeUtils = ScriptCanvasUnitTestFixture;

    static constexpr const char TestClassUuid[] = "{EAC960DB-0D94-4FA8-96CB-728F19E30E21}";
    class TestClass
    {
    public:
        AZ_RTTI(TestClass, TestClassUuid);
        AZ_CLASS_ALLOCATOR(TestClass, AZ::SystemAllocator);

        TestClass() = default;
        virtual ~TestClass() = default;
    };

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, FromAZType_GetExpectedType_WhileProvidingDifferentTypes)
    {
        auto expectedType = Type::AABB();
        auto actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Invalid();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::AssetId();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Boolean();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Color();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::CRC();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::EntityID();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::NamedEntityID();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Matrix3x3();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Matrix4x4();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::MatrixMxN();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Number();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::OBB();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Plane();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Quaternion();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::String();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Transform();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Vector2();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Vector3();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::Vector4();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::VectorN();
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);

        expectedType = Type::BehaviorContextObject(AZ::Uuid::CreateRandom());
        actualType = FromAZType(ToAZType(expectedType));
        EXPECT_EQ(actualType, expectedType);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, FromAZTypeChecked_GetInvalidType_WhileProvidingUnsupportedBehaviorContextType)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> componentApplicationMock =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        AZ::BehaviorContext testBehaviorContext;
        ON_CALL(*componentApplicationMock, GetBehaviorContext())
            .WillByDefault(
                [&testBehaviorContext]()
                {
                    return &testBehaviorContext;
                });

        auto testUuid = AZ::Uuid::CreateRandom();
        auto actualType = FromAZTypeChecked(testUuid);
        EXPECT_EQ(actualType, Type::Invalid());

        componentApplicationMock.reset();
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, FromAZTypeChecked_GetExpectedType_WhileProvidingSupportedBehaviorContextType)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> componentApplicationMock =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        AZ::BehaviorContext testBehaviorContext;
        testBehaviorContext.Class<TestClass>();
        ON_CALL(*componentApplicationMock, GetBehaviorContext())
            .WillByDefault(
                [&testBehaviorContext]()
                {
                    return &testBehaviorContext;
                });

        auto actualType = FromAZTypeChecked(AZ::Uuid(TestClassUuid));
        EXPECT_EQ(actualType, Type::BehaviorContextObject(AZ::Uuid(TestClassUuid)));

        componentApplicationMock.reset();
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsAABB_GetExpectedResult_WhileCheckingAABBType)
    {
        auto actualResult = IsAABB(AZ::Aabb::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsAABB(Type::AABB());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsAssetId_GetExpectedResult_WhileCheckingAssetIdType)
    {
        auto actualResult = IsAssetId(AZ::Data::AssetId::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsAssetId(Type::AssetId());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsBoolean_GetExpectedResult_WhileCheckingBooleanType)
    {
        auto actualResult = IsBoolean(ToAZType(Type::Boolean()));
        EXPECT_TRUE(actualResult);

        actualResult = IsBoolean(Type::Boolean());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsColor_GetExpectedResult_WhileCheckingColorType)
    {
        auto actualResult = IsColor(AZ::Color::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsColor(Type::Color());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsCRC_GetExpectedResult_WhileCheckingCRCType)
    {
        auto actualResult = IsCRC(ToAZType(Type::CRC()));
        EXPECT_TRUE(actualResult);

        actualResult = IsCRC(Type::CRC());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsEntityId_GetExpectedResult_WhileCheckingEntityIdType)
    {
        auto actualResult = IsEntityID(AZ::EntityId::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsEntityID(Type::EntityID());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsNamedEntityID_GetExpectedResult_WhileCheckingNamedEntityIDType)
    {
        auto actualResult = IsNamedEntityID(AZ::NamedEntityId::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsNamedEntityID(Type::NamedEntityID());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsNumber_GetExpectedResult_WhileCheckingNumberType)
    {
        auto actualResult = IsNumber(ToAZType(Type::Number()));
        EXPECT_TRUE(actualResult);

        actualResult = IsNumber(Type::Number());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsMatrix3x3_GetExpectedResult_WhileCheckingMatrix3x3Type)
    {
        auto actualResult = IsMatrix3x3(AZ::Matrix3x3::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsMatrix3x3(Type::Matrix3x3());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsMatrix4x4_GetExpectedResult_WhileCheckingMatrix4x4Type)
    {
        auto actualResult = IsMatrix4x4(AZ::Matrix4x4::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsMatrix4x4(Type::Matrix4x4());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsMatrixMxN_GetExpectedResult_WhileCheckingMatrixMxNType)
    {
        auto actualResult = IsMatrixMxN(AZ::MatrixMxN::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsMatrixMxN(Type::MatrixMxN());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsOBB_GetExpectedResult_WhileCheckingOBBType)
    {
        auto actualResult = IsOBB(AZ::Obb::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsOBB(Type::OBB());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsPlane_GetExpectedResult_WhileCheckingPlaneType)
    {
        auto actualResult = IsPlane(AZ::Plane::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsPlane(Type::Plane());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsQuaternion_GetExpectedResult_WhileCheckingQuaternionType)
    {
        auto actualResult = IsQuaternion(AZ::Quaternion::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsQuaternion(Type::Quaternion());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsString_GetExpectedResult_WhileCheckingStringType)
    {
        auto actualResult = IsString(ToAZType(Type::String()));
        EXPECT_TRUE(actualResult);

        actualResult = IsString(Type::String());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsTransform_GetExpectedResult_WhileCheckingTransformType)
    {
        auto actualResult = IsTransform(AZ::Transform::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsTransform(Type::Transform());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsVector2_GetExpectedResult_WhileCheckingVector2Type)
    {
        auto actualResult = IsVector2(AZ::Vector2::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsVector2(Type::Vector2());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsVector3_GetExpectedResult_WhileCheckingVector3Type)
    {
        auto actualResult = IsVector3(AZ::Vector3::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsVector3(Type::Vector3());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsVector4_GetExpectedResult_WhileCheckingVector4Type)
    {
        auto actualResult = IsVector4(AZ::Vector4::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsVector4(Type::Vector4());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsVectorN_GetExpectedResult_WhileCheckingVectorNType)
    {
        auto actualResult = IsVectorN(AZ::VectorN::TYPEINFO_Uuid());
        EXPECT_TRUE(actualResult);

        actualResult = IsVectorN(Type::VectorN());
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsVectorType_GetExpectedResult_WhileCheckingVectorType)
    {
        auto result = IsVectorType(AZ::Vector2::TYPEINFO_Uuid());
        EXPECT_TRUE(result);
        result = IsVectorType(Type::Vector2());
        EXPECT_TRUE(result);

        result = IsVectorType(AZ::Vector3::TYPEINFO_Uuid());
        EXPECT_TRUE(result);
        result = IsVectorType(Type::Vector3());
        EXPECT_TRUE(result);

        result = IsVectorType(AZ::Vector4::TYPEINFO_Uuid());
        EXPECT_TRUE(result);
        result = IsVectorType(Type::Vector4());
        EXPECT_TRUE(result);

        result = IsVectorType(AZ::VectorN::TYPEINFO_Uuid());
        EXPECT_TRUE(result);
        result = IsVectorType(Type::VectorN());
        EXPECT_TRUE(result);

        result = IsVectorType(AZ::Transform::TYPEINFO_Uuid());
        EXPECT_FALSE(result);
        result = IsVectorType(Type::Transform());
        EXPECT_FALSE(result);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsAutoBoxedType_GetExpectedResult_WhileCheckingAutoBoxedType)
    {
        auto result = IsAutoBoxedType(Type::AABB());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Color());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::CRC());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Matrix3x3());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Matrix4x4());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::OBB());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Quaternion());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Transform());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Vector2());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Vector3());
        EXPECT_TRUE(result);
        result = IsAutoBoxedType(Type::Vector4());
        EXPECT_TRUE(result);

        result = IsAutoBoxedType(Type::String());
        EXPECT_FALSE(result);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsValueType_GetExpectedResult_WhileCheckingValueType)
    {
        auto result = IsValueType(Type::AABB());
        EXPECT_TRUE(result);
        result = IsValueType(Type::CRC());
        EXPECT_TRUE(result);
        result = IsValueType(Type::Vector2());
        EXPECT_TRUE(result);
        result = IsValueType(Type::Number());
        EXPECT_TRUE(result);
        result = IsValueType(Type::String());
        EXPECT_TRUE(result);

        result = IsValueType(Type::BehaviorContextObject(AZ::Uuid::CreateRandom()));
        EXPECT_FALSE(result);
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsContainerType_GetExpectedResult_WhileCheckingVectorType)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> componentApplicationMock =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        AZ::SerializeContext testSerializeContext;
        AZ::GenericClassInfo* genericInfo = AZ::SerializeGenericTypeInfo<AZStd::vector<AZ::u32>>::GetGenericInfo();
        if (genericInfo)
        {
            genericInfo->Reflect(&testSerializeContext);
        }
        ON_CALL(*componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });

        auto containerUuid = AZ::SerializeGenericTypeInfo<AZStd::vector<AZ::u32>>::GetClassTypeId();
        auto result = IsContainerType(containerUuid);
        EXPECT_TRUE(result);
        result = IsContainerType(FromAZType(containerUuid));
        EXPECT_TRUE(result);
        auto nonContainerUuid = ToAZType(Type::Boolean());
        result = IsContainerType(nonContainerUuid);
        EXPECT_FALSE(result);
        result = IsContainerType(nonContainerUuid);
        EXPECT_FALSE(result);

        componentApplicationMock.reset();
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsMapContainerType_GetExpectedResult_WhileCheckingMapType)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> componentApplicationMock =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        AZ::SerializeContext testSerializeContext;
        AZ::GenericClassInfo* genericInfo = AZ::SerializeGenericTypeInfo<AZStd::map<AZ::u32, AZ::u32>>::GetGenericInfo();
        if (genericInfo)
        {
            genericInfo->Reflect(&testSerializeContext);
        }
        ON_CALL(*componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });

        auto mapUuid = AZ::SerializeGenericTypeInfo<AZStd::map<AZ::u32, AZ::u32>>::GetClassTypeId();
        auto result = IsMapContainerType(mapUuid);
        EXPECT_TRUE(result);
        result = IsMapContainerType(FromAZType(mapUuid));
        EXPECT_TRUE(result);
        auto nonMapUuid = ToAZType(Type::Boolean());
        result = IsMapContainerType(nonMapUuid);
        EXPECT_FALSE(result);
        result = IsMapContainerType(nonMapUuid);
        EXPECT_FALSE(result);

        componentApplicationMock.reset();
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsOutcomeType_GetExpectedResult_WhileCheckingOutcomeType)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> componentApplicationMock =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        AZ::SerializeContext testSerializeContext;
        AZ::GenericClassInfo* genericInfo = AZ::SerializeGenericTypeInfo<AZ::Outcome<bool, bool>>::GetGenericInfo();
        if (genericInfo)
        {
            genericInfo->Reflect(&testSerializeContext);
        }
        ON_CALL(*componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });

        auto outcomeUuid = AZ::SerializeGenericTypeInfo<AZ::Outcome<bool, bool>>::GetClassTypeId();
        auto result = IsOutcomeType(outcomeUuid);
        EXPECT_TRUE(result);
        result = IsOutcomeType(FromAZType(outcomeUuid));
        EXPECT_TRUE(result);
        auto nonOutcomeUuid = ToAZType(Type::Boolean());
        result = IsOutcomeType(nonOutcomeUuid);
        EXPECT_FALSE(result);
        result = IsOutcomeType(nonOutcomeUuid);
        EXPECT_FALSE(result);

        componentApplicationMock.reset();
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsSetContainerType_GetExpectedResult_WhileCheckingSetType)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> componentApplicationMock =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        AZ::SerializeContext testSerializeContext;
        AZ::GenericClassInfo* genericInfo = AZ::SerializeGenericTypeInfo<AZStd::set<AZ::u32>>::GetGenericInfo();
        if (genericInfo)
        {
            genericInfo->Reflect(&testSerializeContext);
        }
        ON_CALL(*componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });

        auto setUuid = AZ::SerializeGenericTypeInfo<AZStd::set<AZ::u32>>::GetClassTypeId();
        auto result = IsSetContainerType(setUuid);
        EXPECT_TRUE(result);
        result = IsSetContainerType(FromAZType(setUuid));
        EXPECT_TRUE(result);
        auto nonSetUuid = ToAZType(Type::Boolean());
        result = IsSetContainerType(nonSetUuid);
        EXPECT_FALSE(result);
        result = IsSetContainerType(nonSetUuid);
        EXPECT_FALSE(result);

        componentApplicationMock.reset();
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, IsVectorContainerType_GetExpectedResult_WhileCheckingArrayType)
    {
        AZStd::unique_ptr<testing::NiceMock<UnitTest::MockComponentApplication>> componentApplicationMock =
            AZStd::make_unique<testing::NiceMock<UnitTest::MockComponentApplication>>();
        AZ::SerializeContext testSerializeContext;
        AZ::GenericClassInfo* genericInfo = AZ::SerializeGenericTypeInfo<AZStd::array<AZ::u32, 1>>::GetGenericInfo();
        if (genericInfo)
        {
            genericInfo->Reflect(&testSerializeContext);
        }
        ON_CALL(*componentApplicationMock, GetSerializeContext())
            .WillByDefault(
                [&testSerializeContext]()
                {
                    return &testSerializeContext;
                });

        auto vectorUuid = AZ::SerializeGenericTypeInfo<AZStd::array<AZ::u32, 1>>::GetClassTypeId();
        auto result = IsVectorContainerType(vectorUuid);
        EXPECT_TRUE(result);
        result = IsVectorContainerType(FromAZType(vectorUuid));
        EXPECT_TRUE(result);
        auto nonVectorUuid = ToAZType(Type::Boolean());
        result = IsVectorContainerType(nonVectorUuid);
        EXPECT_FALSE(result);
        result = IsVectorContainerType(nonVectorUuid);
        EXPECT_FALSE(result);

        componentApplicationMock.reset();
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, ToAZType_GetNullUuid_WhileCheckingInvalidType)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto actualResult = ToAZType(eType::Count);
        AZ_TEST_STOP_TRACE_SUPPRESSION_NO_COUNT;
        EXPECT_TRUE(actualResult.IsNull());
    }

    TEST_F(ScriptCanvasUnitTestDataTypeUtils, ToAZType_GetExpectedResult_WhileCheckingBehaviorContextObjectType)
    {
        AZ::Uuid testUuid = AZ::Uuid::CreateRandom();
        auto actualResult = ToAZType(Type::BehaviorContextObject(testUuid));
        EXPECT_EQ(actualResult, testUuid);
    }
} // namespace ScriptCanvasUnitTest
