/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <QtTest>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    TEST_F(UIFixture, FileManager_SaveSourceAsset)
    {
        EMStudio::FileManager* fileManager = EMStudio::GetMainWindow()->GetFileManager();
        const char* filename = "C:/MyAsset.txt";

        // 1. Save the asset.
        EXPECT_FALSE(fileManager->DidSourceAssetGetSaved(filename)) << "Source asset has not been saved yet.";
        fileManager->SourceAssetChanged(filename); // Called after saving the asset.
        fileManager->SourceAssetChanged(filename); // Call it another time to imitate something wrong.

        // 2. Auto-reload callback triggers.
        EXPECT_TRUE(fileManager->DidSourceAssetGetSaved(filename)) << "Source asset should have been saved previously.";
        fileManager->RemoveFromSavedSourceAssets(filename); // Callback removes it from the list.
        EXPECT_FALSE(fileManager->DidSourceAssetGetSaved(filename)) << "As we handled and removed it already, it should not be in the list anymore.";
    }
} // namespace EMotionFX
