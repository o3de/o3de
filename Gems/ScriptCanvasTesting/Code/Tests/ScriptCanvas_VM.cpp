/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/MathReflection.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Source/ScriptCanvasTestBus.h>
#include <ScriptCanvas/Translation/GraphToLuaUtility.h>

using namespace ScriptCanvas;
using namespace ScriptCanvasTests;
using namespace TestNodes;
using namespace ScriptCanvas::Execution;

class Grandparent
{
public:
    AZ_RTTI(Grandparent, "{76EF13EE-7F5E-41C8-A789-A86836D66D10}");
    AZ_CLASS_ALLOCATOR(Grandparent, AZ::SystemAllocator);

    virtual ~Grandparent() = default;

    static void Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<Grandparent>();
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<Grandparent>("Grandparent")
                ->Method("GetGeneration", &Grandparent::GetGeneration);
            ;
        }
    }

    virtual int GetGeneration() const
    {
        return 1;
    }
};

class Parent
    : public Grandparent
{
public:
    AZ_RTTI(Parent, "{2ABA91B7-24F7-495A-ACC6-4F93DE47B507}", Grandparent);
    AZ_CLASS_ALLOCATOR(Parent, AZ::SystemAllocator);

    ~Parent() override = default;

    static void Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<Parent, Grandparent>();
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<Parent>("Parent")
                ;
        }
    }

    int GetGeneration() const override
    {
        return 2;
    }
};

class Child
    : public Parent
{
public:
    AZ_RTTI(Child, "{826DB77C-11B7-42C4-8F3F-3438AFE5B29B}", Parent);
    AZ_CLASS_ALLOCATOR(Parent, AZ::SystemAllocator);

    ~Child() override = default;

    static void Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<Child, Parent>();
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<Child>("Child")
                ;
        }
    }

    int GetGeneration() const override
    {
        return 3;
    }
};

// TEST_F(ScriptCanvasTestFixture, CreateUuidsFast)
// {
//     using namespace ScriptCanvas::Execution;
// 
//     ScriptCanvasEditor::ScopedOutputSuppression outputSuppression;
// 
//     AZ::Uuid candidate;
//     AZ::Uuid reference;
//     AZStd::string candidateString;
//     AZStd::string referenceString;
// 
//     candidateString = /**************/ "0123ABCD4567EFAB0123ABCD4567EFAB";
//     candidate = CreateIdFromStringFast(candidateString.data());
//     reference = AZ::Uuid::CreateString("0123abcd4567efab0123abcd4567efab");
//     EXPECT_EQ(candidate, reference);
//     referenceString = CreateStringFastFromId(candidate);
//     EXPECT_EQ(candidateString, referenceString);
// 
//     candidateString = /**************/ "12345678909876543345676524676553";
//     candidate = CreateIdFromStringFast(candidateString.data());
//     reference = AZ::Uuid::CreateString("12345678909876543345676524676553");
//     EXPECT_EQ(candidate, reference);
//     referenceString = CreateStringFastFromId(candidate);
//     EXPECT_EQ(candidateString, referenceString);
// 
//     candidateString = /**************/ "ABCDEFABCDEFABCDEFABCDEFABCDEFAB";
//     candidate = CreateIdFromStringFast(candidateString.data());
//     reference = AZ::Uuid::CreateString("abcdefabcdefabcdefabcdefabcdefab");
//     EXPECT_EQ(candidate, reference);
//     referenceString = CreateStringFastFromId(candidate);
//     EXPECT_EQ(candidateString, referenceString);
// 
//     candidateString = /**************/ "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF";
//     candidate = CreateIdFromStringFast(candidateString.data());
//     reference = AZ::Uuid::CreateString("ffffffffffffffffffffffffffffffff");
//     EXPECT_EQ(candidate, reference);
//     referenceString = CreateStringFastFromId(candidate);
//     EXPECT_EQ(candidateString, referenceString);
// 
//     candidateString = /**************/ "00000000000000000000000000000000";
//     candidate = CreateIdFromStringFast(candidateString.data());
//     reference = AZ::Uuid::CreateString("00000000000000000000000000000000");
//     EXPECT_EQ(candidate, reference);
//     referenceString = CreateStringFastFromId(candidate);
//     EXPECT_EQ(candidateString, referenceString);
// 
//     candidateString = /**************/ "00000000000000000000000000000001";
//     candidate = CreateIdFromStringFast(candidateString.data());
//     reference = AZ::Uuid::CreateString("00000000000000000000000000000001");
//     EXPECT_EQ(candidate, reference);
//     referenceString = CreateStringFastFromId(candidate);
//     EXPECT_EQ(candidateString, referenceString);
// 
//     candidateString = /**************/ "80000000000000000000000000000000";
//     candidate = CreateIdFromStringFast(candidateString.data());
//     reference = AZ::Uuid::CreateString("80000000000000000000000000000000");
//     EXPECT_EQ(candidate, reference);
//     referenceString = CreateStringFastFromId(candidate);
//     EXPECT_EQ(candidateString, referenceString);
// 
//     // ................................."0123ABCD4567EFAB0123ABCD4567EFAB"
//     EXPECT_DEATH(CreateIdFromStringFast("0123ABCD4567EFAB0123ABCD4567EFA"), "");
//     EXPECT_DEATH(CreateIdFromStringFast("0123ABCD4567EFA"), "");
//     EXPECT_DEATH(CreateIdFromStringFast("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM"), "");
//     EXPECT_DEATH(CreateIdFromStringFast("0123ABCD4567EFAB0123ABCD4567EFAB0"), "");
// }

