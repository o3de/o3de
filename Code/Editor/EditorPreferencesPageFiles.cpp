/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "EditorPreferencesPageFiles.h"

#include <AzCore/Serialization/EditContext.h>

// AzToolsFramework
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzFramework/Translation/TranslationDef.h>

// Editor
#include "Settings.h"
#include "EditorViewportSettings.h"

namespace EditorPreferencesFilesStrings
{
    static const char* FilesClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Files");
    static const char* FilesClassDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "File Preferences");
    static const char* AutoNumberSlicesName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Append numeric value to slices");
    static const char* AutoNumberSlicesDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Should the name of the slice file be automatically numbered. e.g SliceName_001.slice vs. SliceName.slice");
    static const char* BackupOnSaveName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Backup on Save");
    static const char* MaxSaveBackupsName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Maximum Save Backups");
    static const char* TempDirectoryName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Standard Temporary Directory");
    static const char* UISliceSaveLocationName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "UI Slice Save location");
    static const char* UISliceSaveLocationDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Specify the default location to save new UI slices");

    static const char* ExternalEditorsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "External Editors");
    static const char* ScriptsEditorName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Scripts Editor");
    static const char* ScriptsEditorDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Scripts Text Editor (Default to O3DE internal tool when empty)");
    static const char* ScriptsEditorPlaceholder = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Default to O3DE internal tool when empty");
    static const char* ShadersEditorName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Shaders Editor");
    static const char* ShadersEditorDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Shaders Text Editor");
    static const char* BSpaceEditorName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "BSpace Editor");
    static const char* BSpaceEditorDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Bspace Text Editor");
    static const char* TextureEditorName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Texture Editor");
    static const char* AnimationEditorName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Animation Editor");

    static const char* AutoBackupClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Auto Backup");
    static const char* EnableName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Enable");
    static const char* EnableDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Enable Auto Backup");
    static const char* TimeIntervalName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Time Interval");
    static const char* TimeIntervalDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Auto Backup Interval (Minutes)");
    static const char* MaxBackupsName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Maximum Backups");
    static const char* MaxBackupsDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Maximum Auto Backups");
    static const char* RemindTimeName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Remind Time");
    static const char* RemindTimeDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Auto Remind Every (Minutes)");

    static const char* AssetBrowserSettingsClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Asset Browser Settings");
    static const char* MaxItemsDisplayedName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Maximum number of displayed items");
    static const char* MaxItemsDisplayedDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Maximum number of items to display in the Search View.");

    static const char* FilePreferencesClassName = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "File Preferences");
    static const char* FilePreferencesClassDesc = QT_TRANSLATE_NOOP("EditorPreferencesPageFiles", "Class for handling File Preferences");
}

