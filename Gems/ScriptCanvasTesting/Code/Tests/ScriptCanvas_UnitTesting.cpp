/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/IdUtils.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Editor/Framework/ScriptCanvasReporter.h>


#if 0


using namespace ScriptCanvasEditor;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_EQ(LHS, RHS)\
    if (LHS == RHS)\
        ++m_countEQSucceeded;\
    else\
        ++m_countEQFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_NE(LHS, RHS)\
    if (LHS != RHS)\
        ++m_countNESucceeded;\
    else\
        ++m_countNEFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_GT(LHS, RHS)\
    if (LHS > RHS)\
        ++m_countGTSucceeded;\
    else\
        ++m_countGTFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_GE(LHS, RHS)\
    if (LHS >= RHS)\
        ++m_countGESucceeded;\
    else\
        ++m_countGEFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_LT(LHS, RHS)\
    if (LHS < RHS)\
        ++m_countLTSucceeded;\
    else\
        ++m_countLTFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_LE(LHS, RHS)\
    if (LHS <= RHS)\
        ++m_countLESucceeded;\
    else\
        ++m_countLEFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_GT(LHS, RHS)\
    if((LHS.IsGreaterThan(RHS))) ++m_countGTSucceeded; else ++m_countGTFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_GE(LHS, RHS)\
    if((LHS.IsGreaterEqualThan(RHS))) ++m_countGESucceeded; else ++m_countGEFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_LT(LHS, RHS)\
    if((LHS.IsLessThan(RHS))) ++m_countLTSucceeded; else ++m_countLTFailed;

#define SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_LE(LHS, RHS)\
    if((LHS.IsLessEqualThan(RHS))) ++m_countLTSucceeded; else ++m_countLTFailed;


namespace ScriptCanvas_UnitTestingCPP
{
    const double k_tolerance = 0.01;
    const char* k_defaultExtension = "scriptcanvas";
    const char* k_unitTestDirPathRelative = "@engroot@/Gems/ScriptCanvasTesting/Assets/ScriptCanvas/UnitTests";
}

using namespace ScriptCanvasTests;
using namespace ScriptCanvas;
using namespace ScriptCanvas::UnitTesting;
using namespace ScriptCanvasEditor;

class MetaReporter
    : public Reporter
{
public:
    AZ_INLINE AZ::u32 GetCountEQFailed() const { return m_countEQFailed; }
    AZ_INLINE AZ::u32 GetCountEQSucceeded() const { return m_countEQSucceeded; }
    AZ_INLINE AZ::u32 GetCountFalseFailed() const { return m_countFalseFailed; }
    AZ_INLINE AZ::u32 GetCountFalseSucceeded() const { return m_countFalseSucceeded; }
    AZ_INLINE AZ::u32 GetCountGEFailed() const { return m_countGEFailed; }
    AZ_INLINE AZ::u32 GetCountGESucceeded() const { return m_countGESucceeded; }
    AZ_INLINE AZ::u32 GetCountGTFailed() const { return m_countGTFailed; }
    AZ_INLINE AZ::u32 GetCountGTSucceeded() const { return m_countGTSucceeded; }
    AZ_INLINE AZ::u32 GetCountLEFailed() const { return m_countLEFailed; }
    AZ_INLINE AZ::u32 GetCountLESucceeded() const { return m_countLESucceeded; }
    AZ_INLINE AZ::u32 GetCountLTFailed() const { return m_countLTFailed; }
    AZ_INLINE AZ::u32 GetCountLTSucceeded() const { return m_countLTSucceeded; }
    AZ_INLINE AZ::u32 GetCountNEFailed() const { return m_countNEFailed; }
    AZ_INLINE AZ::u32 GetCountNESucceeded() const { return m_countNESucceeded; }
    AZ_INLINE AZ::u32 GetCountTrueFailed() const { return m_countTrueFailed; }
    AZ_INLINE AZ::u32 GetCountTrueSucceeded() const { return m_countTrueSucceeded; }

    bool operator==(const MetaReporter& reporter) const;

    void ExpectFalse(const bool value, const Report& report) override;

    void ExpectTrue(const bool value, const Report& report) override;

    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_OVERRIDES(ExpectEqual);

    SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_OVERRIDES(ExpectNotEqual);

    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectGreaterThan);

    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectGreaterThanEqual);

    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectLessThan);

    SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_OVERRIDES(ExpectLessThanEqual);

