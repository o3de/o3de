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

TEST_F(ScriptCanvasTestFixture, StringMethodCStyle2CStyle)
{
    RunUnitTestGraph("LY_SC_UnitTest_StringMethodCStyle2CStyle", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, EBusStringResultCStyle2CStyle)
{
    RunUnitTestGraph("LY_SC_UnitTest_EBusStringResultCStyle2CStyle", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, EBusStringResultCStyle2String)
{
    RunUnitTestGraph("LY_SC_UnitTest_EBusStringResultCStyle2String", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, EBusStringResultCStyle2StringView)
{
    RunUnitTestGraph("LY_SC_UnitTest_EBusStringResultCStyle2StringView", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, EBusResultNested)
{
    RunUnitTestGraph("LY_SC_UnitTest_EBusResultNested", ExecutionMode::Interpreted);
}
