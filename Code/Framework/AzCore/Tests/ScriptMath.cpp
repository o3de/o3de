/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Aabb.h>

#include <Math/MathTest.h>

using namespace AZ;

namespace UnitTest
{
    // Implemented in script.cpp
    void AZTestAssert(bool check);

    void AZTestAssertFloatClose(float v1, float v2)
    {
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1, v2);
    }

    static void Crc32PropertySet(Crc32 crc)
    {
        (void)crc;
        AZ_TracePrintf("Aha", "%d\n", static_cast<u32>(crc));
    }

    static Crc32 Crc32PropertyGet()
    {
        return Crc32("Test");
    }

    class ScriptMathTest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            behavior = aznew BehaviorContext();
            behavior->Method("AZTestAssert", &AZTestAssert);
            behavior->Method("AZTestAssertFloatClose", &AZTestAssertFloatClose);
            MathReflect(behavior);

            behavior->Property("globalCrc32Prop", &Crc32PropertyGet, &Crc32PropertySet);

            script = aznew ScriptContext();
            script->BindTo(behavior);
        }

        void TearDown() override
        {
            delete script;
            delete behavior;

            LeakDetectionFixture::TearDown();
        }

        BehaviorContext* behavior = nullptr;
        ScriptContext* script = nullptr;
    };

    TEST_F(ScriptMathTest, LuaVector2Test)
    {
        ////Constructors
        script->Execute("vA = Vector2(0)");
        script->Execute("AZTestAssert((vA.x == 0) and (vA.y == 0))");
        script->Execute("vB = Vector2(5)");
        script->Execute("AZTestAssert((vB.x == 5) and (vB.y == 5))");
        script->Execute("vC = Vector2(1, 2)");
        script->Execute("AZTestAssert((vC.x == 1) and (vC.y == 2))");

        ////Create functions
        script->Execute("AZTestAssert(Vector2.CreateOne() == Vector2(1, 1))");
        script->Execute("AZTestAssert(Vector2.CreateZero() == Vector2(0, 0))");
        script->Execute("AZTestAssert(Vector2.CreateAxisX() == Vector2(1, 0))");
        script->Execute("AZTestAssert(Vector2.CreateAxisY() == Vector2(0, 1))");
        script->Execute("AZTestAssert(Vector2.CreateFromAngle() == Vector2(0, 1))");
        script->Execute("fromAngle4 = Vector2.CreateFromAngle(4.0)");
        script->Execute("AZTestAssert(fromAngle4:IsClose(Vector2(-0.7568024953, -0.6536436208), 0.01))");

        //// Create - Comparison functions
        script->Execute("vA:Set(-100, 10)");
        script->Execute("vB:Set(35, 10)");
        script->Execute("vC:Set(35, 20)");
        script->Execute("vD = Vector2(15, 30)");

        //// Compare equal
        //// operation r.x = (cmp1.x == cmp2.x) ? vA:x : vB:x per component
        script->Execute("compareEqualAB = Vector2.CreateSelectCmpEqual(vA, vB, Vector2(1), Vector2(0))");
        script->Execute("AZTestAssert(compareEqualAB:IsClose(Vector2(0, 1)))");
        script->Execute("compareEqualBC = Vector2.CreateSelectCmpEqual(vB, vC, Vector2(1), Vector2(0))");
        script->Execute("AZTestAssert(compareEqualBC:IsClose(Vector2(1, 0)))");

        //// Compare greater equal
        //// operation ( r.x = (cmp1.x >= cmp2.x) ? vA:x : vB:x ) per component
        script->Execute("compareGreaterEqualAB = Vector2.CreateSelectCmpGreaterEqual(vA, vB, Vector2(1), Vector2(0))");
        script->Execute("AZTestAssert(compareGreaterEqualAB:IsClose(Vector2(0, 1)))");
        script->Execute("compareGreaterEqualBD = Vector2.CreateSelectCmpGreaterEqual(vB, vD, Vector2(1), Vector2(0))");
        script->Execute("AZTestAssert(compareGreaterEqualBD:IsClose(Vector2(1, 0)))");

        //// Compare greater
        /////operation ( r.x = (cmp1.x > cmp2.x) ? vA:x : vB:x ) per component
        script->Execute("compareGreaterAB = Vector2.CreateSelectCmpGreater(vA, vB, Vector2(1), Vector2(0))");
        script->Execute("AZTestAssert(compareGreaterAB:IsClose(Vector2(0, 0)))");
        script->Execute("compareGreaterCA = Vector2.CreateSelectCmpGreater(vC, vA, Vector2(1), Vector2(0))");
        script->Execute("AZTestAssert(compareGreaterCA:IsClose(Vector2(1, 1)))");

        ////Get/Set
        script->Execute("vA:Set(2, 3)");
        script->Execute("AZTestAssert(vA == Vector2(2, 3))");
        script->Execute("AZTestAssert(vA.x == 2 and vA.y == 3)");
        script->Execute("vA.x = 10");
        script->Execute("AZTestAssert(vA == Vector2(10, 3))");
        script->Execute("vA.y = 11");
        script->Execute("AZTestAssert(vA == Vector2(10, 11))");
        script->Execute("vA:Set(15)");
        script->Execute("AZTestAssert(vA == Vector2(15))");
        script->Execute("vA:SetElement(0, 20)");
        script->Execute("AZTestAssert(vA.x == 20 and vA.y == 15)");
        script->Execute("AZTestAssert(vA:GetElement(0) == 20 and vA:GetElement(1) == 15)");
        script->Execute("vA:SetElement(1, 21)");
        script->Execute("AZTestAssert(vA.x == 20 and vA.y == 21)");
        script->Execute("AZTestAssert(vA:GetElement(0) == 20 and vA:GetElement(1) == 21)");

        ////Length
        script->Execute("AZTestAssertFloatClose(Vector2(3, 4):GetLengthSq(), 25)");
        script->Execute("AZTestAssertFloatClose(Vector2(4, -3):GetLength(), 5)");

        ////Normalize
        script->Execute("AZTestAssert(Vector2(3, 4):GetNormalized():IsClose(Vector2(3 / 5, 4 / 5)))");

        script->Execute("vA:Set(4, 3)");
        script->Execute("vA:Normalize()");
        script->Execute("AZTestAssert(vA:IsClose(Vector2(4 / 5, 3 / 5)))");

        script->Execute("vA:Set(4, 3)");
        script->Execute("length = vA:NormalizeWithLength()");
        script->Execute("AZTestAssertFloatClose(length, 5)");
        script->Execute("AZTestAssert(vA:IsClose(Vector2(4 / 5, 3 / 5)))");

        script->Execute("vA:Set(4, 3)");
        script->Execute("vA:NormalizeSafe()");
        script->Execute("AZTestAssert(vA:IsClose(Vector2(4 / 5, 3 / 5)))");

        script->Execute("vA:Set(0)");
        script->Execute("vA:NormalizeSafe()");
        script->Execute("AZTestAssert(vA:IsClose(Vector2(0, 0)))");

        script->Execute("vA:Set(4, 3)");
        script->Execute("length = vA:NormalizeSafeWithLength()");
        script->Execute("AZTestAssertFloatClose(length, 5)");
        script->Execute("AZTestAssert(vA:IsClose(Vector2(4 / 5, 3 / 5)))");

        script->Execute("vA:Set(0)");
        script->Execute("length = vA:NormalizeSafeWithLength()");
        script->Execute("AZTestAssertFloatClose(length, 0)");
        script->Execute("AZTestAssert(vA:IsClose(Vector2(0, 0)))");

        script->Execute("AZTestAssert(Vector2(1, 0):IsNormalized())");
        script->Execute("AZTestAssert(Vector2(0.7071, 0.7071):IsNormalized())");
        script->Execute("AZTestAssert( not Vector2(1, 1):IsNormalized())");

        ////Set Length
        script->Execute("vA:Set(3, 4)");
        script->Execute("vA:SetLength(10)");
        script->Execute("AZTestAssert(vA:IsClose(Vector2(6, 8)))");

        ////Distance
        script->Execute("vA:Set(1, 2)");
        script->Execute("AZTestAssertFloatClose(vA:GetDistanceSq(Vector2(-2, 6)), 25)");
        script->Execute("AZTestAssertFloatClose(vA:GetDistance(Vector2(5, -1)), 5)");

        ////Lerp/slerp
        script->Execute("AZTestAssert(Vector2(3, 4):Lerp(Vector2(10, 2), 0.5) == Vector2(6.5, 3))");
        script->Execute("slerped = Vector2(1, 0):Slerp(Vector2(0, 1), 0.5)");
        script->Execute("AZTestAssert(slerped:IsClose(Vector2(0.7071, 0.7071)))");

        ////GetPerpendicular
        script->Execute("AZTestAssert(Vector2(1, 2):GetPerpendicular() == Vector2(-2, 1))");

        ////Close
        script->Execute("AZTestAssert(Vector2(1, 2):IsClose(Vector2(1, 2)))");
        script->Execute("AZTestAssert( not Vector2(1, 2):IsClose(Vector2(1, 3)))");
        ////Verify the closeness tolerance works
        script->Execute("AZTestAssert(Vector2(1, 2):IsClose(Vector2(1, 2.4), 0.5))");

        ////IsZero
        script->Execute("AZTestAssert(Vector2(0):IsZero())");
        script->Execute("AZTestAssert( not Vector2(1):IsZero())");
        ////Verify the IsZero tolerance works
        script->Execute("AZTestAssert(Vector2(0.05):IsZero(0.1))");

        ////Operator== and Operator!=
        script->Execute("vA:Set(1, 2)");
        script->Execute("AZTestAssert(vA == Vector2(1, 2))");
        script->Execute("AZTestAssert( not (vA == Vector2(5, 6)))");
        script->Execute("AZTestAssert(vA ~= Vector2(7, 8))");
        script->Execute("AZTestAssert( not (vA ~= Vector2(1, 2)))");

        ////Comparisons - returns true only if all the components pass
        ////IsLessThan
        script->Execute("AZTestAssert(Vector2(1, 2):IsLessThan(Vector2(2, 3)))");
        script->Execute("AZTestAssert( not Vector2(1, 2):IsLessThan(Vector2(0, 3)))");
        script->Execute("AZTestAssert( not Vector2(1, 2):IsLessThan(Vector2(2, 2)))");
        ////IsLessEqualThan
        script->Execute("AZTestAssert(Vector2(1, 2):IsLessEqualThan(Vector2(2, 3)))");
        script->Execute("AZTestAssert( not Vector2(1, 2):IsLessEqualThan(Vector2(0, 3)))");
        script->Execute("AZTestAssert(Vector2(1, 2):IsLessEqualThan(Vector2(2, 2)))");
        ////isGreaterThan
        script->Execute("AZTestAssert(Vector2(1, 2):IsGreaterThan(Vector2(0, 1)))");
        script->Execute("AZTestAssert( not Vector2(1, 2):IsGreaterThan(Vector2(0, 3)))");
        script->Execute("AZTestAssert( not Vector2(1, 2):IsGreaterThan(Vector2(0, 2)))");
        ////isGreaterEqualThan
        script->Execute("AZTestAssert(Vector2(1, 2):IsGreaterEqualThan(Vector2(0, 1)))");
        script->Execute("AZTestAssert( not Vector2(1, 2):IsGreaterEqualThan(Vector2(0, 3)))");
        script->Execute("AZTestAssert(Vector2(1, 2):IsGreaterEqualThan(Vector2(0, 2)))");

        ////min,max,clamp
        script->Execute("AZTestAssert(Vector2(2, 3):GetMin(Vector2(1, 4)) == Vector2(1, 3))");
        script->Execute("AZTestAssert(Vector2(2, 3):GetMax(Vector2(1, 4)) == Vector2(2, 4))");
        script->Execute("AZTestAssert(Vector2(1, 2):GetClamp(Vector2(5, -10), Vector2(10, -5)) == Vector2(5, -5))");
        script->Execute("AZTestAssert(Vector2(1, 2):GetClamp(Vector2(0, 0), Vector2(10, 10)) == Vector2(1, 2))");

        ////Select
        script->Execute("vA:Set(1)");
        script->Execute("vB:Set(2)");
        script->Execute("AZTestAssert(vA:GetSelect(Vector2(0, 1), vB) == Vector2(1, 2))");
        script->Execute("vA:Select(Vector2(1, 0), vB)");
        script->Execute("AZTestAssert(vA == Vector2(2, 1))");

        ////Abs
        script->Execute("AZTestAssert(Vector2(-1, 2):GetAbs() == Vector2(1, 2))");
        script->Execute("AZTestAssert(Vector2(3, -4):GetAbs() == Vector2(3, 4))");

        ////Operators
        script->Execute("AZTestAssert((-Vector2(1, -2)) == Vector2(-1, 2))");
        script->Execute("AZTestAssert((Vector2(1, 2) + Vector2(2, -1)) == Vector2(3, 1))");
        script->Execute("AZTestAssert((Vector2(1, 2) - Vector2(2, -1)) == Vector2(-1, 3))");
        script->Execute("AZTestAssert((Vector2(3, 2) * Vector2(2, -4)) == Vector2(6, -8))");
        script->Execute("AZTestAssert((Vector2(3, 2) * 2) == Vector2(6, 4))");
        script->Execute("AZTestAssert((Vector2(30, 20) / Vector2(10, -4)) == Vector2(3, -5))");
        script->Execute("AZTestAssert((Vector2(30, 20) / 10) == Vector2(3, 2))");

        ////Dot product
        script->Execute("AZTestAssertFloatClose(Vector2(1, 2):Dot(Vector2(-1, 5)), 9)");

        ////Madd : Multiply and add
        script->Execute("AZTestAssert(Vector2(1, 2):GetMadd(Vector2(2, 6), Vector2(3, 4)) == Vector2(5, 16))");
        script->Execute("vA:Set(1, 2)");
        script->Execute("vA:Madd(Vector2(2, 6), Vector2(3, 4))");
        script->Execute("AZTestAssert(vA == Vector2(5, 16))");

        ////Project
        script->Execute("vA:Set(0.5)");
        script->Execute("vA:Project(Vector2(0, 2))");
        script->Execute("AZTestAssert(vA == Vector2(0, 0.5))");
        script->Execute("vA:Set(0.5)");
        script->Execute("vA:ProjectOnNormal(Vector2(0, 1))");
        script->Execute("AZTestAssert(vA == Vector2(0, 0.5))");
        script->Execute("vA:Set(2, 4)");
        script->Execute("AZTestAssert(vA:GetProjected(Vector2(1, 1)) == Vector2(3, 3))");
        script->Execute("AZTestAssert(vA:GetProjectedOnNormal(Vector2(1, 0)) == Vector2(2, 0))");

        ////IsFinite
        script->Execute("AZTestAssert(Vector2(1, 1):IsFinite())");
        script->Execute("AZTestAssert( not Vector2(math.huge, math.huge):IsFinite())");
    }

    TEST_F(ScriptMathTest, LuaVector3Test)
    {
        // Not all Vector3 C++ functions are reflected into Lua:
        // Compound assignment operators are not supported in Lua (+=, -=, /=, *=)
        // Estimate functions are not specified, the performance benefits of estimates
        // are are dominated by marshalling between Lua and C++.

        // Constructors
        script->Execute("v1 = Vector3(0)  AZTestAssert(v1.x == 0 and v1.y == 0 and v1.x == 0)");
        script->Execute("v2 = Vector3(5)  AZTestAssert(v2.x == 5 and v2.y == 5 and v2.x == 5)");
        script->Execute("v3 = Vector3(1,2,3)  AZTestAssert(v3.x == 1 and v3.y == 2 and v3.z == 3)");

        // Create functions
        script->Execute("AZTestAssert(Vector3.CreateOne() == Vector3(1, 1, 1))");
        script->Execute("AZTestAssert(Vector3.CreateZero() == Vector3(0, 0, 0))");
        script->Execute("AZTestAssert(Vector3.CreateAxisX() == Vector3(1, 0, 0))");
        script->Execute("AZTestAssert(Vector3.CreateAxisY() == Vector3(0, 1, 0))");
        script->Execute("AZTestAssert(Vector3.CreateAxisZ() == Vector3(0, 0, 1))");

        // Set
        script->Execute("v1:Set(0)  AZTestAssert(v1.x == 0 and v1.y == 0 and v1.z == 0)");
        script->Execute("v1:Set(2,3,4)  AZTestAssert(v1.x == 2 and v1.y == 3 and v1.z == 4)");
        script->Execute("v1.x = 11  AZTestAssert(v1.x == 11)");
        script->Execute("v1.y = 12  AZTestAssert(v1.y == 12)");
        script->Execute("v1.z = 13  AZTestAssert(v1.z == 13)");

        // index element
        script->Execute("v1:Set(1,2,3)  AZTestAssert(v1:GetElement(0) == 1 and v1:GetElement(1) == 2 and v1:GetElement(2) == 3)");
        script->Execute("v1:SetElement(0,5) v1:SetElement(1,6) v1:SetElement(2,7) AZTestAssert(v1.x == 5 and v1.y == 6 and v1.z == 7)");

        //length
        script->Execute("AZTestAssertFloatClose(Vector3(3,4,0):GetLengthSq(),25)");
        script->Execute("AZTestAssertFloatClose(Vector3(3.0, 4.0, 0.0):GetLengthSq(), 25.0)");
        script->Execute("AZTestAssertFloatClose(Vector3(0.0, 4.0, -3.0):GetLength(), 5.0)");
        script->Execute("AZTestAssertFloatClose(Vector3(0.0, 4.0, -3.0):GetLengthReciprocal(), 0.2)");

        //normalize
        script->Execute("AZTestAssert(Vector3(3, 0, 4):GetNormalized():IsClose(Vector3(3 / 5, 0, 4 / 5)))");

        script->Execute("AZTestAssert(Vector3(3, 0, 4):GetNormalizedSafe():IsClose(Vector3(3 / 5, 0, 4 / 5)))");
        script->Execute("AZTestAssert(Vector3(0):GetNormalizedSafe() == Vector3(0, 0, 0))");

        script->Execute("v1:Set(4, 3, 0)");
        script->Execute("v1:Normalize()");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(4 / 5, 3 / 5, 0)))");

        script->Execute("v1:Set(4, 3, 0)");
        script->Execute("length = v1:NormalizeWithLength()");
        script->Execute("AZTestAssertFloatClose(length, 5)");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(4 / 5, 3 / 5, 0)))");

        script->Execute("v1:Set(0, 3, 4)");
        script->Execute("v1:NormalizeSafe()");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(0, 3 / 5, 4 / 5)))");
        script->Execute("v1 = Vector3(0)");
        script->Execute("v1:NormalizeSafe()");
        script->Execute("AZTestAssert(v1 == Vector3(0, 0, 0))");

        script->Execute("v1:Set(0, 3, 4)");
        script->Execute("length = v1:NormalizeSafeWithLength()");
        script->Execute("AZTestAssertFloatClose(length, 5)");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(0, 3 / 5, 4 / 5)))");
        script->Execute("v1 = Vector3(0)");
        script->Execute("length = v1:NormalizeSafeWithLength()");
        script->Execute("AZTestAssert(length == 0)");
        script->Execute("AZTestAssert(v1 == Vector3(0, 0, 0))");
        script->Execute("v1:Set(0, 3, 4)");

        script->Execute("AZTestAssert(Vector3(1, 0, 0):IsNormalized())");
        script->Execute("AZTestAssert(Vector3(0.7071, 0.7071, 0):IsNormalized())");
        script->Execute("AZTestAssert(not Vector3(1, 1, 0):IsNormalized())");

        ////setlength
        script->Execute("v1:Set(3, 4, 0)");
        script->Execute("v1:SetLength(10)");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(6, 8, 0)))");
        script->Execute("v1:Set(3, 4, 0)");
        script->Execute("v1:SetLengthEstimate(10)");


        #if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        #define LUA_VECTOR3_TEST_LENGTH_ESTIMATE_TOLERANCE "0.0195" // 10*2^(-9) ~= 0.0195 <- NEON Reciprocal ~ 9 bits
        #else
        #define LUA_VECTOR3_TEST_LENGTH_ESTIMATE_TOLERANCE "0.0049" // 10*2^(-11) ~= 0.0049 <- x86 Reciprocal ~ 11 bits
        #endif
        script->Execute("AZTestAssert(v1:IsClose(Vector3(6, 8, 0), " LUA_VECTOR3_TEST_LENGTH_ESTIMATE_TOLERANCE "))");

        ////distance
        script->Execute("v1:Set(1, 2, 3)");
        script->Execute("AZTestAssertFloatClose(v1:GetDistanceSq(Vector3(-2, 6, 3)), 25)");
        script->Execute("AZTestAssertFloatClose(v1:GetDistance(Vector3(-2, 2, -1)), 5)");

        ////lerp/slerp
        script->Execute("AZTestAssert(Vector3(3, 4, 5):Lerp(Vector3(10, 2, 1), 0.5) == Vector3(6.5, 3, 3))");
        script->Execute("AZTestAssert(Vector3(1, 0, 0):Slerp(Vector3(0, 1, 0), 0.5):IsClose(Vector3(0.7071, 0.7071, 0)))");

        ////dot product
        script->Execute("AZTestAssertFloatClose(Vector3(1, 2, 3):Dot(Vector3(-1, 5, 3)), 18)");

        ////cross products
        script->Execute("AZTestAssert(Vector3(1, 2, 3):Cross(Vector3(3, 1, -1)) == Vector3(-5, 10, -5))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):CrossXAxis() == Vector3(0, 3, -2))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):CrossYAxis() == Vector3(-3, 0, 1))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):CrossZAxis() == Vector3(2, -1, 0))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):XAxisCross() == Vector3(0, -3, 2))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):YAxisCross() == Vector3(3, 0, -1))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):ZAxisCross() == Vector3(-2, 1, 0))");

        ////close
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsClose(Vector3(1, 2, 3)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 3):IsClose(Vector3(1, 2, 4)))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsClose(Vector3(1, 2, 3.4), 0.5))");

        ////operator==
        script->Execute("v3:Set(1, 2, 3)");
        script->Execute("AZTestAssert(v3 == Vector3(1, 2, 3))");
        script->Execute("AZTestAssert( not (v3 == Vector3(1, 2, 4)))");
        script->Execute("AZTestAssert(v3 ~= Vector3(1, 2, 5))");
        script->Execute("AZTestAssert( not (v3 ~= Vector3(1, 2, 3)))");

        ////comparisons
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsLessThan(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 3):IsLessThan(Vector3(0, 3, 4)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 3):IsLessThan(Vector3(2, 2, 4)))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsLessEqualThan(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 3):IsLessEqualThan(Vector3(0, 3, 4)))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsLessEqualThan(Vector3(2, 2, 4)))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsGreaterThan(Vector3(0, 1, 2)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 3):IsGreaterThan(Vector3(0, 3, 2)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 3):IsGreaterThan(Vector3(0, 2, 2)))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsGreaterEqualThan(Vector3(0, 1, 2)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 3):IsGreaterEqualThan(Vector3(0, 3, 2)))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):IsGreaterEqualThan(Vector3(0, 2, 2)))");

        ////min,max,clamp
        script->Execute("AZTestAssert(Vector3(2, 5, 6):GetMin(Vector3(1, 6, 5)) == Vector3(1, 5, 5))");
        script->Execute("AZTestAssert(Vector3(2, 5, 6):GetMax(Vector3(1, 6, 5)) == Vector3(2, 6, 6))");
        script->Execute("AZTestAssert(Vector3(1, 2, 3):GetClamp(Vector3(0, -1, 4), Vector3(2, 1, 10)) == Vector3(1, 1, 4))");

        ////trig
        script->Execute("AZTestAssert(Vector3(Math.DegToRad(78), Math.DegToRad(-150), Math.DegToRad(190)):GetAngleMod():IsClose(Vector3(Math.DegToRad(78), Math.DegToRad(-150), Math.DegToRad(-170))))");
        script->Execute("AZTestAssert(Vector3(Math.DegToRad(390), Math.DegToRad(-190), Math.DegToRad(-400)):GetAngleMod():IsClose(Vector3(Math.DegToRad(30), Math.DegToRad(170), Math.DegToRad(-40))))");
        script->Execute("AZTestAssert(Vector3(Math.DegToRad(60), Math.DegToRad(105), Math.DegToRad(-174)):GetSin():IsClose(Vector3(0.866, 0.966, -0.105), 0.005))");
        script->Execute("AZTestAssert(Vector3(Math.DegToRad(60), Math.DegToRad(105), Math.DegToRad(-174)):GetCos():IsClose(Vector3(0.5, -0.259, -0.995), 0.005))");

        script->Execute("v1:Set(Math.DegToRad(60), Math.DegToRad(105), Math.DegToRad(-174))");
        script->Execute("sin, cos = v1:GetSinCos()");
        script->Execute("AZTestAssert(sin:IsClose(Vector3(0.866, 0.966, -0.105), 0.005))");
        script->Execute("AZTestAssert(cos:IsClose(Vector3(0.5, -0.259, -0.995), 0.005))");

        ////abs
        script->Execute("AZTestAssert(Vector3(-1, 2, -5):GetAbs() == Vector3(1, 2, 5))");

        ////reciprocal
        script->Execute("AZTestAssert(Vector3(2, 4, 5):GetReciprocal():IsClose(Vector3(0.5, 0.25, 0.2)))");

        ////operators
        script->Execute("AZTestAssert((-Vector3(1, 2, -3)) == Vector3(-1, -2, 3))");
        script->Execute("AZTestAssert((Vector3(1, 2, 3) + Vector3(-1, 4, 5)) == Vector3(0, 6, 8))");
        script->Execute("AZTestAssert((Vector3(1, 2, 3) - Vector3(-1, 4, 5)) == Vector3(2, -2, -2))");
        script->Execute("AZTestAssert((Vector3(1, 2, 3)*Vector3(-1, 4, 5)) == Vector3(-1, 8, 15))");
        script->Execute("AZTestAssert((Vector3(1, 2, 3) / Vector3(-1, 4, 5)):IsClose(Vector3(-1, 0.5, 3 / 5)))");
        script->Execute("AZTestAssert((Vector3(1, 2, 3)*2) == Vector3(2, 4, 6))");
        //TODO: float * Vector3 is not functional yet.
        //script->Execute("AZTestAssert((2*Vector3(1, 2, 3)) == Vector3(2, 4, 6))");
        script->Execute("AZTestAssert((Vector3(1, 2, 3) / 2):IsClose(Vector3(0.5, 1, 1.5)))");

        ////build tangent basis
        script->Execute("v1 = Vector3(1, 1, 2):GetNormalized()");
        script->Execute("v2, v3 = v1:BuildTangentBasis()");
        script->Execute("AZTestAssert(v2:IsNormalized())");
        script->Execute("AZTestAssert(v3:IsNormalized())");
        script->Execute("AZTestAssert(math.abs(v2:Dot(v1)) < 0.001)");
        script->Execute("AZTestAssert(math.abs(v3:Dot(v1)) < 0.001)");
        script->Execute("AZTestAssert(math.abs(v2:Dot(v3)) < 0.001)");

        ////madd
        script->Execute("AZTestAssert(Vector3(1, 2, 3):GetMadd(Vector3(2, 1, 4), Vector3(1, 2, 4)) == Vector3(3, 4, 16))");
        script->Execute("v1:Set(1, 2, 3)");
        script->Execute("v1:Madd(Vector3(2, 1, 4), Vector3(1, 2, 4))");
        script->Execute("AZTestAssert(v1 == Vector3(3, 4, 16))");

        ////isPerpendicular
        script->Execute("AZTestAssert(Vector3(1, 2, 0):IsPerpendicular(Vector3(0, 0, 1)))");
        script->Execute("AZTestAssert( not Vector3(1, 2, 0):IsPerpendicular(Vector3(0, 1, 1)))");
    }

    TEST_F(ScriptMathTest, LuaVector4Test)
    {
        //script->Execute("constructors
        script->Execute("v1 = Vector4(0)");
        script->Execute("AZTestAssert((v1.x == 0) and (v1.y == 0) and (v1.z == 0) and (v1.w == 0))");
        script->Execute("v2 = Vector4(5)");
        script->Execute("AZTestAssert((v2.x == 5) and (v2.y == 5) and (v2.z == 5) and (v2.w == 5))");
        script->Execute("v3 = Vector4(1, 2, 3, 4)");
        script->Execute("AZTestAssert((v3.x == 1) and (v3.y == 2) and (v3.z == 3) and (v3.w == 4))");
        script->Execute("v5 = Vector4.CreateFromVector3(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert((v5.x == 2) and (v5.y == 3) and (v5.z == 4) and (v5.w == 1))");
        script->Execute("v6 = Vector4.CreateFromVector3AndFloat(Vector3(2, 3, 4), 5)");
        script->Execute("AZTestAssert((v6.x == 2) and (v6.y == 3) and (v6.z == 4) and (v6.w == 5))");

        ////Create functions
        script->Execute("AZTestAssert(Vector4.CreateOne() == Vector4(1, 1, 1, 1))");
        script->Execute("AZTestAssert(Vector4.CreateZero() == Vector4(0))");
        script->Execute("AZTestAssert(Vector4.CreateAxisX() == Vector4(1, 0, 0, 0))");
        script->Execute("AZTestAssert(Vector4.CreateAxisY() == Vector4(0, 1, 0, 0))");
        script->Execute("AZTestAssert(Vector4.CreateAxisZ() == Vector4(0, 0, 1, 0))");
        script->Execute("AZTestAssert(Vector4.CreateAxisW() == Vector4(0, 0, 0, 1))");

        ////Get/Set
        script->Execute("v1:Set(2, 3, 4, 5)");
        script->Execute("AZTestAssert(v1 == Vector4(2, 3, 4, 5))");
        script->Execute("v1.x = 10");
        script->Execute("AZTestAssert(v1 == Vector4(10, 3, 4, 5))");
        script->Execute("v1.y = 11");
        script->Execute("AZTestAssert(v1 == Vector4(10, 11, 4, 5))");
        script->Execute("v1.z = 12");
        script->Execute("AZTestAssert(v1 == Vector4(10, 11, 12, 5))");
        script->Execute("v1.w = 13");
        script->Execute("AZTestAssert(v1 == Vector4(10, 11, 12, 13))");
        script->Execute("v1:Set(1)");
        script->Execute("AZTestAssert((v1.x == 1) and (v1.y == 1) and (v1.z == 1) and (v1.w == 1))");
        script->Execute("v1:Set(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert((v1.x == 2) and (v1.y == 3) and (v1.z == 4) and (v1.w == 1))");
        script->Execute("v1:Set(Vector3(2, 3, 4), 5)");
        script->Execute("AZTestAssert((v1.x == 2) and (v1.y == 3) and (v1.z == 4) and (v1.w == 5))");

        ////index operators
        script->Execute("v1:Set(1, 2, 3, 4)");
        script->Execute("AZTestAssert(v1:GetElement(0) == 1)");
        script->Execute("AZTestAssert(v1:GetElement(1) == 2)");
        script->Execute("AZTestAssert(v1:GetElement(2) == 3)");
        script->Execute("AZTestAssert(v1:GetElement(3) == 4)");
        script->Execute("v1:SetElement(0, 5)");
        script->Execute("v1:SetElement(1, 6)");
        script->Execute("v1:SetElement(2, 7)");
        script->Execute("v1:SetElement(3, 8)");
        script->Execute("AZTestAssert(v1:GetElement(0) == 5)");
        script->Execute("AZTestAssert(v1:GetElement(1) == 6)");
        script->Execute("AZTestAssert(v1:GetElement(2) == 7)");
        script->Execute("AZTestAssert(v1:GetElement(3) == 8)");

        ////operator==
        script->Execute("v3:Set(1, 2, 3, 4)");
        script->Execute("AZTestAssert(v3 == Vector4(1, 2, 3, 4.0))");
        script->Execute("AZTestAssert( not (v3 == Vector4(1, 2, 3, 5)))");
        script->Execute("AZTestAssert(v3 ~= Vector4(1, 2, 3, 5))");
        script->Execute("AZTestAssert( not (v3 ~= Vector4(1, 2, 3, 4)))");

        ////comparisons
        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsLessThan(Vector4(2, 3, 4, 5)))");
        script->Execute("AZTestAssert( not Vector4(1, 2, 3, 4):IsLessThan(Vector4(0, 3, 4, 5)))");
        script->Execute("AZTestAssert( not Vector4(1, 2, 3, 4):IsLessThan(Vector4(1, 2, 4, 5)))");
        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsLessEqualThan(Vector4(2, 3, 4, 5)))");
        script->Execute("AZTestAssert( not Vector4(1, 2, 3, 4):IsLessEqualThan(Vector4(0, 3, 4, 5)))");
        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsLessEqualThan(Vector4(2, 2, 4, 5)))");

        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsGreaterThan(Vector4(0, 1, 2, 3)))");
        script->Execute("AZTestAssert( not Vector4(1, 2, 3, 4):IsGreaterThan(Vector4(0, 3, 2, 3)))");
        script->Execute("AZTestAssert( not Vector4(1, 2, 3, 4):IsGreaterThan(Vector4(0, 2, 2, 3)))");
        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsGreaterEqualThan(Vector4(0, 1, 2, 3)))");
        script->Execute("AZTestAssert( not Vector4(1, 2, 3, 4):IsGreaterEqualThan(Vector4(0, 3, 2, 3)))");
        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsGreaterEqualThan(Vector4(0, 2, 2, 3)))");

        ////dot product
        script->Execute("AZTestAssertFloatClose(Vector4(1, 2, 3, 4):Dot(Vector4(-1, 5, 3, 2)), 26)");
        script->Execute("AZTestAssertFloatClose(Vector4(1, 2, 3, 4):Dot3(Vector3(-1, 5, 3)), 18)");

        ////close
        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsClose(Vector4(1, 2, 3, 4)))");
        script->Execute("AZTestAssert( not Vector4(1, 2, 3, 4):IsClose(Vector4(1, 2, 3, 5)))");
        script->Execute("AZTestAssert(Vector4(1, 2, 3, 4):IsClose(Vector4(1, 2, 3, 4.4), 0.5))");

        ////homogenize
        script->Execute("v1:Set(1, 2, 3, 4)");
        script->Execute("AZTestAssert(v1:GetHomogenized():IsClose(Vector3(0.25, 0.5, 0.75)))");
        script->Execute("v1:Homogenize()");
        script->Execute("AZTestAssert(v1:IsClose(Vector4(0.25, 0.5, 0.75, 1)))");

        ////abs
        script->Execute("AZTestAssert(Vector4(-1, 2, -5, 1):GetAbs() == Vector4(1, 2, 5, 1))");
        script->Execute("AZTestAssert(Vector4(1, -2, 5, -1):GetAbs() == Vector4(1, 2, 5, 1))");

        ////reciprocal
        script->Execute("AZTestAssert(Vector4(2, 4, 5, 10):GetReciprocal():IsClose(Vector4(0.5, 0.25, 0.2, 0.1)))");

        //// operators
        script->Execute("AZTestAssert((-Vector4(1, 2, -3, -1)) == Vector4(-1, -2, 3, 1))");
        script->Execute("AZTestAssert((Vector4(1, 2, 3, 4) + Vector4(-1, 4, 5, 2)) == Vector4(0, 6, 8, 6))");
        script->Execute("AZTestAssert((Vector4(1, 2, 3, 4) - Vector4(-1, 4, 5, 2)) == Vector4(2, -2, -2, 2))");
        script->Execute("AZTestAssert((Vector4(1, 2, 3, 4)*Vector4(-1, 4, 5, 2)) == Vector4(-1, 8, 15, 8))");
        script->Execute("AZTestAssert((Vector4(1, 2, 3, 4) / Vector4(-1, 4, 5, 2)):IsClose(Vector4(-1, 0.5, 3 / 5, 2)))");
        script->Execute("AZTestAssert((Vector4(1, 2, 3, 4)*2) == Vector4(2, 4, 6, 8))");
        //TODO float * Vector4 is not functional yet
        //script->Execute("AZTestAssert((2*Vector4(1, 2, 3, 4)) == Vector4(2, 4, 6, 8))");
        script->Execute("AZTestAssert((Vector4(1, 2, 3, 4) / 2):IsClose(Vector4(0.5, 1, 1.5, 2)))");
        script->Execute("v1:Set(1, 2, 3, 4)");
    }

    TEST_F(ScriptMathTest, LuaQuaternionTest)
    {
        //constructors
        script->Execute("q1 = Quaternion(0, 0, 0, 1)");
        script->Execute("AZTestAssert((q1.x == 0) and (q1.y == 0) and (q1.z == 0) and (q1.w == 1))");
        script->Execute("q2 = Quaternion(5)");
        script->Execute("AZTestAssert((q2.x == 5) and (q2.y == 5) and (q2.z == 5) and (q2.w == 5))");
        script->Execute("q3 = Quaternion(1, 2, 3, 4)");
        script->Execute("AZTestAssert((q3.x == 1) and (q3.y == 2) and (q3.z == 3) and (q3.w == 4))");
        script->Execute("q4 = Quaternion.CreateFromVector3AndValue(Vector3(1, 2, 3), 4)");
        script->Execute("AZTestAssert((q4.x == 1) and (q4.y == 2) and (q4.z == 3) and (q4.w == 4))");
        script->Execute("q7 = Quaternion.CreateFromMatrix3x3(Matrix3x3.CreateRotationX(Math.DegToRad(120)))");
        script->Execute("AZTestAssert(q7:IsClose(Quaternion(0.866, 0, 0, 0.5)))");
        script->Execute("q8 = Quaternion.CreateFromMatrix4x4(Matrix4x4.CreateRotationX(Math.DegToRad(-60)))");
        script->Execute("AZTestAssert(q8:IsClose(Quaternion(-0.5, 0, 0, 0.866)))");
        script->Execute("q9 = Quaternion.CreateFromVector3(Vector3(1, 2, 3))");
        script->Execute("AZTestAssert((q9.x == 1) and (q9.y == 2) and (q9.z == 3) and (q9.w == 0))");
        script->Execute("q10 = Quaternion.CreateFromAxisAngle(Vector3.CreateAxisZ(), Math.DegToRad(45))");
        script->Execute("AZTestAssert(q10:IsClose(Quaternion.CreateRotationZ(Math.DegToRad(45))))");

        //Create functions
        script->Execute("AZTestAssert(Quaternion.CreateIdentity() == Quaternion(0, 0, 0, 1))");
        script->Execute("AZTestAssert(Quaternion.CreateZero() == Quaternion(0))");
        script->Execute("AZTestAssert(Quaternion.CreateRotationX(Math.DegToRad(60)):IsClose(Quaternion(0.5, 0, 0, 0.866)))");
        script->Execute("AZTestAssert(Quaternion.CreateRotationY(Math.DegToRad(60)):IsClose(Quaternion(0, 0.5, 0, 0.866)))");
        script->Execute("AZTestAssert(Quaternion.CreateRotationZ(Math.DegToRad(60)):IsClose(Quaternion(0, 0, 0.5, 0.866)))");

        //test shortest arc
        script->Execute("v1 = Vector3(1, 2, 3):GetNormalized()");
        script->Execute("v2 = Vector3(-2, 7, -1):GetNormalized()");
        script->Execute("q3 = Quaternion.CreateShortestArc(v1, v2)");        //q3 should transform v1 into v2
        script->Execute("AZTestAssert(v2:IsClose(q3*v1, 0.001))");
        script->Execute("q4 = Quaternion.CreateShortestArc(Vector3(1, 0, 0), Vector3(0, 1, 0))");
        script->Execute("AZTestAssert((q4*Vector3(0, 0, 1)):IsClose(Vector3(0, 0, 1), 0.001))");    //perpendicular vector should be unaffected
        script->Execute("AZTestAssert((q4*Vector3(0, -1, 0)):IsClose(Vector3(1, 0, 0), 0.001))");   //make sure we rotate the right direction, i.e. actually shortest arc

        //Get/Set
        script->Execute("q1.x = 10");
        script->Execute("AZTestAssert(q1.x == 10)");
        script->Execute("q1.y = 11");
        script->Execute("AZTestAssert(q1.y == 11)");
        script->Execute("q1.z = 12");
        script->Execute("AZTestAssert(q1.z == 12)");
        script->Execute("q1.w = 13");
        script->Execute("AZTestAssert(q1.w == 13)");
        script->Execute("q1:Set(15)");
        script->Execute("AZTestAssert(q1 == Quaternion(15))");
        script->Execute("q1:Set(2, 3, 4, 5)");
        script->Execute("AZTestAssert(q1 == Quaternion(2, 3, 4, 5))");
        script->Execute("q1:Set(Vector3(5, 6, 7), 8)");
        script->Execute("AZTestAssert(q1 == Quaternion(5, 6, 7, 8))");

        //GetElement/SetElement
        script->Execute("q1:SetElement(0, 1)");
        script->Execute("q1:SetElement(1, 2)");
        script->Execute("q1:SetElement(2, 3)");
        script->Execute("q1:SetElement(3, 4)");
        script->Execute("AZTestAssert(q1:GetElement(0) == 1)");
        script->Execute("AZTestAssert(q1:GetElement(1) == 2)");
        script->Execute("AZTestAssert(q1:GetElement(2) == 3)");
        script->Execute("AZTestAssert(q1:GetElement(3) == 4)");

        //IsIdentity
        script->Execute("q1:Set(0, 0, 0, 1)");
        script->Execute("AZTestAssert(q1:IsIdentity())");

        //conjugate
        script->Execute("q1:Set(1, 2, 3, 4)");
        script->Execute("AZTestAssert(q1:GetConjugate() == Quaternion(-1, -2, -3, 4))");

        //inverse
        script->Execute("q1 = Quaternion.CreateRotationX(Math.DegToRad(25)) * Quaternion.CreateRotationY(Math.DegToRad(70))");
        script->Execute("AZTestAssert((q1*q1:GetInverseFast()):IsClose(Quaternion.CreateIdentity()))");
        script->Execute("q2 = q1:Clone()");
        script->Execute("q2:InvertFast()");
        script->Execute("AZTestAssert(q1.x == -q2.x)");
        script->Execute("AZTestAssert(q1.y == -q2.y)");
        script->Execute("AZTestAssert(q1.z == -q2.z)");
        script->Execute("AZTestAssert(q1.w == q2.w)");
        script->Execute("AZTestAssert((q1*q2):IsClose(Quaternion.CreateIdentity()))");

        script->Execute("q1:Set(1, 2, 3, 4)");
        script->Execute("AZTestAssert((q1*q1:GetInverseFull()):IsClose(Quaternion.CreateIdentity()))");

        //dot product
        script->Execute("AZTestAssertFloatClose(Quaternion(1, 2, 3, 4):Dot(Quaternion(-1, 5, 3, 2)), 26)");

        //length
        script->Execute("AZTestAssertFloatClose(Quaternion(-1, 2, 1, 3):GetLengthSq(), 15)");
        script->Execute("AZTestAssertFloatClose(Quaternion(-4, 2, 0, 4):GetLength(), 6)");

        //normalize
        // TODO This asserted for some reason
        script->Execute("AZTestAssert(Quaternion(0, -4, 2, 4):GetNormalized():IsClose(Quaternion(0, -0.6666, 0.3333, 0.6666)))");
        script->Execute("q1:Set(2, 0, 4, -4)");
        script->Execute("q1:Normalize()");
        script->Execute("AZTestAssert(q1:IsClose(Quaternion(0.3333, 0, 0.6666, -0.6666)))");
        script->Execute("q1:Set(2, 0, 4, -4)");
        script->Execute("length = q1:NormalizeWithLength()");
        script->Execute("AZTestAssertFloatClose(length, 6)");
        script->Execute("AZTestAssert(q1:IsClose(Quaternion(0.3333, 0, 0.6666, -0.6666)))");

        //interpolation
        script->Execute("AZTestAssert(Quaternion(1, 2, 3, 4):Lerp(Quaternion(2, 3, 4, 5), 0.5):IsClose(Quaternion(1.5, 2.5, 3.5, 4.5)))");
        script->Execute("AZTestAssert(Quaternion.CreateRotationX(Math.DegToRad(10)):Slerp(Quaternion.CreateRotationY(Math.DegToRad(60)), 0.5):IsClose(Quaternion(0.045, 0.259, 0, 0.965), 0.001))");
        script->Execute("AZTestAssert(Quaternion.CreateRotationX(Math.DegToRad(10)):Squad(Quaternion.CreateRotationY(Math.DegToRad(60)), Quaternion.CreateRotationZ(Math.DegToRad(35)), Quaternion.CreateRotationX(Math.DegToRad(80)), 0.5):IsClose(Quaternion(0.2, 0.132, 0.083, 0.967), 0.001))");

        //close
        script->Execute("AZTestAssert(Quaternion(1, 2, 3, 4):IsClose(Quaternion(1, 2, 3, 4)))");
        script->Execute("AZTestAssert( not Quaternion(1, 2, 3, 4):IsClose(Quaternion(1, 2, 3, 5)))");
        script->Execute("AZTestAssert(Quaternion(1, 2, 3, 4):IsClose(Quaternion(1, 2, 3, 4.4), 0.5))");

        //operators
        script->Execute("AZTestAssert((-Quaternion(1, 2, 3, 4)) == Quaternion(-1, -2, -3, -4))");
        script->Execute("AZTestAssert((Quaternion(1, 2, 3, 4) + Quaternion(2, 3, 5, -1)):IsClose(Quaternion(3, 5, 8, 3)))");
        script->Execute("AZTestAssert((Quaternion(1, 2, 3, 4) - Quaternion(2, 3, 5, -1)):IsClose(Quaternion(-1, -1, -2, 5)))");
        script->Execute("AZTestAssert((Quaternion(1, 2, 3, 4)*Quaternion(2, 3, 5, -1)):IsClose(Quaternion(8, 11, 16, -27)))");
        script->Execute("AZTestAssert((Quaternion(1, 2, 3, 4)*2):IsClose(Quaternion(2, 4, 6, 8)))");
        //TODO float * Quaternion is not functional yet
        //script->Execute("AZTestAssert((2*Quaternion(1, 2, 3, 4)):IsClose(Quaternion(2, 4, 6, 8)))");
        script->Execute("AZTestAssert((Quaternion(1, 2, 3, 4) / 2):IsClose(Quaternion(0.5, 1, 1.5, 2)))");

        //operator==
        script->Execute("q3:Set(1, 2, 3, 4)");
        script->Execute("AZTestAssert(q3 == Quaternion(1, 2, 3, 4.0))");
        script->Execute("AZTestAssert( not (q3 == Quaternion(1, 2, 3, 5)))");
        script->Execute("AZTestAssert(q3 ~= Quaternion(1, 2, 3, 5))");
        script->Execute("AZTestAssert( not (q3 ~= Quaternion(1, 2, 3, 4)))");

        //vector transform
        script->Execute("AZTestAssert((Quaternion.CreateRotationX(Math.DegToRad(45))*Vector3(4, 1, 0)):IsClose(Vector3(4, 0.7071, 0.7071)))");

        //GetImaginary
        script->Execute("q1:Set(21, 22, 23, 24)");
        script->Execute("AZTestAssert(q1:GetImaginary() == Vector3(21, 22, 23))");

        //GetAngle
        script->Execute("q1 = Quaternion.CreateRotationX(Math.DegToRad(35))");
        script->Execute("AZTestAssertFloatClose(q1:GetAngle(),Math.DegToRad(35))");

        //make sure that our transformations concatenate in the correct order
        script->Execute("q1 = Quaternion.CreateRotationZ(Math.DegToRad(90))");
        script->Execute("q2 = Quaternion.CreateRotationX(Math.DegToRad(90))");
        script->Execute("v = (q2 * q1) * Vector3(1, 0, 0)");
        script->Execute("AZTestAssert(v:IsClose(Vector3(0, 0, 1)))");
    }

    TEST_F(ScriptMathTest, LuaMatrix3x3Test)
    {
        script->Execute("testIndices = { 0, 1, 2, 3 }");

        //create functions
        script->Execute("m1 = Matrix3x3.CreateIdentity()");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector3(1, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector3(0, 1, 0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector3(0, 0, 1))");
        script->Execute("m1 = Matrix3x3.CreateZero()");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector3(0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector3(0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector3(0))");
        script->Execute("m1 = Matrix3x3.CreateFromValue(2)");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector3(2))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector3(2))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector3(2))");

        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(0, 0.866, -0.5)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(0, 0.5, 0.866)))");
        script->Execute("m1 = Matrix3x3.CreateRotationY(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(0.866, 0, 0.5)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(0, 1, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(-0.5, 0, 0.866)))");
        script->Execute("m1 = Matrix3x3.CreateRotationZ(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(0.866, -0.5, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(0.5, 0.866, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(0, 0, 1)))");
        script->Execute("m1 = Matrix3x3.CreateFromTransform(Transform.CreateRotationX(Math.DegToRad(30)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(0, 0.866, -0.5)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(0, 0.5, 0.866)))");
        script->Execute("m1 = Matrix3x3.CreateFromMatrix4x4(Matrix4x4.CreateRotationX(Math.DegToRad(30)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(0, 0.866, -0.5)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(0, 0.5, 0.866)))");
        script->Execute("m1 = Matrix3x3.CreateFromQuaternion(Quaternion.CreateRotationX(Math.DegToRad(30)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(0, 0.866, -0.5)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(0, 0.5, 0.866)))");
        script->Execute("m1 = Matrix3x3.CreateScale(Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector3(1, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector3(0, 2, 0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector3(0, 0, 3))");
        script->Execute("m1 = Matrix3x3.CreateDiagonal(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector3(2, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector3(0, 3, 0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector3(0, 0, 4))");
        script->Execute("m1 = Matrix3x3.CreateCrossProduct(Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector3(0, -3, 2))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector3(3, 0, -1))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector3(-2, 1, 0))");

        //element access
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssertFloatClose(m1:GetElement(1, 2), -0.5)");
        script->Execute("AZTestAssertFloatClose(m1:GetElement(2, 2), 0.866)");

        script->Execute("AZTestAssertFloatClose(m1.basisZ.y, -0.5)");

        script->Execute("m1:SetElement(2, 1, 5)");
        script->Execute("AZTestAssert(m1:GetElement(2, 1) == 5)");

        //index accessors
        script->Execute("AZTestAssertFloatClose(m1.basisZ.y, -0.5)");
        script->Execute("AZTestAssertFloatClose(m1.basisZ.z, 0.866)");
        script->Execute("m1:SetElement(2, 1, 15)");
        script->Execute("AZTestAssert(m1.basisY.z == 15)");

        //row access
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(0, 0.5, 0.866)))");
        script->Execute("m1:SetRow(0, 1, 2, 3)");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(1, 2, 3)))");
        script->Execute("m1:SetRow(1, Vector3(4, 5, 6))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(4, 5, 6)))");
        script->Execute("m1:SetRow(2, Vector3(7, 8, 9))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(7, 8, 9)))");
        //test GetRow with non - constant, we have different implementations for constants and variables
        script->Execute("AZTestAssert(m1:GetRow(testIndices[1]):IsClose(Vector3(1, 2, 3)))");
        script->Execute("AZTestAssert(m1:GetRow(testIndices[2]):IsClose(Vector3(4, 5, 6)))");
        script->Execute("AZTestAssert(m1:GetRow(testIndices[3]):IsClose(Vector3(7, 8, 9)))");
        script->Execute("row0, row1, row2 = m1:GetRows()");
        script->Execute("AZTestAssert(row0:IsClose(Vector3(1, 2, 3)))");
        script->Execute("AZTestAssert(row1:IsClose(Vector3(4, 5, 6)))");
        script->Execute("AZTestAssert(row2:IsClose(Vector3(7, 8, 9)))");
        script->Execute("m1:SetRows(Vector3(10, 11, 12), Vector3(13, 14, 15), Vector3(16, 17, 18))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(10, 11, 12)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(13, 14, 15)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(16, 17, 18)))");

        //column access
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetColumn(1):IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("m1:SetColumn(2, 1, 2, 3)");
        script->Execute("AZTestAssert(m1:GetColumn(2):IsClose(Vector3(1, 2, 3)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(1, 0, 1)))"); //checking all components in case others get messed up with the shuffling
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(0, 0.866, 2)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(0, 0.5, 3)))");
        script->Execute("m1:SetColumn(0, Vector3(2, 3, 4))");
        script->Execute("AZTestAssert(m1:GetColumn(0):IsClose(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector3(2, 0, 1)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector3(3, 0.866, 2)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector3(4, 0.5, 3)))");
        //test GetColumn with non - constant, we have different implementations for constants and variables
        script->Execute("AZTestAssert(m1:GetColumn(testIndices[1]):IsClose(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert(m1:GetColumn(testIndices[2]):IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(m1:GetColumn(testIndices[3]):IsClose(Vector3(1, 2, 3)))");
        script->Execute("col0, col1, col2 = m1:GetColumns()");
        script->Execute("AZTestAssert(col0:IsClose(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert(col1:IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(col2:IsClose(Vector3(1, 2, 3)))");
        script->Execute("m1:SetColumns(Vector3(10, 11, 12), Vector3(13, 14, 15), Vector3(16, 17, 18))");
        script->Execute("AZTestAssert(m1:GetColumn(0):IsClose(Vector3(10, 11, 12)))");
        script->Execute("AZTestAssert(m1:GetColumn(1):IsClose(Vector3(13, 14, 15)))");
        script->Execute("AZTestAssert(m1:GetColumn(2):IsClose(Vector3(16, 17, 18)))");

        //basis access
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1.basisX:IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(m1.basisY:IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(m1.basisZ:IsClose(Vector3(0, -0.5, 0.866)))");
        script->Execute("m1.basisX = Vector3(1, 2, 3)");
        script->Execute("AZTestAssert(m1.basisX:IsClose(Vector3(1, 2, 3)))");
        script->Execute("m1.basisY = Vector3(4, 5, 6)");
        script->Execute("AZTestAssert(m1.basisY:IsClose(Vector3(4, 5, 6)))");
        script->Execute("m1.basisZ = Vector3(7, 8, 9)");
        script->Execute("AZTestAssert(m1.basisZ:IsClose(Vector3(7, 8, 9)))");
        script->Execute("m1.basisX = Vector3(10, 11, 12)");
        script->Execute("AZTestAssert(m1.basisX:IsClose(Vector3(10, 11, 12)))");
        script->Execute("m1.basisY = Vector3(13, 14, 15)");
        script->Execute("AZTestAssert(m1.basisY:IsClose(Vector3(13, 14, 15)))");
        script->Execute("m1.basisZ = Vector3(16, 17, 18)");
        script->Execute("AZTestAssert(m1.basisZ:IsClose(Vector3(16, 17, 18)))");
        script->Execute("basisX, basisY, basisZ = m1:GetBasis()");
        script->Execute("AZTestAssert(basisX:IsClose(Vector3(10, 11, 12)))");
        script->Execute("AZTestAssert(basisY:IsClose(Vector3(13, 14, 15)))");
        script->Execute("AZTestAssert(basisZ:IsClose(Vector3(16, 17, 18)))");
        script->Execute("m1:SetBasis(Vector3(1, 2, 3), Vector3(4, 5, 6), Vector3(7, 8, 9))");
        script->Execute("AZTestAssert(m1.basisX:IsClose(Vector3(1, 2, 3)))");
        script->Execute("AZTestAssert(m1.basisY:IsClose(Vector3(4, 5, 6)))");
        script->Execute("AZTestAssert(m1.basisZ:IsClose(Vector3(7, 8, 9)))");

        //multiplication by matrices
        script->Execute("m1:SetRow(0, 1, 2, 3)");
        script->Execute("m1:SetRow(1, 4, 5, 6)");
        script->Execute("m1:SetRow(2, 7, 8, 9)");
        script->Execute("m2 = Matrix3x3()");
        script->Execute("m2:SetRow(0, 7, 8, 9)");
        script->Execute("m2:SetRow(1, 10, 11, 12)");
        script->Execute("m2:SetRow(2, 13, 14, 15)");
        script->Execute("m3 = m1 * m2");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(66, 72, 78)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(156, 171, 186)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(246, 270, 294)))");
        script->Execute("m3 = m1:TransposedMultiply(m2)");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(138, 150, 162)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(168, 183, 198)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(198, 216, 234)))");

        //multiplication by vectors
        script->Execute("AZTestAssert((m1*Vector3(1, 2, 3)):IsClose(Vector3(14, 32, 50)))");

        //other matrix operations
        script->Execute("m3 = m1 + m2");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(8, 10, 12)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(14, 16, 18)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(20, 22, 24)))");
        script->Execute("m3 = m1 - m2");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(-6, -6, -6)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(-6, -6, -6)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(-6, -6, -6)))");
        script->Execute("m3 = m1 * 2");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(2, 4, 6)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(8, 10, 12)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(14, 16, 18)))");
        //TODO float * matrix is not functional
        //script->Execute("m3 = 2 * m1");
        //script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(2, 4, 6)))");
        //script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(8, 10, 12)))");
        //script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(14, 16, 18)))");
        script->Execute("m3 = m1 / 0.5");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(2, 4, 6)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(8, 10, 12)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(14, 16, 18)))");
        script->Execute("m3 = -m1");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(-1, -2, -3)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(-4, -5, -6)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(-7, -8, -9)))");

        //transpose
        script->Execute("m1:SetRow(0, 1, 2, 3)");
        script->Execute("m1:SetRow(1, 4, 5, 6)");
        script->Execute("m1:SetRow(2, 7, 8, 9)");
        script->Execute("m2 = m1:GetTranspose()");
        script->Execute("AZTestAssert(m2:GetRow(0):IsClose(Vector3(1, 4, 7)))");
        script->Execute("AZTestAssert(m2:GetRow(1):IsClose(Vector3(2, 5, 8)))");
        script->Execute("AZTestAssert(m2:GetRow(2):IsClose(Vector3(3, 6, 9)))");
        script->Execute("m2 = m1:Clone()");
        script->Execute("m2:Transpose()");
        script->Execute("AZTestAssert(m2:GetRow(0):IsClose(Vector3(1, 4, 7)))");
        script->Execute("AZTestAssert(m2:GetRow(1):IsClose(Vector3(2, 5, 8)))");
        script->Execute("AZTestAssert(m2:GetRow(2):IsClose(Vector3(3, 6, 9)))");

        //test fast inverse, orthogonal matrix only
        script->Execute("m1 = Matrix3x3.CreateRotationX(1)");
        script->Execute("AZTestAssert((m1*m1:GetInverseFast()):IsClose(Matrix3x3.CreateIdentity(), 0.02))");
        script->Execute("m2 = Matrix3x3.CreateRotationZ(2) * Matrix3x3.CreateRotationX(1)");
        script->Execute("m3 = m2:GetInverseFast()");
        // allow a little bigger threshold, because of the 2 rot matrices(sin, cos differences)
        script->Execute("AZTestAssert((m2*m3):IsClose(Matrix3x3.CreateIdentity(), 0.1))");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector3(-0.420, 0.909, 0), 0.06))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector3(-0.493, -0.228, 0.841), 0.06))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector3(0.765, 0.353, 0.542), 0.06))");

        //test full inverse, any arbitrary matrix
        script->Execute("m1:SetRow(0, -1, 2, 3)");
        script->Execute("m1:SetRow(1, 4, 5, 6)");
        script->Execute("m1:SetRow(2, 7, 8, -9)");
        script->Execute("AZTestAssert((m1*m1:GetInverseFull()):IsClose(Matrix3x3.CreateIdentity()))");

        //scale access
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(40)) * Matrix3x3.CreateScale(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert(m1:RetrieveScale():IsClose(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert(m1:ExtractScale():IsClose(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert(m1:RetrieveScale():IsClose(Vector3.CreateOne()))");
        script->Execute("m1:MultiplyByScale(Vector3(3, 4, 5))");
        script->Execute("AZTestAssert(m1:RetrieveScale():IsClose(Vector3(3, 4, 5)))");

        //polar decomposition
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("m2 = Matrix3x3.CreateScale(Vector3(5, 6, 7))");
        script->Execute("m3 = m1 * m2");
        script->Execute("AZTestAssert(m3:GetPolarDecomposition():IsClose(m1))");
        script->Execute("m4, m5 = m3:GetPolarDecomposition()");
        script->Execute("AZTestAssert((m4*m5):IsClose(m3))");
        script->Execute("AZTestAssert(m4:IsClose(m1))");
        script->Execute("AZTestAssert(m5:IsClose(m2, 0.01))");

        //orthogonalize
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30)) * Matrix3x3.CreateScale(Vector3(2, 3, 4))");
        script->Execute("m1:SetElement(0, 1, 0.2)");
        script->Execute("m2 = m1:GetOrthogonalized()");
        script->Execute("AZTestAssertFloatClose(m2:GetRow(0):GetLength(),1)");
        script->Execute("AZTestAssertFloatClose(m2:GetRow(1):GetLength(),1)");
        script->Execute("AZTestAssertFloatClose(m2:GetRow(2):GetLength(),1)");
        script->Execute("AZTestAssertFloatClose(m2:GetRow(0):Dot(m2:GetRow(1)),0)");
        script->Execute("AZTestAssertFloatClose(m2:GetRow(0):Dot(m2:GetRow(2)),0)");
        script->Execute("AZTestAssertFloatClose(m2:GetRow(1):Dot(m2:GetRow(2)),0)");
        script->Execute("AZTestAssert(m2:GetRow(0):Cross(m2:GetRow(1)):IsClose(m2:GetRow(2)))");
        script->Execute("AZTestAssert(m2:GetRow(1):Cross(m2:GetRow(2)):IsClose(m2:GetRow(0)))");
        script->Execute("AZTestAssert(m2:GetRow(2):Cross(m2:GetRow(0)):IsClose(m2:GetRow(1)))");
        script->Execute("m1:Orthogonalize()");
        script->Execute("AZTestAssert(m1:IsClose(m2))");

        //orthogonal
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:IsOrthogonal(0.05))");
        script->Execute("m1:SetRow(1, m1:GetRow(1)*2)");
        script->Execute("AZTestAssert( not m1:IsOrthogonal(0.05))");
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("m1:SetRow(1, Vector3(0, 1, 0))");
        script->Execute("AZTestAssert( not m1:IsOrthogonal(0.05))");

        //IsClose
        script->Execute("m1 = Matrix3x3.CreateRotationX(Math.DegToRad(30))");
        script->Execute("m2 = m1:Clone()");
        script->Execute("AZTestAssert(m1:IsClose(m2))");
        script->Execute("m2:SetElement(0, 0, 2)");
        script->Execute("AZTestAssert( not m1:IsClose(m2))");
        script->Execute("m2 = m1:Clone()");
        script->Execute("m2:SetElement(0, 2, 2)");
        script->Execute("AZTestAssert( not m1:IsClose(m2))");

        //get diagonal
        script->Execute("m1:SetRow(0, 1, 2, 3)");
        script->Execute("m1:SetRow(1, 4, 5, 6)");
        script->Execute("m1:SetRow(2, 7, 8, 9)");
        script->Execute("AZTestAssert(m1:GetDiagonal() == Vector3(1, 5, 9))");

        //determinant
        script->Execute("m1:SetRow(0, -1, 2, 3)");
        script->Execute("m1:SetRow(1, 4, 5, 6)");
        script->Execute("m1:SetRow(2, 7, 8, -9)");
        script->Execute("AZTestAssertFloatClose(m1:GetDeterminant(),240)");

        //adjugate
        script->Execute("m1:SetRow(0, 1, 2, 3)");
        script->Execute("m1:SetRow(1, 4, 5, 6)");
        script->Execute("m1:SetRow(2, 7, 8, 9)");
        script->Execute("m2 = m1:GetAdjugate()");
        script->Execute("AZTestAssert(m2:GetRow(0):IsClose(Vector3(-3, 6, -3)))");
        script->Execute("AZTestAssert(m2:GetRow(1):IsClose(Vector3(6, -12, 6)))");
        script->Execute("AZTestAssert(m2:GetRow(2):IsClose(Vector3(-3, 6, -3)))");
    }

    TEST_F(ScriptMathTest, LuaMatrix4x4Test)
    {
        script->Execute("testIndices = { 0, 1, 2, 3 }");

        ////create functions
        script->Execute("m1 = Matrix4x4.CreateIdentity()");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector4(1, 0, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector4(0, 1, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector4(0, 0, 1, 0))");
        script->Execute("AZTestAssert(m1:GetRow(3) == Vector4(0, 0, 0, 1))");
        script->Execute("m1 = Matrix4x4.CreateZero()");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector4(0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector4(0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector4(0))");
        script->Execute("AZTestAssert(m1:GetRow(3) == Vector4(0))");
        script->Execute("m1 = Matrix4x4.CreateFromValue(2)");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector4(2))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector4(2))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector4(2))");
        script->Execute("AZTestAssert(m1:GetRow(3) == Vector4(2))");

        script->Execute("m1 = Matrix4x4.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(1, 0, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 0.866, -0.5, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0.5, 0.866, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 0, 1)))");
        script->Execute("m1 = Matrix4x4.CreateRotationY(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(0.866, 0, 0.5, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 1, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(-0.5, 0, 0.866, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 0, 1)))");
        script->Execute("m1 = Matrix4x4.CreateRotationZ(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(0.866, -0.5, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0.5, 0.866, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0, 1, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 0, 1)))");
        script->Execute("m1 = Matrix4x4.CreateFromQuaternion(Quaternion.CreateRotationX(Math.DegToRad(30)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(1, 0, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 0.866, -0.5, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0.5, 0.866, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 0, 1)))");
        script->Execute("m1 = Matrix4x4.CreateFromQuaternionAndTranslation(Quaternion.CreateRotationX(Math.DegToRad(30)), Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(1, 0, 0, 1)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 0.866, -0.5, 2)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0.5, 0.866, 3)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 0, 1)))");
        script->Execute("m1 = Matrix4x4.CreateScale(Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector4(1, 0, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector4(0, 2, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector4(0, 0, 3, 0))");
        script->Execute("AZTestAssert(m1:GetRow(3) == Vector4(0, 0, 0, 1))");
        script->Execute("m1 = Matrix4x4.CreateDiagonal(Vector4(2, 3, 4, 5))");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector4(2, 0, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector4(0, 3, 0, 0))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector4(0, 0, 4, 0))");
        script->Execute("AZTestAssert(m1:GetRow(3) == Vector4(0, 0, 0, 5))");
        script->Execute("m1 = Matrix4x4.CreateTranslation(Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(m1:GetRow(0) == Vector4(1, 0, 0, 1))");
        script->Execute("AZTestAssert(m1:GetRow(1) == Vector4(0, 1, 0, 2))");
        script->Execute("AZTestAssert(m1:GetRow(2) == Vector4(0, 0, 1, 3))");
        script->Execute("AZTestAssert(m1:GetRow(3) == Vector4(0, 0, 0, 1))");

        script->Execute("m1 = Matrix4x4.CreateProjection(Math.DegToRad(30), 16 / 9, 1, 1000)");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(-2.0993, 0, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 3.732, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0, 1.002, -2.002)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 1, 0)))");
        script->Execute("m1 = Matrix4x4.CreateProjectionFov(Math.DegToRad(30), Math.DegToRad(60), 1, 1000)");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(-3.732, 0, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 1.732, 0, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0, 1.002, -2.002)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 1, 0)))");
        script->Execute("m1 = Matrix4x4.CreateProjectionOffset(0.5, 1, 0, 0.5, 1, 1000)");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(-4, 0, -3, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 4, -1, 0)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0, 1.002, -2.002)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 1, 0)))");

        ////element access
        script->Execute("m1 = Matrix4x4.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssertFloatClose(m1:GetElement(1, 2), -0.5)");
        script->Execute("AZTestAssertFloatClose(m1:GetElement(2, 2), 0.866)");
        script->Execute("m1:SetElement(2, 1, 5)");
        script->Execute("AZTestAssert(m1:GetElement(2, 1) == 5)");

        ////row access
        script->Execute("m1 = Matrix4x4.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0.5, 0.866, 0)))");
        script->Execute("AZTestAssert(m1:GetRowAsVector3(2):IsClose(Vector3(0, 0.5, 0.866)))");
        script->Execute("m1:SetRow(0, 1, 2, 3, 4)");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(1, 2, 3, 4)))");
        script->Execute("m1:SetRow(1, Vector3(5, 6, 7), 8)");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(5, 6, 7, 8)))");
        script->Execute("m1:SetRow(2, Vector4(3, 4, 5, 6))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(3, 4, 5, 6)))");
        script->Execute("m1:SetRow(3, Vector4(7, 8, 9, 10))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(7, 8, 9, 10)))");
        ////test GetRow with non-constant, we have different implementations for constants and variables
        script->Execute("AZTestAssert(m1:GetRow(testIndices[1]):IsClose(Vector4(1, 2, 3, 4)))");
        script->Execute("AZTestAssert(m1:GetRow(testIndices[2]):IsClose(Vector4(5, 6, 7, 8)))");
        script->Execute("AZTestAssert(m1:GetRow(testIndices[3]):IsClose(Vector4(3, 4, 5, 6)))");
        script->Execute("AZTestAssert(m1:GetRow(testIndices[4]):IsClose(Vector4(7, 8, 9, 10)))");

        ////column access
        script->Execute("m1 = Matrix4x4.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(m1:GetColumn(1):IsClose(Vector4(0, 0.866, 0.5, 0)))");
        script->Execute("m1:SetColumn(3, 1, 2, 3, 4)");
        script->Execute("AZTestAssert(m1:GetColumn(3):IsClose(Vector4(1, 2, 3, 4)))");
        script->Execute("AZTestAssert(m1:GetColumnAsVector3(3):IsClose(Vector3(1, 2, 3)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(1, 0, 0, 1)))");  //checking all components in case others get messed up with the shuffling
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 0.866, -0.5, 2)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0.5, 0.866, 3)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(0, 0, 0, 4)))");
        script->Execute("m1:SetColumn(0, Vector4(2, 3, 4, 5))");
        script->Execute("AZTestAssert(m1:GetColumn(0):IsClose(Vector4(2, 3, 4, 5)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(2, 0, 0, 1)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(3, 0.866, -0.5, 2)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(4, 0.5, 0.866, 3)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(5, 0, 0, 4)))");
        ////test GetColumn with non-constant, we have different implementations for constants and variables
        script->Execute("AZTestAssert(m1:GetColumn(testIndices[1]):IsClose(Vector4(2, 3, 4, 5)))");
        script->Execute("AZTestAssert(m1:GetColumn(testIndices[2]):IsClose(Vector4(0, 0.866, 0.5, 0)))");
        script->Execute("AZTestAssert(m1:GetColumn(testIndices[3]):IsClose(Vector4(0, -0.5, 0.866, 0)))");
        script->Execute("AZTestAssert(m1:GetColumn(testIndices[4]):IsClose(Vector4(1, 2, 3, 4)))");

        ////translation access
        script->Execute("m1 = Matrix4x4.CreateTranslation(Vector3(5, 6, 7))");
        script->Execute("AZTestAssert(m1:GetTranslation():IsClose(Vector3(5, 6, 7)))");
        script->Execute("m1:SetTranslation(1, 2, 3)");
        script->Execute("AZTestAssert(m1:GetTranslation():IsClose(Vector3(1, 2, 3)))");
        script->Execute("m1:SetTranslation(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert(m1:GetTranslation():IsClose(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert(m1:GetTranslation():IsClose(Vector3(2, 3, 4)))");
        script->Execute("m1:SetTranslation(4, 5, 6)");
        script->Execute("AZTestAssert(m1:GetTranslation():IsClose(Vector3(4, 5, 6)))");
        script->Execute("m1:SetTranslation(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert(m1:GetTranslation():IsClose(Vector3(2, 3, 4)))");

        ////multiplication by matrices
        script->Execute("m1:SetRow(0, 1, 2, 3, 4)");
        script->Execute("m1:SetRow(1, 5, 6, 7, 8)");
        script->Execute("m1:SetRow(2, 9, 10, 11, 12)");
        script->Execute("m1:SetRow(3, 13, 14, 15, 16)");
        script->Execute("m2 = Matrix4x4()");
        script->Execute("m2:SetRow(0, 7, 8, 9, 10)");
        script->Execute("m2:SetRow(1, 11, 12, 13, 14)");
        script->Execute("m2:SetRow(2, 15, 16, 17, 18)");
        script->Execute("m2:SetRow(3, 19, 20, 21, 22)");
        script->Execute("m3 = m1 * m2");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector4(150, 160, 170, 180)))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector4(358, 384, 410, 436)))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector4(566, 608, 650, 692)))");
        script->Execute("AZTestAssert(m3:GetRow(3):IsClose(Vector4(774, 832, 890, 948)))");

        ////multiplication by vectors
        script->Execute("AZTestAssert((m1*Vector3(1, 2, 3)):IsClose(Vector3(18, 46, 74)))");
        script->Execute("AZTestAssert((m1*Vector4(1, 2, 3, 4)):IsClose(Vector4(30, 70, 110, 150)))");
        script->Execute("AZTestAssert(m1:TransposedMultiply3x3(Vector3(1, 2, 3)):IsClose(Vector3(38, 44, 50)))");
        script->Execute("AZTestAssert(m1:Multiply3x3(Vector3(1, 2, 3)):IsClose(Vector3(14, 38, 62)))");

        ////transpose
        script->Execute("m1:SetRow(0, 1, 2, 3, 4)");
        script->Execute("m1:SetRow(1, 5, 6, 7, 8)");
        script->Execute("m1:SetRow(2, 9, 10, 11, 12)");
        script->Execute("m1:SetRow(3, 13, 14, 15, 16)");
        script->Execute("m2 = m1:GetTranspose()");
        script->Execute("AZTestAssert(m2:GetRow(0):IsClose(Vector4(1, 5, 9, 13)))");
        script->Execute("AZTestAssert(m2:GetRow(1):IsClose(Vector4(2, 6, 10, 14)))");
        script->Execute("AZTestAssert(m2:GetRow(2):IsClose(Vector4(3, 7, 11, 15)))");
        script->Execute("AZTestAssert(m2:GetRow(3):IsClose(Vector4(4, 8, 12, 16)))");
        script->Execute("m2 = m1:Clone()");
        script->Execute("m2:Transpose()");
        script->Execute("AZTestAssert(m2:GetRow(0):IsClose(Vector4(1, 5, 9, 13)))");
        script->Execute("AZTestAssert(m2:GetRow(1):IsClose(Vector4(2, 6, 10, 14)))");
        script->Execute("AZTestAssert(m2:GetRow(2):IsClose(Vector4(3, 7, 11, 15)))");
        script->Execute("AZTestAssert(m2:GetRow(3):IsClose(Vector4(4, 8, 12, 16)))");

        ////test fast inverse, orthogonal matrix with translation only
        script->Execute("m1 = Matrix4x4.CreateRotationX(1)");
        script->Execute("m1:SetTranslation(Vector3(10, -3, 5))");
        script->Execute("AZTestAssert((m1*m1:GetInverseFast()):IsClose(Matrix4x4.CreateIdentity(), 0.02))");
        script->Execute("m2 = Matrix4x4.CreateRotationZ(2) * Matrix4x4.CreateRotationX(1)");
        script->Execute("m2:SetTranslation(Vector3(-5, 4.2, -32))");
        script->Execute("m3 = m2:GetInverseFast()");
        //// allow a little bigger threshold, because of the 2 rot matrices (sin,cos differences)
        script->Execute("AZTestAssert((m2*m3):IsClose(Matrix4x4.CreateIdentity(), 0.1))");
        script->Execute("AZTestAssert(m3:GetRow(0):IsClose(Vector4(-0.420, 0.909, 0, -5.920), 0.06))");
        script->Execute("AZTestAssert(m3:GetRow(1):IsClose(Vector4(-0.493, -0.228, 0.841, 25.418), 0.06))");
        script->Execute("AZTestAssert(m3:GetRow(2):IsClose(Vector4(0.765, 0.353, 0.542, 19.703), 0.06))");
        script->Execute("AZTestAssert(m3:GetRow(3):IsClose(Vector4(0, 0, 0, 1)))");

        ////test transform inverse, should handle non-orthogonal matrices, last row must still be (0,0,0,1) though
        script->Execute("m1:SetElement(0, 1, 23.1234)");
        script->Execute("AZTestAssert((m1*m1:GetInverseTransform()):IsClose(Matrix4x4.CreateIdentity(), 0.01))");

        ////test full inverse, any arbitrary matrix
        script->Execute("m1:SetRow(0, -1, 2, 3, 4)");
        script->Execute("m1:SetRow(1, 3, 4, 5, 6)");
        script->Execute("m1:SetRow(2, 9, 10, 11, 12)");
        script->Execute("m1:SetRow(3, 13, 14, 15, -16)");
        script->Execute("AZTestAssert((m1*m1:GetInverseFull()):IsClose(Matrix4x4.CreateIdentity()))");

        ////IsClose
        script->Execute("m1 = Matrix4x4.CreateRotationX(Math.DegToRad(30))");
        script->Execute("m2 = m1:Clone()");
        script->Execute("AZTestAssert(m1:IsClose(m2))");
        script->Execute("m2:SetElement(0, 0, 2)");
        script->Execute("AZTestAssert( not m1:IsClose(m2))");
        script->Execute("m2 = m1:Clone()");
        script->Execute("m2:SetElement(0, 3, 2)");
        script->Execute("AZTestAssert( not m1:IsClose(m2))");

        ////set rotation part
        script->Execute("m1 = Matrix4x4.CreateTranslation(Vector3(1, 2, 3))");
        script->Execute("m1:SetRow(3, 5, 6, 7, 8)");
        script->Execute("m1:SetRotationPartFromQuaternion(Quaternion.CreateRotationX(Math.DegToRad(30)))");
        script->Execute("AZTestAssert(m1:GetRow(0):IsClose(Vector4(1, 0, 0, 1)))");
        script->Execute("AZTestAssert(m1:GetRow(1):IsClose(Vector4(0, 0.866, -0.5, 2)))");
        script->Execute("AZTestAssert(m1:GetRow(2):IsClose(Vector4(0, 0.5, 0.866, 3)))");
        script->Execute("AZTestAssert(m1:GetRow(3):IsClose(Vector4(5, 6, 7, 8)))");

        ////get diagonal
        script->Execute("m1:SetRow(0, 1, 2, 3, 4)");
        script->Execute("m1:SetRow(1, 5, 6, 7, 8)");
        script->Execute("m1:SetRow(2, 9, 10, 11, 12)");
        script->Execute("m1:SetRow(3, 13, 14, 15, 16)");
        script->Execute("AZTestAssert(m1:GetDiagonal() == Vector4(1, 6, 11, 16))");
    }

    TEST_F(ScriptMathTest, LuaTransformTest)
    {
        script->Execute("testIndices = { 0, 1, 2, 3 }");

        ////create functions
        script->Execute("t1 = Transform.CreateIdentity()");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0 ,1, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, 0, 1)))");
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(30))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, -0.5, 0.866)))");
        script->Execute("t1 = Transform.CreateRotationY(Math.DegToRad(30))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(0.866, 0, -0.5)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 1, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0.5, 0, 0.866)))");
        script->Execute("t1 = Transform.CreateRotationZ(Math.DegToRad(30))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(0.866, 0.5, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(-0.5, 0.866, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, 0, 1)))");
        script->Execute("t1 = Transform.CreateFromQuaternion(Quaternion.CreateRotationX(Math.DegToRad(30)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, -0.5, 0.866)))");
        script->Execute("t1 = Transform.CreateFromQuaternionAndTranslation(Quaternion.CreateRotationX(Math.DegToRad(30)), Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, -0.5, 0.866)))");
        script->Execute("t1 = Transform.CreateUniformScale(2)");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(2, 0, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 2, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, 0, 2)))");
        script->Execute("t1 = Transform.CreateTranslation(Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 1, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, 0, 1)))");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(1, 2, 3)))");

        ////basis access
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(30))");
        script->Execute("t1:SetTranslation(1, 2, 3)");
        script->Execute("AZTestAssert(t1:GetBasisX():IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(t1:GetBasisY():IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(t1:GetBasisZ():IsClose(Vector3(0, -0.5, 0.866)))");
        script->Execute("basis0, basis1, basis2, pos = t1:GetBasisAndTranslation()");
        script->Execute("AZTestAssert(basis0:IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(basis1:IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(basis2:IsClose(Vector3(0, -0.5, 0.866)))");
        script->Execute("AZTestAssert(pos:IsClose(Vector3(1, 2, 3)))");

        ////translation access
        script->Execute("t1 = Transform.CreateTranslation(Vector3(5, 6, 7))");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(5, 6, 7)))");
        script->Execute("t1:SetTranslation(1, 2, 3)");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(1, 2, 3)))");
        script->Execute("t1:SetTranslation(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(2, 3, 4)))");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(2, 3, 4)))");
        script->Execute("t1:SetTranslation(4, 5, 6)");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(4, 5, 6)))");
        script->Execute("t1:SetTranslation(Vector3(2, 3, 4))");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(2, 3, 4)))");

        ////multiplication by transforms
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(30))");
        script->Execute("t1:SetTranslation(Vector3(1, 2, 3))");
        script->Execute("t2 = Transform.CreateRotationY(Math.DegToRad(30))");
        script->Execute("t2:SetTranslation(Vector3(2, 3, 4))");
        script->Execute("t3 = t1 * t2");
        script->Execute("AZTestAssert(t3:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(0.866, 0.25, -0.433)))");
        script->Execute("AZTestAssert(t3:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(t3:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0.5, -0.433, 0.75)))");
        script->Execute("AZTestAssert(t3:GetTranslation():IsClose(Vector3(3, 2.598, 7.964), 0.001))");

        ////multiplication by vectors
        script->Execute("AZTestAssert((t1*Vector3(1, 2, 3)):IsClose(Vector3(2, 2.224, 6.598), 0.02))");
        script->Execute("AZTestAssert((t1*Vector4(1, 2, 3, 4)):IsClose(Vector4(5, 8.224, 15.598, 4), 0.02))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 2, 3)):IsClose(Vector3(1, 0.224, 3.598), 0.02))");

        ////test fast inverse, orthogonal matrix with translation only
        script->Execute("t1 = Transform.CreateRotationX(1)");
        script->Execute("t1:SetTranslation(Vector3(10, -3, 5))");
        script->Execute("AZTestAssert((t1*t1:GetInverse()):IsClose(Transform.CreateIdentity(), 0.02))");
        script->Execute("t2 = Transform.CreateRotationZ(2) * Transform.CreateRotationX(1)");
        script->Execute("t2:SetTranslation(Vector3(-5, 4.2, -32))");
        script->Execute("t3 = t2:GetInverse()");
        //// allow a little bigger threshold, because of the 2 rot matrices (sin,cos differences)
        script->Execute("AZTestAssert((t2*t3):IsClose(Transform.CreateIdentity(), 0.1))");
        script->Execute("AZTestAssert(t3:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(-0.420, -0.493, 0.765), 0.06))");
        script->Execute("AZTestAssert(t3:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0.909,-0.228, 0.353), 0.06))");
        script->Execute("AZTestAssert(t3:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, 0.841, 0.542), 0.06))");
        script->Execute("AZTestAssert(t3:GetTranslation():IsClose(Vector3(-5.90, 25.415, 19.645), 0.001))");

        ////test inverse, should handle non-orthogonal matrices
        script->Execute("t1 = Transform.CreateRotationX(1) * Transform.CreateUniformScale(2)");
        script->Execute("AZTestAssert((t1*t1:GetInverse()):IsClose(Transform.CreateIdentity()))");

        ////scale access
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(40)) * Transform.CreateUniformScale(3)");
        script->Execute("AZTestAssertFloatClose(t1:GetUniformScale(), 3)");
        script->Execute("AZTestAssertFloatClose(t1:ExtractUniformScale(), 3)");
        script->Execute("AZTestAssertFloatClose(t1:GetUniformScale(), 1)");
        script->Execute("t1:MultiplyByUniformScale(2)");
        script->Execute("AZTestAssertFloatClose(t1:GetUniformScale(), 2)");

        ////orthogonalize
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(30)) * Transform.CreateUniformScale(3)");
        script->Execute("t1:SetTranslation(Vector3(1,2,3))");
        script->Execute("t2 = t1:GetOrthogonalized()");
        script->Execute("AZTestAssertFloatClose(t2:GetBasisX():GetLength(), 1)");
        script->Execute("AZTestAssertFloatClose(t2:GetBasisY():GetLength(), 1)");
        script->Execute("AZTestAssertFloatClose(t2:GetBasisZ():GetLength(), 1)");
        script->Execute("AZTestAssertFloatClose(t2:GetBasisX():Dot(t2:GetBasisY()), 0)");
        script->Execute("AZTestAssertFloatClose(t2:GetBasisX():Dot(t2:GetBasisZ()), 0)");
        script->Execute("AZTestAssertFloatClose(t2:GetBasisY():Dot(t2:GetBasisZ()), 0)");
        script->Execute("AZTestAssert(t2:GetBasisX():Cross(t2:GetBasisY()):IsClose(t2:GetBasisZ()))");
        script->Execute("AZTestAssert(t2:GetBasisY():Cross(t2:GetBasisZ()):IsClose(t2:GetBasisX()))");
        script->Execute("AZTestAssert(t2:GetBasisZ():Cross(t2:GetBasisX()):IsClose(t2:GetBasisY()))");
        script->Execute("t1:Orthogonalize()");
        script->Execute("AZTestAssert(t1:IsClose(t2))");

        ////orthogonal
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(30))");
        script->Execute("t1:SetTranslation(Vector3(1, 2, 3))");
        script->Execute("AZTestAssert(t1:IsOrthogonal(0.05))");
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(30)) * Transform.CreateUniformScale(2)");
        script->Execute("AZTestAssert( not t1:IsOrthogonal(0.05))");

        ////IsClose
        script->Execute("t1 = Transform.CreateRotationX(Math.DegToRad(30))");
        script->Execute("t2 = t1:Clone()");
        script->Execute("AZTestAssert(t1:IsClose(t2))");
        script->Execute("t2 = Transform.CreateRotationX(Math.DegToRad(30.01))");
        script->Execute("AZTestAssert(t1:IsClose(t2))");
        script->Execute("t2 = Transform.CreateRotationX(Math.DegToRad(32))");
        script->Execute("AZTestAssert( not t1:IsClose(t2))");

        ////set rotation part
        script->Execute("t1 = Transform.CreateTranslation(Vector3(1,2,3))");
        script->Execute("t1:SetRotation(Quaternion.CreateRotationX(Math.DegToRad(30)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(1, 0, 0)):IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 1, 0)):IsClose(Vector3(0, 0.866, 0.5)))");
        script->Execute("AZTestAssert(t1:TransformVector(Vector3(0, 0, 1)):IsClose(Vector3(0, -0.5, 0.866)))");
        script->Execute("AZTestAssert(t1:GetTranslation():IsClose(Vector3(1, 2, 3)))");
    }

    TEST_F(ScriptMathTest, LuaPlaneTest)
    {
        script->Execute("pl = Plane.CreateFromNormalAndDistance(Vector3(1,0,0),-100)");
        script->Execute("AZTestAssertFloatClose(pl:GetDistance(),-100)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().x,1)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().y,0)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().z,0)");

        script->Execute("pl = Plane.CreateFromNormalAndPoint(Vector3(0, 1, 0), Vector3(10, 10, 10))");
        script->Execute("AZTestAssertFloatClose(pl:GetDistance(), -10)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().x, 0)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().y, 1)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().z, 0)");

        script->Execute("pl = Plane.CreateFromCoefficients(0,-1,0,-5)");
        script->Execute("AZTestAssertFloatClose(pl:GetDistance(),-5)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().x,0)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().y,-1)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().z,0)");

        AZ_MATH_TEST_START_TRACE_SUPPRESSION;
        script->Execute("pl:Set(12, 13, 14, 15)");
        AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(1);
        script->Execute("AZTestAssertFloatClose(pl:GetDistance(), 15)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().x, 12)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().y, 13)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().z, 14)");

        AZ_MATH_TEST_START_TRACE_SUPPRESSION;
        script->Execute("pl:Set(Vector3(22, 23, 24), 25)");
        AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(1);
        script->Execute("AZTestAssertFloatClose(pl:GetDistance(), 25)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().x, 22)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().y, 23)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().z, 24)");

        script->Execute("pl:Set(Vector4(32, 33, 34, 35))");
        script->Execute("AZTestAssertFloatClose(pl:GetDistance(), 35)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().x, 32)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().y, 33)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().z, 34)");

        script->Execute("pl:SetNormal(Vector3(0,0,1))");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().x,0)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().y,0)");
        script->Execute("AZTestAssertFloatClose(pl:GetNormal().z,1)");

        script->Execute("pl:SetDistance(55)");
        script->Execute("AZTestAssertFloatClose(pl:GetDistance(),55)");

        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().w,55)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().x,0)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().y,0)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().z,1)");

        script->Execute("tm = Transform.CreateRotationY(Math.DegToRad(90))");
        script->Execute("pl = pl:GetTransform(tm)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().w,55)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().x,1)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().y,0)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().z, 0)");

        script->Execute("dist = pl:GetPointDist(Vector3(10, 0, 0))");
        script->Execute("AZTestAssertFloatClose(dist, 65)");

        script->Execute("tm = Transform.CreateRotationZ(Math.DegToRad(45))");
        script->Execute("pl:ApplyTransform(tm)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().w, 55)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().x, 0.7071)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().y, 0.7071)");
        script->Execute("AZTestAssertFloatClose(pl:GetPlaneEquationCoefficients().z, 0)");

        script->Execute("pl:SetNormal(Vector3(0, 0, 1))");
        script->Execute("v1 = pl:GetProjected(Vector3(10, 15, 20))");
        script->Execute("AZTestAssert(v1 == Vector3(10, 15, 0))");

        script->Execute("pl:Set(0, 0, 1, 10)");
        script->Execute("hit, v1, time = pl:CastRay(Vector3(0, 0, 0), Vector3(0, 0, 1))");
        script->Execute("AZTestAssert(hit == true)");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(0, 0, -10)))");

        script->Execute("pl:Set(0, 1, 0, 10)");
        script->Execute("hit, v1, time = pl:CastRay(Vector3(0, 1, 0), Vector3(0, -1, 0))");
        script->Execute("AZTestAssert(hit == true)");
        script->Execute("AZTestAssertFloatClose(time,10.999)");

        script->Execute("pl:Set(1, 0, 0, 5)");
        script->Execute("hit, v1, time = pl:CastRay(Vector3(0, 1, 0), Vector3(0, -1, 0))");
        script->Execute("AZTestAssert(hit == false)");

        script->Execute("pl:Set(1, 0, 0, 0)");
        script->Execute("hit, v1, time = pl:IntersectSegment(Vector3(-1, 0, 0), Vector3(1, 0, 0))");
        script->Execute("AZTestAssert(hit == true)");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(0, 0, 0)))");
        script->Execute("AZTestAssertFloatClose(time, 0.5)");

        script->Execute("pl:Set(0, 1, 0, 0)");
        script->Execute("hit, v1, time = pl:IntersectSegment(Vector3(0, -1, 0), Vector3(0, 9, 0))");
        script->Execute("AZTestAssert(hit == true)");
        script->Execute("AZTestAssert(v1:IsClose(Vector3(0, 0, 0)))");
        script->Execute("AZTestAssertFloatClose(time, 0.1)");

        script->Execute("pl:Set(0, 1, 0, 20)");
        script->Execute("hit, v1, time = pl:IntersectSegment(Vector3(-1, 0, 0), Vector3(1, 0, 0))");
        script->Execute("AZTestAssert(hit == false)");

        script->Execute("pl:Set(1, 0, 0, 0)");
        script->Execute("AZTestAssert(pl:IsFinite())");
        AZ_MATH_TEST_START_TRACE_SUPPRESSION;
        script->Execute("pl:Set(math.huge, math.huge, math.huge, math.huge)");
        AZ_MATH_TEST_STOP_TRACE_SUPPRESSION(1);
        script->Execute("AZTestAssert( not pl:IsFinite())");
    }

    TEST_F(ScriptMathTest, LuaUuidTest)
    {
        // null
        script->Execute("id = Uuid.CreateNull()");
        script->Execute("AZTestAssert(id:IsNull())");

        script->Execute("defId = Uuid.CreateString(\"{B5700F2E-661B-4AC0-9335-817CB4C09CCB}\")");

        script->Execute("idStr1 = \"{B5700F2E-661B-4AC0-9335-817CB4C09CCB}\"");
        script->Execute("idStr2 = \"{B5700F2E661B4AC09335817CB4C09CCB}\"");
        script->Execute("idStr3 = \"B5700F2E-661B-4AC0-9335-817CB4C09CCB\"");
        script->Execute("idStr4 = \"B5700F2E661B4AC09335817CB4C09CCB\"");

        // create from string
        script->Execute("id = Uuid.CreateString(idStr1)");
        script->Execute("AZTestAssert(id==defId)");
        script->Execute("id = Uuid.CreateString(idStr2)");
        script->Execute("AZTestAssert(id==defId)");
        script->Execute("id = Uuid.CreateString(idStr3)");
        script->Execute("AZTestAssert(id==defId)");
        script->Execute("id = Uuid.CreateString(idStr4)");
        script->Execute("AZTestAssert(id==defId)");

        // tostring
        script->Execute("id = Uuid.CreateString(idStr1)");
        script->Execute("asString = tostring(id)");
        script->Execute("AZTestAssert(asString == idStr1)");

        // operators
        script->Execute("idBigger = Uuid(\"C5700F2E661B4ac09335817CB4C09CCB\")");
        script->Execute("AZTestAssert(id < idBigger)");
        script->Execute("AZTestAssert(id ~= idBigger)");
        script->Execute("AZTestAssert(idBigger > id)");


        // test the name function
        script->Execute("uuidName = Uuid.CreateName(\"BlaBla\")");
        // check id
        script->Execute("AZTestAssert(uuidName == Uuid.CreateName(\"BlaBla\"))");
    }

    TEST_F(ScriptMathTest, LuaCrc32Test)
    {
        // constructor, value, tostring.
        script->Execute("stringToCrc = \"blabla\" crc32 = Crc32(stringToCrc)");
        script->Execute("AZTestAssert(crc32.value == 0x93086ff3)");
        script->Execute("AZTestAssert(tostring(crc32) == \"0x93086ff3\")");

        // == and ~= operators.
        script->Execute("crc32 = Crc32(stringToCrc)");
        script->Execute("AZTestAssert(crc32 == Crc32(stringToCrc))");
        script->Execute("AZTestAssert(crc32 ~= Crc32(\"SomeOtherString\"))");

        // TODO : calling any functions implemented with a generic
        // script context on Crc32 causes an error.
        // add(string), add(data, size)
        /*script->Execute("stringHello = \"Hello\" stringWorld = \"World\"");
        script->Execute("crcAdded = Crc32(stringHello)");
        script->Execute("crcAdded:Add(stringWorld)");
        script->Execute("crc32 = Crc32(\"HelloWorld\")");
        script->Execute("AZTestAssert(crcAdded == crc32");*/

        // tostring
        script->Execute("crc32 = Crc32(0x12345680)");
        script->Execute("AZTestAssert(tostring(crc32) == \"0x12345680\")");

        AZ_TEST_START_TRACE_SUPPRESSION;
        script->Execute("globalCrc32Prop = crc32.value");
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(ScriptMathTest, LuaAabbTest)
    {
        ////CreateNull
        ////IsValid
        script->Execute("aabb = Aabb.CreateNull()");
        script->Execute("AZTestAssert( not aabb:IsValid())");
        script->Execute("AZTestAssert(aabb.min:IsGreaterThan(aabb.max))");

        ////CreateFromPoint
        script->Execute("aabb = Aabb.CreateFromPoint(Vector3(0))");
        script->Execute("AZTestAssert(aabb:IsValid())");
        script->Execute("AZTestAssert(aabb:IsFinite())");
        script->Execute("AZTestAssert(aabb.min:IsClose(aabb.max))");

        ////CreateFromMinMax
        script->Execute("aabb = Aabb.CreateFromMinMax(Vector3(0),Vector3(1))");
        script->Execute("AZTestAssert(aabb:IsValid())");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(0)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(1)))");

        ////CreateCenterHalfExtents
        script->Execute("aabb = Aabb.CreateCenterHalfExtents(Vector3(1), Vector3(1))");
        script->Execute("AZTestAssert(aabb:IsValid())");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(0)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(2)))");
        script->Execute("AZTestAssert(aabb:GetCenter():IsClose(Vector3(1)))");

        ////CreateCenterRadius
        ////GetExtents
        script->Execute("aabb = Aabb.CreateCenterRadius(Vector3(4), 2)");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(2)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(6)))");
        script->Execute("AZTestAssert(aabb:GetExtents():IsClose(Vector3(4)))");

        ////CreateFromObb
        script->Execute("position = Vector3(1, 1, 1)");
        script->Execute("rotation = Quaternion.CreateRotationZ(Math.DegToRad(45))");
        script->Execute("halfLengths = Vector3(1, 1, 1)");
        script->Execute("obb = Obb.CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths)");
        script->Execute("aabb = Aabb.CreateFromObb(obb)");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(-0.414, -0.414, 0.0), 0.001))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(2.414, 2.414, 2.0), 0.001))");
        script->Execute("AZTestAssert(aabb:GetCenter():IsClose(Vector3(1, 1, 1)))");

        ////Set(min, max)
        ////GetMin
        ////GetMax
        script->Execute("aabb:Set(Vector3(1), Vector3(4))");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(1)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(4)))");

        ////SetMin
        script->Execute("aabb.min = Vector3(0)");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(0)))");
        script->Execute("AZTestAssert(aabb:GetCenter():IsClose(Vector3(2)))");

        ////SetMax
        script->Execute("aabb.max = Vector3(6)");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(6)))");
        script->Execute("AZTestAssert(aabb:GetCenter():IsClose(Vector3(3)))");

        ////GetXExtent
        ////GetYExtent
        ////GetZExtent
        script->Execute("aabb:Set(Vector3(0), Vector3(1, 2, 3))");
        script->Execute("AZTestAssertFloatClose(aabb:GetXExtent(), 1)");
        script->Execute("AZTestAssertFloatClose(aabb:GetYExtent(), 2)");
        script->Execute("AZTestAssertFloatClose(aabb:GetZExtent(), 3)");

        ////GetAsSphere
        script->Execute("aabb:Set(Vector3(-2, 0, 0), Vector3(2, 0, 0))");
        script->Execute("center, radius = aabb:GetAsSphere()");
        script->Execute("AZTestAssert(center:IsClose(Vector3(0)))");
        script->Execute("AZTestAssertFloatClose(radius,2)");

        ////Contains(Vector3)
        script->Execute("aabb:Set(Vector3(-2), Vector3(2))");
        script->Execute("AZTestAssert(aabb:Contains(Vector3(1)))");

        ////Contains(Aabb)
        script->Execute("aabb:Set(Vector3(0), Vector3(3))");
        script->Execute("aabb2 = Aabb.CreateFromMinMax(Vector3(1), Vector3(2))");
        script->Execute("AZTestAssert(aabb:Contains(aabb2))");
        script->Execute("AZTestAssert( not aabb2:Contains(aabb))");

        ////Overlaps
        script->Execute("aabb:Set(Vector3(0), Vector3(2))");
        script->Execute("aabb2:Set(Vector3(1), Vector3(3))");
        script->Execute("AZTestAssert(aabb:Overlaps(aabb2))");
        script->Execute("aabb2:Set(Vector3(5), Vector3(6))");
        script->Execute("AZTestAssert( not aabb:Overlaps(aabb2))");

        ////Expand
        script->Execute("aabb:Set(Vector3(0), Vector3(2))");
        script->Execute("aabb:Expand(Vector3(1))");
        script->Execute("AZTestAssert(aabb:GetCenter():IsClose(Vector3(1)))");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(-1)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(3)))");

        ////GetExpanded
        script->Execute("aabb:Set(Vector3(-1), Vector3(1))");
        script->Execute("aabb = aabb:GetExpanded(Vector3(9))");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(-10)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(10)))");

        ////AddPoint
        script->Execute("aabb:Set(Vector3(-1), Vector3(1))");
        script->Execute("aabb:AddPoint(Vector3(1))");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(-1)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(1)))");
        script->Execute("aabb:AddPoint(Vector3(2))");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(-1)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(2)))");

        ////AddAabb
        script->Execute("aabb:Set(Vector3(0), Vector3(2))");
        script->Execute("aabb2:Set(Vector3(-2), Vector3(0))");
        script->Execute("aabb:AddAabb(aabb2)");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(-2)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(2)))");

        ////GetDistance
        script->Execute("aabb:Set(Vector3(-1), Vector3(1))");
        script->Execute("AZTestAssertFloatClose(aabb:GetDistance(Vector3(2, 0, 0)),1)");
        //// make sure a point inside the box returns zero, even if that point isn't the center.
        script->Execute("AZTestAssertFloatClose(aabb:GetDistance(Vector3(0.5, 0, 0)),0)");

        ////GetClamped
        script->Execute("aabb:Set(Vector3(0), Vector3(2))");
        script->Execute("aabb2:Set(Vector3(1), Vector3(4))");
        script->Execute("aabb = aabb:GetClamped(aabb2)");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(1)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(2)))");

        ////Clamp
        script->Execute("aabb:Set(Vector3(0), Vector3(2))");
        script->Execute("aabb2:Set(Vector3(-2), Vector3(1))");
        script->Execute("aabb:Clamp(aabb2)");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(0)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(1)))");

        ////SetNull
        script->Execute("aabb:SetNull()");
        script->Execute("AZTestAssert( not aabb:IsValid())");

        ////Translate
        script->Execute("aabb:Set(Vector3(-1), Vector3(1))");
        script->Execute("aabb:Translate(Vector3(2))");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(1)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(3)))");

        ////GetTranslated
        script->Execute("aabb:Set(Vector3(1), Vector3(3))");
        script->Execute("aabb = aabb:GetTranslated(Vector3(-2))");
        script->Execute("AZTestAssert(aabb.min:IsClose(Vector3(-1)))");
        script->Execute("AZTestAssert(aabb.max:IsClose(Vector3(1)))");

        ////GetSurfaceArea
        script->Execute("aabb:Set(Vector3(0), Vector3(1))");
        script->Execute("AZTestAssertFloatClose(aabb:GetSurfaceArea(), 6)");

        ////IsFinite
        script->Execute("infinity = math.huge");
        script->Execute("infiniteV3 = Vector3(infinity, infinity, infinity)");
        script->Execute("aabb:Set(Vector3(0),infiniteV3)");
        //// A bounding box is only invalid if the min is greater than the max.
        //// A bounding box with an infinite min or max is valid as long as this is true.
        script->Execute("AZTestAssert(aabb:IsValid())");
        script->Execute("AZTestAssert( not aabb:IsFinite())");
    }

    TEST_F(ScriptMathTest, LuaAabbTransformCompareTest)
    {
        // create aabb
        script->Execute("min = Vector3(-100,50,0)");
        script->Execute("max = Vector3(120,300,50)");
        script->Execute("aabb = Aabb.CreateFromMinMax(min,max)");

        // make the transformation matrix
        script->Execute("tm = Transform.CreateRotationX(1)");
        script->Execute("tm:SetTranslation(100,0,-50)");

        // transform
        script->Execute("obb = aabb:GetTransformedObb(tm)");
        script->Execute("transAabb = aabb:GetTransformedAabb(tm)");
        script->Execute("transAabb2 = Aabb.CreateFromObb(obb)");
        script->Execute("aabb:ApplyTransform(tm)");

        // compare the transformations
        script->Execute("AZTestAssert(transAabb.min:IsClose(transAabb2.min))");
        script->Execute("AZTestAssert(transAabb.max:IsClose(transAabb2.max))");
        script->Execute("AZTestAssert(aabb.min:IsClose(transAabb.min))");
        script->Execute("AZTestAssert(aabb.max:IsClose(transAabb.max))");
    }

    TEST_F(ScriptMathTest, LuaObbTest)
    {
        //CreateFromPositionRotationAndHalfLengths
        //GetPosition
        //GetAxisX
        //GetAxisY
        //GetAxisZ
        //GetAxis
        //GetHalfLengthX
        //GetHalfLengthY
        //GetHalfLengthZ
        //GetHalfLength

        script->Execute("position = Vector3(1, 2, 3)");
        script->Execute("rotation = Quaternion.CreateRotationZ(Math.DegToRad(45))");
        script->Execute("halfLengths = Vector3(0.5)");
        script->Execute("obb = Obb.CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths)");
        script->Execute("AZTestAssert(obb.position:IsClose(position))");
        script->Execute("AZTestAssert(obb:GetAxis(0):IsClose(Vector3(0.7071, 0.7071, 0.0)))");
        script->Execute("AZTestAssert(obb:GetAxis(1):IsClose(Vector3(-0.7071, 0.7071, 0.0)))");
        script->Execute("AZTestAssert(obb:GetAxis(2):IsClose(Vector3(0.0, 0.0, 1.0)))");
        script->Execute("AZTestAssert(obb.halfLengths:IsClose(halfLengths))");
        script->Execute("AZTestAssertFloatClose(obb:GetHalfLengthX(), 0.5)");
        script->Execute("AZTestAssertFloatClose(obb:GetHalfLengthY(), 0.5)");
        script->Execute("AZTestAssertFloatClose(obb:GetHalfLengthZ(), 0.5)");

        //CreateFromAabb
        script->Execute("min = Vector3(-100, 50, 0)");
        script->Execute("max = Vector3(120, 300, 50)");
        script->Execute("aabb = Aabb.CreateFromMinMax(min, max)");
        script->Execute("obb = Obb.CreateFromAabb(aabb)");
        script->Execute("AZTestAssert(obb.position:IsClose(Vector3(10, 175, 25)))");
        script->Execute("AZTestAssert(obb:GetAxisX():IsClose(Vector3(1, 0, 0)))");
        script->Execute("AZTestAssert(obb:GetAxisY():IsClose(Vector3(0, 1, 0)))");
        script->Execute("AZTestAssert(obb:GetAxisZ():IsClose(Vector3(0, 0, 1)))");

        // Transform * Obb
        script->Execute("obb = Obb.CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths)");
        script->Execute("transform = Transform.CreateRotationY(Math.DegToRad(90))");
        script->Execute("obb = transform * obb");
        script->Execute("AZTestAssert(obb.position:IsClose(Vector3(3, 2, -1)))");
        script->Execute("AZTestAssert(obb:GetAxisX():IsClose(Vector3(0.0, 0.7071, -0.7071)))");
        script->Execute("AZTestAssert(obb:GetAxisY():IsClose(Vector3(0.0, 0.7071, 0.7071)))");
        script->Execute("AZTestAssert(obb:GetAxisZ():IsClose(Vector3(1.0, 0.0, 0.0)))");

        //SetPosition
        //SetHalfLengthX
        //SetHalfLengthY
        //SetHalfLengthZ
        //SetHalfLength
        script->Execute("obb.position = position");
        script->Execute("AZTestAssert(obb.position:IsClose(position))");
        script->Execute("obb:SetHalfLengthX(2)");
        script->Execute("AZTestAssertFloatClose(obb:GetHalfLengthX(), 2)");
        script->Execute("obb:SetHalfLengthY(3)");
        script->Execute("AZTestAssertFloatClose(obb:GetHalfLengthY(), 3)");
        script->Execute("obb:SetHalfLengthZ(4)");
        script->Execute("AZTestAssertFloatClose(obb:GetHalfLengthZ(), 4)");
        script->Execute("obb:SetHalfLength(2, 5)");
        script->Execute("AZTestAssertFloatClose(obb:GetHalfLength(2), 5)");

        //IsFinite
        script->Execute("obb = Obb.CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths)");
        script->Execute("AZTestAssert(obb:IsFinite())");
        script->Execute("infinity = math.huge");
        script->Execute("infiniteV3 = Vector3(infinity, infinity, infinity)");
        // Test to make sure that setting properties of the bounding box
        // properly mark it as infinite, and when reset it becomes finite again.
        script->Execute("obb.position = infiniteV3");
        script->Execute("AZTestAssert( not obb:IsFinite())");
        script->Execute("obb.position = position");
        script->Execute("AZTestAssert(obb:IsFinite())");
        script->Execute("obb:SetHalfLengthX(infinity)");
        script->Execute("AZTestAssert( not obb:IsFinite())");
        script->Execute("obb:SetHalfLengthX(2.0)");
        script->Execute("AZTestAssert(obb:IsFinite())");
    }

    TEST_F(ScriptMathTest, LuaMathHelpersTest)
    {
        script->Execute(R"LUA(
            AZTestAssert(Math.IsClose(3.0, 3.0, FloatEpsilon))
            AZTestAssert(Math.IsClose(3.0, 3.0))
            AZTestAssert(Math.IsClose(3.20345678, 3.20345679))

            AZTestAssert(not Math.IsClose(3.0, 4.0, FloatEpsilon))
            AZTestAssert(not Math.IsClose(3.0, 4.0))

            AZTestAssert(Math.IsClose(1.0, 5.0, 10.0))
)LUA");
    }
}