private:
    AZ::u32 m_countEQFailed = 0;
    AZ::u32 m_countEQSucceeded = 0;
    AZ::u32 m_countFalseFailed = 0;
    AZ::u32 m_countFalseSucceeded = 0;
    AZ::u32 m_countGEFailed = 0;
    AZ::u32 m_countGESucceeded = 0;
    AZ::u32 m_countGTFailed = 0;
    AZ::u32 m_countGTSucceeded = 0;
    AZ::u32 m_countLEFailed = 0;
    AZ::u32 m_countLESucceeded = 0;
    AZ::u32 m_countLTFailed = 0;
    AZ::u32 m_countLTSucceeded = 0;
    AZ::u32 m_countNEFailed = 0;
    AZ::u32 m_countNESucceeded = 0;
    AZ::u32 m_countTrueFailed = 0;
    AZ::u32 m_countTrueSucceeded = 0;
}; // class MetaReporter

bool MetaReporter::operator==(const MetaReporter& other) const
{
    return m_countEQFailed == other.m_countEQFailed
        && m_countEQSucceeded == other.m_countEQSucceeded
        && m_countFalseFailed == other.m_countFalseFailed
        && m_countFalseSucceeded == other.m_countFalseSucceeded
        && m_countGEFailed == other.m_countGEFailed
        && m_countGESucceeded == other.m_countGESucceeded
        && m_countGTFailed == other.m_countGTFailed
        && m_countGTSucceeded == other.m_countGTSucceeded
        && m_countLEFailed == other.m_countLEFailed
        && m_countLESucceeded == other.m_countLESucceeded
        && m_countLTFailed == other.m_countLTFailed
        && m_countLTSucceeded == other.m_countLTSucceeded
        && m_countNEFailed == other.m_countNEFailed
        && m_countNESucceeded == other.m_countNESucceeded
        && m_countTrueSucceeded == other.m_countTrueSucceeded
        && m_countTrueFailed == other.m_countTrueFailed
        && *static_cast<const Reporter*>(this) == *static_cast<const Reporter*>(&other);
}

// Handler
void MetaReporter::ExpectFalse(const bool value, const Report&)
{
    if (!value)
        ++m_countFalseSucceeded;
    else 
        ++m_countFalseFailed;
}

void MetaReporter::ExpectTrue(const bool value, const Report&)
{
    if (value)
        ++m_countTrueSucceeded;
    else
        ++m_countTrueFailed;
}

void MetaReporter::ExpectEqual(const Data::NumberType lhs, const Data::NumberType rhs, const Report&)
{
    if (AZ::IsClose(lhs, rhs, ScriptCanvas_UnitTestingCPP::k_tolerance))
        ++m_countEQSucceeded;
    else
        ++m_countEQFailed;
}

void MetaReporter::ExpectNotEqual(const Data::NumberType lhs, const Data::NumberType rhs, const Report&)
{
    if (!AZ::IsClose(lhs, rhs, ScriptCanvas_UnitTestingCPP::k_tolerance))
        ++m_countNESucceeded;
    else
        ++m_countNEFailed;
}

SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectEqual, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_EQ)

SCRIPT_CANVAS_UNIT_TEST_EQUALITY_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectNotEqual, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_NE)

SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectGreaterThan, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_GT)

SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectGreaterThanEqual, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_GE)

SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectLessThan, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_LT)

SCRIPT_CANVAS_UNIT_TEST_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectLessThanEqual, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_EXPECT_LE)

// Vector Implementations
SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectGreaterThan, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_GT)
SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectGreaterThanEqual, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_GE)
SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectLessThan, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_LT)
SCRIPT_CANVAS_UNIT_TEST_VECTOR_COMPARE_OVERLOAD_IMPLEMENTATIONS(MetaReporter, ExpectLessThanEqual, SCRIPT_CANVAS_UNIT_TEST_META_REPORTER_VECTOR_EXPECT_LE)

MetaReporter MetaRunUnitTestGraph(AZStd::string_view path)
{
    MetaReporter interpretedReporter;
    const AZStd::string filePath = AZStd::string::format("%s/%s.%s", ScriptCanvas_UnitTestingCPP::k_unitTestDirPathRelative, path.data(), ScriptCanvas_UnitTestingCPP::k_defaultExtension);

    RunGraphSpec runGraphSpec;
    runGraphSpec.graphPath = filePath;
    runGraphSpec.dirPath = ScriptCanvas_UnitTestingCPP::k_unitTestDirPathRelative;
    runGraphSpec.runSpec.execution = ExecutionMode::Interpreted;
    ScriptCanvasEditor::RunGraphImplementation(runGraphSpec, interpretedReporter);
    EXPECT_TRUE(interpretedReporter.IsReportFinished());
    return interpretedReporter;
}

