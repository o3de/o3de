/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzTest/AzTest.h>
#include <AzFramework/Engine/Engine.h>
#include <Utils/Utils.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace EngineTests
{

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class EngineTest : public ::testing::Test
    {
    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        void SetUp() override
        {

        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void TearDown() override
        {

        }

    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(EngineTest, FindEngineRoot_EngineBelowExecutable_Success)
    {
        // We assume in this test that our engine lives above our binaries.  If that's not the case we should pass in
        // a valid path to a folder below the engine root to verify this functionality
        auto foundRoot = AzFramework::Engine::FindEngineRoot();
        EXPECT_FALSE(foundRoot.empty());
    }

#if AZ_TRAIT_DISABLE_FAILED_FRAMEWORK_TESTS
    TEST_F(EngineTest, DISABLED_FindEngineRoot_StartTempFolder_NoRootFound)
#else
    TEST_F(EngineTest, FindEngineRoot_StartTempFolder_NoRootFound)
#endif // AZ_TRAIT_DISABLE_FAILED_FRAMEWORK_TESTS
    {
        UnitTest::ScopedTemporaryDirectory tempDir;
        auto foundRoot = AzFramework::Engine::FindEngineRoot(tempDir.GetDirectory());
        EXPECT_TRUE(foundRoot.empty());
    }

} // namespace EngineTests
