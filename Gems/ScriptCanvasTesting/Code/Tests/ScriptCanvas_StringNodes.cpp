/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvas;
  
TEST_F(ScriptCanvasTestFixture, StringNodes)
{
    RunUnitTestGraph("LY_SC_UnitTest_StringNodes", ExecutionMode::Interpreted);
}
