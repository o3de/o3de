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
    void PrefabAssetPathChangeTestFixture::SetUpEditorFixtureImpl()
    {
        PrefabTestFixture::SetUpEditorFixtureImpl();

        m_settingsRegistryInterface->Get(m_projectPath, AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath);
    }

    InstanceOptionalReference PrefabAssetPathChangeTestFixture::CreatePrefabInstance(
        AZStd::string_view folderPath,
        AZStd::string_view fileName)
    {
        const AZStd::string entityName = "Entity1";

        // Create and instantiate a prefab
        AZ::IO::Path prefabFilepath = GetAbsoluteFilePathName(folderPath, fileName);
        AZ::EntityId editorEntityId = CreateEditorEntityUnderRoot(entityName);
        AZ::EntityId containerId = CreateEditorPrefab(prefabFilepath, { editorEntityId });
        InstanceOptionalReference prefabInstance = m_instanceEntityMapperInterface->FindOwningInstance(containerId);

        return prefabInstance;
    }

    void PrefabAssetPathChangeTestFixture::ChangePrefabFileName(
        AZStd::string_view folderPath,
        AZStd::string_view fromFileName,
        AZStd::string_view toFileName)
    {
        AZ::IO::Path fromAbsoluteFilePath = GetAbsoluteFilePathName(folderPath, fromFileName);
        AZ::IO::Path toAbsoluteFilePath = GetAbsoluteFilePathName(folderPath, toFileName);

        SendFilePathNameChangeEvent(fromAbsoluteFilePath.c_str(), toAbsoluteFilePath.c_str());
        PropagateAllTemplateChanges();
    }

    void PrefabAssetPathChangeTestFixture::ChangePrefabFolderPath(AZStd::string_view fromFolderPath, AZStd::string_view toFolderPath)
    {
        AZ::IO::Path fromAbsoluteFolderPath = GetAbsoluteFilePathName(fromFolderPath, "");
        AZ::IO::Path toAbsoluteFolderPath = GetAbsoluteFilePathName(toFolderPath, "");

        SendFolderPathNameChangeEvent(fromAbsoluteFolderPath.c_str(), toAbsoluteFolderPath.c_str());
        PropagateAllTemplateChanges();
    }

    void PrefabAssetPathChangeTestFixture::SendFolderPathNameChangeEvent(AZStd::string_view fromPath, AZStd::string_view toPath)
    {
        AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotificationBus::Events::OnSourceFolderPathNameChanged,
            fromPath,
            toPath);
    }

    void PrefabAssetPathChangeTestFixture::SendFilePathNameChangeEvent(AZStd::string_view fromPath, AZStd::string_view toPath)
    {
        AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotificationBus::Events::OnSourceFilePathNameChanged,
            fromPath,
            toPath);
    }

    AZ::IO::Path PrefabAssetPathChangeTestFixture::GetAbsoluteFilePathName(AZStd::string_view folderPath, AZStd::string_view fileName)
    {
        AZ::IO::Path absolutePath(m_projectPath, AZ::IO::PosixPathSeparator);
        absolutePath /= folderPath;
        absolutePath /= fileName;
        return absolutePath;
    }

    AZ::IO::Path PrefabAssetPathChangeTestFixture::GetPrefabFilePathForSerialization(AZStd::string_view folderPath, AZStd::string_view fileName)
    {
        AZ::IO::Path absolutePath = GetAbsoluteFilePathName(folderPath, fileName);
        return m_prefabLoaderInterface->GenerateRelativePath(absolutePath);
    }
} // namespace UnitTest
