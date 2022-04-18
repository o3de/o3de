/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include <QStringList>
AZ_POP_DISABLE_WARNING

class QImage;
class QWidget;

namespace AtomToolsFramework
{
    using LoadImageAsyncCallback = AZStd::function<void(const QImage&)>;

    //! Loads an image file asynchronously and invokes the callback with the resulting image after load is complete
    //! @path Absolute path of image file to be loaded
    //! @param callback callback that will be triggered once the file is loaded
    void LoadImageAsync(const AZStd::string& path, LoadImageAsyncCallback callback);

    //! Get a pointer to the application main window
    //! @returns a pointer to the application main window 
    QWidget* GetToolMainWindow();

    //! Returns a sanitized display name by removing the path, extension, and replacing special characters in a filename
    //! @param path File path that will be converted into a display name
    //! @returns the display name generated from the file path
    AZStd::string GetDisplayNameFromPath(const AZStd::string& path);

    //! Opens a dialog to prompt the user to select a save file path
    //! @param initialPath File path initially selected in the dialog
    //! @param title Description of the filetype being opened that's displayed at the top of the dialog
    //! @returns Absolute path of the selected file, or an empty string if nothing was selected
    AZStd::string GetSaveFilePath(const AZStd::string& initialPath, const AZStd::string& title = "Document");

    //! Opens a dialog to prompt the user to select one or more files to open 
    //! @param filter A regular expression filter to determine which files are selectable and displayed in the dialog 
    //! @param title Description of the filetype being opened that's displayed at the top of the dialog
    //! @returns Container of selected files matching the filter 
    AZStd::vector<AZStd::string> GetOpenFilePaths(const QRegExp& filter, const AZStd::string& title = "Document");

    //! Converts an input file path to a unique file path by appending a unique number
    //! @param initialPath The starting path that will be compared to other existing files and modified until it is unique
    //! @returns A unique file path based on the initial file path
    AZStd::string GetUniqueFilePath(const AZStd::string& initialPath);

    //! Generates a unique, untitled file path in the project asset folder
    //! @param Extension Extension of the file path to be generated
    //! @returns Absolute file path with a unique filename
    AZStd::string GetUniqueDefaultSaveFilePath(const AZStd::string& extension);

    //! Opens a dialog to prompt the user to select a file path where the initial file will be duplicated 
    //! @param initialPath File path to be duplicated, will be used to generate a unique filename for the new file 
    //! @returns Absolute path of the selected file, or an empty string if nothing was selected
    AZStd::string GetUniqueDuplicateFilePath(const AZStd::string& initialPath);

    //! Verifies that an input file path is not empty, is not relative, can be normalized, and is a valid source file path accessible by the project
    //! @param path File path to be validated and normalized
    //! @returns true if the path is valid, otherwise false
    bool ValidateDocumentPath(AZStd::string& path);

    //! Determines if a file path exists in a valid asset folder for the project or one of its gems
    //! @param path File path to be validated
    //! @returns true if the path is valid, otherwise false
    bool IsDocumentPathInSupportedFolder(const AZStd::string& path);

    //! Compares the specified source asset path to registry settings to determine if it can be opened or edited in a tool
    //! @param path Absolute path of the file to be tested
    //! @returns true if the file can be opened or edited, otherwise false
    bool IsDocumentPathEditable(const AZStd::string& path);

    //! Compares the specified source asset path to registry settings to determine if it can be used to display thumbnail images
    //! @param path Absolute path of the file to be tested
    //! @returns true if the file can be previewed, otherwise false
    bool IsDocumentPathPreviewable(const AZStd::string& path);

    //! Launches an O3DE application in a detached process
    //! @param baseName Base filename of the application executable that must be in the bin folder
    //! @returns true if the process was launched, otherwise false
    bool LaunchTool(const QString& baseName, const QStringList& arguments);

    //! Generate a file path that is relative to either the source asset root or the export path
    //! @param exportPath absolute path of the file being saved
    //! @param referencePath absolute path of a file that will be treated as an external reference
    //! @param relativeToExportPath specifies if the path is relative to the source asset root or the export path
    AZStd::string GetExteralReferencePath(
        const AZStd::string& exportPath, const AZStd::string& referencePath, const bool relativeToExportPath = false);

    //! Traverse up the instance data hierarchy to find a node containing the corresponding type
    template<typename T>
    const T* FindAncestorInstanceDataNodeByType(const AzToolsFramework::InstanceDataNode* pNode)
    {
        // Traverse up the hierarchy from the input node to search for an instance matching the specified type
        for (const auto* currentNode = pNode; currentNode; currentNode = currentNode->GetParent())
        {
            const auto* context = currentNode->GetSerializeContext();
            const auto* classData = currentNode->GetClassMetadata();
            if (context && classData)
            {
                if (context->CanDowncast(classData->m_typeId, azrtti_typeid<T>(), classData->m_azRtti, nullptr))
                {
                    return static_cast<const T*>(currentNode->FirstInstance());
                }
            }
        }

        return nullptr;
    }

    template<typename T>
    T GetSettingsValue(AZStd::string_view path, const T& defaultValue = {})
    {
        T result;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return (settingsRegistry && settingsRegistry->Get(result, path)) ? result : defaultValue;
    }

    template<typename T>
    bool SetSettingsValue(AZStd::string_view path, const T& value)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return settingsRegistry && settingsRegistry->Set(path, value);
    }

    template<typename T>
    T GetSettingsObject(AZStd::string_view path, const T& defaultValue = {})
    {
        T result;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return (settingsRegistry && settingsRegistry->GetObject<T>(result, path)) ? result : defaultValue;
    }

    template<typename T>
    bool SetSettingsObject(AZStd::string_view path, const T& value)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return settingsRegistry && settingsRegistry->SetObject<T>(path, value);
    }

    bool SaveSettingsToFile(const AZ::IO::FixedMaxPath& savePath, const AZStd::vector<AZStd::string>& filters);
} // namespace AtomToolsFramework
