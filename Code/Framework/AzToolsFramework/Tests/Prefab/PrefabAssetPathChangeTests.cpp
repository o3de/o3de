/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Prefab/PrefabAssetPathChangeTestFixture.h>

namespace UnitTest
{
    using PrefabAssetPathChangeTests = PrefabAssetPathChangeTestFixture;

    TEST_F(PrefabAssetPathChangeTests, ChangePrefabFileName)
    {
        const AZStd::string prefabFolderPath = "";
        const AZStd::string prefabFileName = "Prefab.prefab";
        const AZStd::string newPrefabFileName = "PrefabRenamed.prefab";

        InstanceOptionalReference prefabInstance = CreatePrefabInstance(prefabFolderPath, prefabFileName);
        ASSERT_TRUE(prefabInstance.has_value());

        AZ::IO::Path originalTemplateSourcePath = prefabInstance->get().GetTemplateSourcePath();
        EXPECT_EQ(originalTemplateSourcePath.Native(), prefabFileName);

        // Rename prefab file
        ChangePrefabFileName(prefabFolderPath, prefabFileName, newPrefabFileName);

        AZ::IO::Path newTemplateSourcePath = prefabInstance->get().GetTemplateSourcePath();
        EXPECT_EQ(newTemplateSourcePath.Native(), newPrefabFileName);

        EXPECT_NE(originalTemplateSourcePath, newTemplateSourcePath);
    }

    TEST_F(PrefabAssetPathChangeTests, ChangeFolderName)
    {
        const AZStd::string prefabFileName = "Prefab.prefab";
        const AZStd::string prefabFolderPath = "PrefabFolder";
        const AZStd::string newPrefabFolderPath = "PrefabFolderRenamed";

        InstanceOptionalReference prefabInstance = CreatePrefabInstance(prefabFolderPath, prefabFileName);
        ASSERT_TRUE(prefabInstance.has_value());

        AZ::IO::Path originalTemplateSourcePath = prefabInstance->get().GetTemplateSourcePath();
        EXPECT_EQ(originalTemplateSourcePath, GetPrefabFilePathForSerialization(prefabFolderPath, prefabFileName));

        ChangePrefabFolderPath(prefabFolderPath, newPrefabFolderPath);

        AZ::IO::Path newTemplateSourcePath = prefabInstance->get().GetTemplateSourcePath();
        EXPECT_EQ(newTemplateSourcePath, GetPrefabFilePathForSerialization(newPrefabFolderPath, prefabFileName));

        EXPECT_NE(originalTemplateSourcePath, newTemplateSourcePath);
    }

    TEST_F(PrefabAssetPathChangeTests, ChangeAncestorFolderName)
    {
        const AZStd::string prefabFileName = "Prefab.prefab";
        const AZStd::string prefabBaseFolder = "PrefabFolder";
        const AZStd::string newPrefabBaseFolder = "PrefabFolderRenamed";
        const AZStd::string prefabFolderPath = "PrefabFolder/PrefabSubfolder";
        const AZStd::string newPrefabFolderPath = "PrefabFolderRenamed/PrefabSubfolder";

        InstanceOptionalReference prefabInstance = CreatePrefabInstance(prefabFolderPath, prefabFileName);
        ASSERT_TRUE(prefabInstance.has_value());

        AZ::IO::Path originalTemplateSourcePath = prefabInstance->get().GetTemplateSourcePath();
        EXPECT_EQ(originalTemplateSourcePath, GetPrefabFilePathForSerialization(prefabFolderPath, prefabFileName));

        ChangePrefabFolderPath(prefabBaseFolder, newPrefabBaseFolder);

        AZ::IO::Path newTemplateSourcePath = prefabInstance->get().GetTemplateSourcePath();
        EXPECT_EQ(newTemplateSourcePath, GetPrefabFilePathForSerialization(newPrefabFolderPath, prefabFileName));

        EXPECT_NE(originalTemplateSourcePath, newTemplateSourcePath);
    }

    TEST_F(PrefabAssetPathChangeTests, MoveFolderWithMultiplePrefabsToAncestor)
    {
        const AZStd::string prefab1FileName = "Prefab1.prefab";
        const AZStd::string prefab2FileName = "Prefab2.prefab";
        const AZStd::string prefabFolderPath = "PrefabsFolder/PrefabsSubfolder";
        const AZStd::string newPrefabFolderPath = "PrefabsSubfolder";

        InstanceOptionalReference prefabInstance1 = CreatePrefabInstance(prefabFolderPath, prefab1FileName);
        ASSERT_TRUE(prefabInstance1.has_value());
        InstanceOptionalReference prefabInstance2 = CreatePrefabInstance(prefabFolderPath, prefab2FileName);
        ASSERT_TRUE(prefabInstance2.has_value());

        ChangePrefabFolderPath(prefabFolderPath, newPrefabFolderPath);

        AZ::IO::Path expectedInstance1Path = GetPrefabFilePathForSerialization(newPrefabFolderPath, prefab1FileName);
        AZ::IO::Path expectedInstance2Path = GetPrefabFilePathForSerialization(newPrefabFolderPath, prefab2FileName);
        EXPECT_EQ(prefabInstance1->get().GetTemplateSourcePath(), expectedInstance1Path);
        EXPECT_EQ(prefabInstance2->get().GetTemplateSourcePath(), expectedInstance2Path);
    }
} // namespace UnitTest
