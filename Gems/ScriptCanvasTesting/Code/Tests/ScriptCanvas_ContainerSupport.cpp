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

TEST_F(ScriptCanvasTestFixture, AZStdArray)
{
    RunUnitTestGraph("LY_SC_UnitTest_AZStdArray", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, ForEachNode)
{
    RunUnitTestGraph("LY_SC_UnitTest_ForEachNode", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, MapContainerStringVec3)
{
    RunUnitTestGraph("LY_SC_UnitTest_MapContainerStringVec3", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, VectorContainerVector3)
{
    RunUnitTestGraph("LY_SC_UnitTest_VectorContainerVector3", ExecutionMode::Interpreted);
}