void CEditorPreferencesPage_Files::Reflect(AZ::SerializeContext& serialize)
{
    serialize.Class<Files>()
        ->Version(3)
        ->Field("AutoNumberSlices", &Files::m_autoNumberSlices)
        ->Field("BackupOnSave", &Files::m_backupOnSave)
        ->Field("BackupOnSaveMaxCount", &Files::m_backupOnSaveMaxCount)
        ->Field("TempDirectory", &Files::m_standardTempDirectory)
        ->Field("SliceSaveLocation", &Files::m_saveLocation);

    serialize.Class<ExternalEditors>()
        ->Version(1)
        ->Field("Scripts", &ExternalEditors::m_scripts)
        ->Field("Shaders", &ExternalEditors::m_shaders)
        ->Field("BSpaces", &ExternalEditors::m_BSpaces)
        ->Field("Textures", &ExternalEditors::m_textures)
        ->Field("Animations", &ExternalEditors::m_animations);

    serialize.Class<AutoBackup>()
        ->Version(1)
        ->Field("Enabled", &AutoBackup::m_enabled)
        ->Field("Interval", &AutoBackup::m_timeInterval)
        ->Field("MaxCount", &AutoBackup::m_maxCount)
        ->Field("RemindTime", &AutoBackup::m_remindTime);

    serialize.Class<AssetBrowserSettings>()
        ->Version(1)
        ->Field("MaxEntriesShownCount", &AssetBrowserSettings::m_maxNumberOfItemsShownInSearch);

    serialize.Class<CEditorPreferencesPage_Files>()
        ->Version(1)
        ->Field("Files", &CEditorPreferencesPage_Files::m_files)
        ->Field("Editors", &CEditorPreferencesPage_Files::m_editors)
        ->Field("AutoBackup", &CEditorPreferencesPage_Files::m_autoBackup)
        ->Field("AssetBrowserSettings", &CEditorPreferencesPage_Files::m_assetBrowserSettings);

    AZ::EditContext* editContext = serialize.GetEditContext();
    if (editContext)
    {
        using namespace EditorPreferencesFilesStrings;

        editContext->Class<Files>(FilesClassName, FilesClassDesc)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Files::m_autoNumberSlices, AutoNumberSlicesName, AutoNumberSlicesDesc)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Files::m_backupOnSave, BackupOnSaveName, BackupOnSaveName)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &Files::m_backupOnSaveMaxCount, MaxSaveBackupsName, MaxSaveBackupsName)
            ->Attribute(AZ::Edit::Attributes::Min, 1)
            ->Attribute(AZ::Edit::Attributes::Max, 100)
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &Files::m_standardTempDirectory, TempDirectoryName, TempDirectoryName)
            ->DataElement(AZ::Edit::UIHandlers::LineEdit, &Files::m_saveLocation, UISliceSaveLocationName, UISliceSaveLocationDesc);

        editContext->Class<ExternalEditors>(ExternalEditorsClassName, ExternalEditorsClassName)
            ->DataElement(AZ::Edit::UIHandlers::ExeSelectBrowseEdit, &ExternalEditors::m_scripts, ScriptsEditorName, ScriptsEditorDesc)
            ->Attribute(AZ::Edit::Attributes::PlaceholderText, ScriptsEditorPlaceholder)
            ->DataElement(AZ::Edit::UIHandlers::ExeSelectBrowseEdit, &ExternalEditors::m_shaders, ShadersEditorName, ShadersEditorDesc)
            ->DataElement(AZ::Edit::UIHandlers::ExeSelectBrowseEdit, &ExternalEditors::m_BSpaces, BSpaceEditorName, BSpaceEditorDesc)
            ->DataElement(AZ::Edit::UIHandlers::ExeSelectBrowseEdit, &ExternalEditors::m_textures, TextureEditorName, TextureEditorName)
            ->DataElement(AZ::Edit::UIHandlers::ExeSelectBrowseEdit, &ExternalEditors::m_animations, AnimationEditorName, AnimationEditorName);

        editContext->Class<AutoBackup>(AutoBackupClassName, AutoBackupClassName)
            ->DataElement(AZ::Edit::UIHandlers::CheckBox, &AutoBackup::m_enabled, EnableName, EnableDesc)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AutoBackup::m_timeInterval, TimeIntervalName, TimeIntervalDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 2)
            ->Attribute(AZ::Edit::Attributes::Max, 10000)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AutoBackup::m_maxCount, MaxBackupsName, MaxBackupsDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 1)
            ->Attribute(AZ::Edit::Attributes::Max, 100)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AutoBackup::m_remindTime, RemindTimeName, RemindTimeDesc);

        editContext->Class<AssetBrowserSettings>(AssetBrowserSettingsClassName, AssetBrowserSettingsClassName)
            ->DataElement(
                AZ::Edit::UIHandlers::SpinBox, &AssetBrowserSettings::m_maxNumberOfItemsShownInSearch, MaxItemsDisplayedName, MaxItemsDisplayedDesc)
            ->Attribute(AZ::Edit::Attributes::Min, 50)
            ->Attribute(AZ::Edit::Attributes::Max, 5000);

        editContext->Class<CEditorPreferencesPage_Files>(FilePreferencesClassName, FilePreferencesClassDesc)
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_Files::m_files, FilesClassName, FilesClassDesc)
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_Files::m_editors, ExternalEditorsClassName, ExternalEditorsClassName)
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_Files::m_autoBackup, AutoBackupClassName, AutoBackupClassName)
            ->DataElement(AZ::Edit::UIHandlers::Default, &CEditorPreferencesPage_Files::m_assetBrowserSettings, AssetBrowserSettingsClassName, AssetBrowserSettingsClassName);
    }
}


