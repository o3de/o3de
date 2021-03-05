/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affililates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvas;
 
TEST_F(ScriptCanvasTestFixture, OutcomeFailureVE)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeFailureVE");
}

TEST_F(ScriptCanvasTestFixture, OutcomeFailureV)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeFailureV");
}

TEST_F(ScriptCanvasTestFixture, OutcomeFailureE)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeFailureE");
}

TEST_F(ScriptCanvasTestFixture, OutcomeFailure)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeFailure");
}

TEST_F(ScriptCanvasTestFixture, OutcomeSuccessVE)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeSuccessVE");
}

TEST_F(ScriptCanvasTestFixture, OutcomeSuccessV)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeSuccessV");
}

TEST_F(ScriptCanvasTestFixture, OutcomeSuccessE)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeSuccessE");
}

TEST_F(ScriptCanvasTestFixture, OutcomeSuccess)
{
    RunUnitTestGraph("LY_SC_UnitTest_OutcomeSuccess");
}