TEST_F(ScriptCanvasTestFixture, InterpretedHelloWorld)
{
    RunUnitTestGraph("LY_SC_UnitTest_HelloWorld");
}

TEST_F(ScriptCanvasTestFixture, TypeInheritance)
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Grammar;

    // \todo make a version of this to test in the editor for node connections
    // \todo make a version of this using events to check the methods results using self and table explicit calls

    Grandparent::Reflect(m_serializeContext);
    Grandparent::Reflect(m_behaviorContext);
    Parent::Reflect(m_serializeContext);
    Parent::Reflect(m_behaviorContext);
    Child::Reflect(m_serializeContext);
    Child::Reflect(m_behaviorContext);

    Data::Type grandparentType = Data::Type::BehaviorContextObject(azrtti_typeid<Grandparent>());
    Data::Type grandparentType2 = Data::Type::BehaviorContextObject(azrtti_typeid<Grandparent>());
    Data::Type parentType = Data::Type::BehaviorContextObject(azrtti_typeid<Parent>());
    Data::Type parentType2 = Data::Type::BehaviorContextObject(azrtti_typeid<Parent>());
    Data::Type childType = Data::Type::BehaviorContextObject(azrtti_typeid<Child>());
    Data::Type childType2 = Data::Type::BehaviorContextObject(azrtti_typeid<Child>());

    EXPECT_TRUE(grandparentType.IS_A(grandparentType2));
    EXPECT_TRUE(grandparentType.IS_EXACTLY_A(grandparentType2));
    EXPECT_FALSE(grandparentType.IS_A(parentType));
    EXPECT_FALSE(grandparentType.IS_EXACTLY_A(parentType));
    EXPECT_FALSE(grandparentType.IS_A(childType));
    EXPECT_FALSE(grandparentType.IS_EXACTLY_A(childType));

    EXPECT_TRUE(parentType.IS_A(grandparentType));
    EXPECT_FALSE(parentType.IS_EXACTLY_A(grandparentType));
    EXPECT_TRUE(parentType.IS_A(parentType2));
    EXPECT_TRUE(parentType.IS_EXACTLY_A(parentType2));
    EXPECT_FALSE(parentType.IS_A(childType));
    EXPECT_FALSE(parentType.IS_EXACTLY_A(childType));

    EXPECT_TRUE(childType.IS_A(grandparentType));
    EXPECT_FALSE(childType.IS_EXACTLY_A(grandparentType));
    EXPECT_TRUE(childType.IS_A(parentType));
    EXPECT_FALSE(childType.IS_EXACTLY_A(parentType));
    EXPECT_TRUE(childType.IS_A(childType2));
    EXPECT_TRUE(childType.IS_EXACTLY_A(childType2));
}

TEST_F(ScriptCanvasTestFixture, TestLuaStringHandling)
{
    AZ::ScriptContext sc;
    AZStd::string testingStrings[11] = {
        "test",
        "test]",
        "[test]",
        "=[test]=",
        "[=[test]=]",
        "[==[=[test]=]==]",
        "==[=[test]=]==",
        "test]=",
        "[=[==[test]==]=]",
        "[===[=[==[test]==]=]===]",
        "\"\\/.'].["
    };

    for (AZStd::string item : testingStrings)
    {
        AZStd::string safeStringLiteral = Translation::MakeRuntimeSafeStringLiteral(item);
        EXPECT_TRUE(sc.Execute(AZStd::string::format("print(%s)", safeStringLiteral.c_str()).c_str()));
    }
}

// \todo turn this into a unit test nodeable that adds a unit test failure on destruction if it was never triggered (or triggered the required number of times)
class Marker
{
public:
    AZ_TYPE_INFO(Marker, "{BEEB4BF4-81B8-45A0-AD3F-D1875703315B}");
    AZ_CLASS_ALLOCATOR(Marker, AZ::SystemAllocator);

    static AZStd::vector<int> s_markedPositions;
    
    static void MarkPosition(int mark)
    {
        s_markedPositions.push_back(mark);
    }

    static void Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflectContext))
        {
            behaviorContext->Class<Marker>("Marker")->Method("MarkPosition", &MarkPosition);
        }
    }
};

AZStd::vector<int> Marker::s_markedPositions;