CEditorPreferencesPage_Files::CEditorPreferencesPage_Files()
{
    InitializeSettings();

    m_icon = QIcon(":/res/Files.svg");
}

QIcon& CEditorPreferencesPage_Files::GetIcon()
{
    return m_icon;
}

void CEditorPreferencesPage_Files::OnApply()
{
    using namespace AzToolsFramework::SliceUtilities;

    auto sliceSettings = AZ::UserSettings::CreateFind<SliceUserSettings>(AZ_CRC_CE("SliceUserSettings"), AZ::UserSettings::CT_LOCAL);
    sliceSettings->m_autoNumber = m_files.m_autoNumberSlices;
    sliceSettings->m_saveLocation = m_files.m_saveLocation;

    gSettings.bBackupOnSave = m_files.m_backupOnSave;
    gSettings.backupOnSaveMaxCount = m_files.m_backupOnSaveMaxCount;
    gSettings.strStandardTempDirectory = m_files.m_standardTempDirectory.c_str();

    gSettings.textEditorForScript = m_editors.m_scripts.c_str();
    gSettings.textEditorForShaders = m_editors.m_shaders.c_str();
    gSettings.textEditorForBspaces = m_editors.m_BSpaces.c_str();
    gSettings.textureEditor = m_editors.m_textures.c_str();
    gSettings.animEditor = m_editors.m_animations.c_str();

    gSettings.autoBackupEnabled = m_autoBackup.m_enabled;
    gSettings.autoBackupTime = m_autoBackup.m_timeInterval;
    gSettings.autoBackupMaxCount = m_autoBackup.m_maxCount;
    gSettings.autoRemindTime = m_autoBackup.m_remindTime;

    SandboxEditor::SetMaxItemsShownInAssetBrowserSearch(m_assetBrowserSettings.m_maxNumberOfItemsShownInSearch);
}

void CEditorPreferencesPage_Files::InitializeSettings()
{
    using namespace AzToolsFramework::SliceUtilities;
    auto sliceSettings = AZ::UserSettings::CreateFind<SliceUserSettings>(AZ_CRC_CE("SliceUserSettings"), AZ::UserSettings::CT_LOCAL);

    m_files.m_autoNumberSlices = sliceSettings->m_autoNumber;
    m_files.m_saveLocation = sliceSettings->m_saveLocation;
    m_files.m_backupOnSave = gSettings.bBackupOnSave;
    m_files.m_backupOnSaveMaxCount = gSettings.backupOnSaveMaxCount;
    m_files.m_standardTempDirectory = gSettings.strStandardTempDirectory.toUtf8().data();

    m_editors.m_scripts = gSettings.textEditorForScript.toUtf8().data();
    m_editors.m_shaders = gSettings.textEditorForShaders.toUtf8().data();
    m_editors.m_BSpaces = gSettings.textEditorForBspaces.toUtf8().data();
    m_editors.m_textures = gSettings.textureEditor.toUtf8().data();
    m_editors.m_animations = gSettings.animEditor.toUtf8().data();

    m_autoBackup.m_enabled = gSettings.autoBackupEnabled;
    m_autoBackup.m_timeInterval = gSettings.autoBackupTime;
    m_autoBackup.m_maxCount = gSettings.autoBackupMaxCount;
    m_autoBackup.m_remindTime = gSettings.autoRemindTime;

    m_assetBrowserSettings.m_maxNumberOfItemsShownInSearch = SandboxEditor::MaxItemsShownInAssetBrowserSearch();
}