//////////////////////////////////////////////////////////////////////////
// if this test doesn't pass, our fixture is broken, and our unit tests are meaningless
//////////////////////////////////////////////////////////////////////////

TEST_F(ScriptCanvasTestFixture, FixtureSanity)
{
    SUCCEED();
}

//////////////////////////////////////////////////////////////////////////
// if these tests do not pass, our SC unit test framework is broken, and such tests are meaningless
//////////////////////////////////////////////////////////////////////////

TEST_F(ScriptCanvasTestFixture, AddFailure)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_AddFailure");
    
    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }
    
    EXPECT_EQ(reporter.GetFailure().size(), 3);
    
    if (reporter.GetFailure().size() == 3)
    {
        EXPECT_EQ(reporter.GetFailure()[0], "zero");
        EXPECT_EQ(reporter.GetFailure()[1], "one");
        EXPECT_EQ(reporter.GetFailure()[2], "two");
    }
    
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_FALSE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, AddSuccess)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_AddSuccess");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetSuccess().size(), 3);
    
    if (reporter.GetFailure().size() == 3)
    {
        EXPECT_EQ(reporter.GetSuccess()[0], "zero");
        EXPECT_EQ(reporter.GetSuccess()[1], "one");
        EXPECT_EQ(reporter.GetSuccess()[2], "two");
    }
    
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectTrueFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectTrueFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountTrueSucceeded(), 0);
    EXPECT_EQ(reporter.GetCountTrueFailed(), 1);
    EXPECT_TRUE(reporter.IsErrorFree());
}
 
TEST_F(ScriptCanvasTestFixture, ExpectTrueSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectTrueSucceed");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountTrueSucceeded(), 1);
    EXPECT_EQ(reporter.GetCountTrueFailed(), 0);
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectEqualFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectEqualFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountEQFailed(), 1);
    EXPECT_EQ(reporter.GetCountEQSucceeded(), 0);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectEqualSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectEqualSucceed");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountEQFailed(), 0);
    EXPECT_EQ(reporter.GetCountEQSucceeded(), 1);   
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectNotEqualFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectNotEqualFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountNEFailed(), 1);
    EXPECT_EQ(reporter.GetCountNESucceeded(), 0);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectNotEqualSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectNotEqualSucceed");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountNEFailed(), 0);
    EXPECT_EQ(reporter.GetCountNESucceeded(), 1);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, MarkCompleteFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_MarkCompleteFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountTrueSucceeded(), 1);
    EXPECT_FALSE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, MarkCompleteSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_MarkCompleteSucceed");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountTrueSucceeded(), 1);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectGreaterThanFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectGreaterThanFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountGTFailed(), 1);
    EXPECT_EQ(reporter.GetCountGTSucceeded(), 0);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectGreaterThanSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectGreaterThanSucceed");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountGTFailed(), 0);
    EXPECT_EQ(reporter.GetCountGTSucceeded(), 1);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectGreaterThanEqualFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectGreaterThanEqualFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountGEFailed(), 1);
    EXPECT_EQ(reporter.GetCountGESucceeded(), 0);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectGreaterThanEqualSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectGreaterThanEqualSucceed");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountGEFailed(), 0);
    EXPECT_EQ(reporter.GetCountGESucceeded(), 2);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture,     ExpectLessThanFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectLessThanFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountLTFailed(), 1);
    EXPECT_EQ(reporter.GetCountLTSucceeded(), 0);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectLessThanSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectLessThanSucceed");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountLTFailed(), 0);
    EXPECT_EQ(reporter.GetCountLTSucceeded(), 1);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectLessThanEqualFail)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectLessThanEqualFail");

    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountLEFailed(), 1);
    EXPECT_EQ(reporter.GetCountLESucceeded(), 0);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}

TEST_F(ScriptCanvasTestFixture, ExpectLessThanEqualSucceed)
{
    MetaReporter reporter = MetaRunUnitTestGraph("LY_SC_UnitTest_Meta_ExpectLessThanEqualSucceed");
    
    if (!reporter.GetScriptCanvasId().IsValid())
    {
        ADD_FAILURE() << "Graph is not valid";
        return;
    }

    EXPECT_EQ(reporter.GetCountLEFailed(), 0);
    EXPECT_EQ(reporter.GetCountLESucceeded(), 2);
    EXPECT_TRUE(reporter.IsComplete());
    EXPECT_TRUE(reporter.IsDeactivated());
    EXPECT_TRUE(reporter.IsErrorFree());
}



#endif
