/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabSaveLoadHandler.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    using AzToolsFramework::Prefab::PrefabSaveHandler;

    TEST(PrefabSaveLoadHandlerTests, IsPrefabSourcePathAcceptsOnlyPrefabExtensions)
    {
        EXPECT_TRUE(PrefabSaveHandler::IsPrefabSourcePath("Levels/Test/Test.prefab"));
        EXPECT_TRUE(PrefabSaveHandler::IsPrefabSourcePath("Test.PREFAB"));
        EXPECT_FALSE(PrefabSaveHandler::IsPrefabSourcePath("Levels/Test/Test.spawnable"));
        EXPECT_FALSE(PrefabSaveHandler::IsPrefabSourcePath("Test.prefab.bak"));
        EXPECT_FALSE(PrefabSaveHandler::IsPrefabSourcePath(""));
    }
} // namespace UnitTest
