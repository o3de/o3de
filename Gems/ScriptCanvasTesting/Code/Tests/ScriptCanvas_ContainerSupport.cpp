/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
