/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>

#include <AzCore/Math/Vector3.h>
#include <ScriptCanvas/Libraries/Math/Math.h>
#include <ScriptCanvas/Data/Data.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvas;

TEST_F(ScriptCanvasTestFixture, MathOperations_Graph)
{
    RunUnitTestGraph("LY_SC_UnitTest_MathOperations", ExecutionMode::Interpreted);
}

TEST_F(ScriptCanvasTestFixture, MathCustom_Graph)
{
    RunUnitTestGraph("LY_SC_UnitTest_UnitTest_MathCustom", ExecutionMode::Interpreted);
}
