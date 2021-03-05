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
#include <AzCore/ScriptCanvas/ScriptCanvasAttributes.h>
#include <AzCore/RTTI/AttributeReader.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvas; 

TEST_F(ScriptCanvasTestFixture, FillWithOrdinals)
{
    RunUnitTestGraph("LY_SC_UnitTest_FillWithOrdinals");
}

TEST_F(ScriptCanvasTestFixture, AZStdArray)
{
    // Enable when BE2.0 goes to main
    //RunUnitTestGraph("LY_SC_UnitTest_AZStdArray");
}

/*
TEST_F(ScriptCanvasTestFixture, ForEachNested)
{
    RunUnitTestGraph("LY_SC_UnitTest_ForEachNested");
}

TEST_F(ScriptCanvasTestFixture, ForEachNestedBreak)
{
    RunUnitTestGraph("LY_SC_UnitTest_ForEachNestedBreak");
}
*/

TEST_F(ScriptCanvasTestFixture, ForEachNode)
{
    // Enable when BE2.0 goes to main
   // RunUnitTestGraph("LY_SC_UnitTest_ForEachNode");
}

/*
TEST_F(ScriptCanvasTestFixture, ForEachBreak)
{
    RunUnitTestGraph("LY_SC_UnitTest_ForEachBreak");
}
*/

// TODO: needs to be recreated with new operator nodes
//TEST_F(ScriptCanvasTestFixture, VectorContainerVector3)
//{
//    RunUnitTestGraph("LY_SC_UnitTest_VectorContainerVector3");
//}

TEST_F(ScriptCanvasTestFixture, MapContainerStringVec3)
{
    // Enable when BE2.0 goes to main
    //RunUnitTestGraph("LY_SC_UnitTest_MapContainerStringVec3");
}

TEST_F(ScriptCanvasTestFixture, SetContainerNum)
{
    // Enable when BE2.0 goes to main
    //RunUnitTestGraph("LY_SC_UnitTest_SetContainerNum");
}
