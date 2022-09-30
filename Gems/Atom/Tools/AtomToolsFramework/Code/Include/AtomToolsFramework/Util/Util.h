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
class QMenu;
class QMimeData;
class QWidget;

// Macros for printing information to the console if a condition is met.
#if defined(AZ_ENABLE_TRACING)
#define AZ_TracePrintf_IfTrue(window, expression, ...)\
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")\
    if (expression)\
    {\
        AZ_TraceFmtCompileTimeCheck(\
            expression,\
            AZ_VA_HAS_ARGS(__VA_ARGS__),\
            "String used in place of boolean expression for AZ_TracePrintf_IfTrue.",\
            "Did you mean AZ_TracePrintf_IfTrue(" #window ", true, \"%s\", " #expression "); ?",\
            "Did you mean AZ_TracePrintf_IfTrue(" #window ", true, " #expression ", " #__VA_ARGS__ "); ?");\
        AZ::Debug::Trace::Instance().Printf(window, __VA_ARGS__);\
    }\
    AZ_POP_DISABLE_WARNING

#define AZ_TracePrintfOnce_IfTrue(window, expression, ...)\
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")\
    if (expression)\
    {\
        AZ_TraceFmtCompileTimeCheck(\
            expression,\
            AZ_VA_HAS_ARGS(__VA_ARGS__),\
            "String used in place of boolean expression for AZ_TracePrintfOnce_IfTrue.",\
            "Did you mean AZ_TracePrintfOnce_IfTrue(" #window ", true, \"%s\", " #expression "); ?",\
            "Did you mean AZ_TracePrintfOnce_IfTrue(" #window ", true, " #expression ", " #__VA_ARGS__ "); ?");\
        static bool AZ_CONCAT_VAR_NAME(azTraceDisplayed, __LINE__) = false;\
        if (!AZ_CONCAT_VAR_NAME(azTraceDisplayed, __LINE__))\
        {\
            AZ::Debug::Trace::Instance().Printf(window, __VA_ARGS__);\
            AZ_CONCAT_VAR_NAME(azTraceDisplayed, __LINE__) = true;\
        }\
    }\
    AZ_POP_DISABLE_WARNING
#else // !AZ_ENABLE_TRACING
#define AZ_TracePrintf_IfTrue(...)
#define AZ_TracePrintfOnce_IfTrue(...)
#endif

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

    //! Converts input text into a code-friendly symbol name, removing special characters and replacing whitespace with underscores.
    //! @param text Input text that will be converted into a symbol name
    //! @returns the symbol name generated from the text
    AZStd::string GetSymbolNameFromText(const AZStd::string& text);

    //! Converts input text into a user-friendly display name, splitting words at camelcase and non-word character boundaries.
    //! @param text Input text that will be converted into a display name
    //! @returns the display name generated from the text
    AZStd::string GetDisplayNameFromText(const AZStd::string& text);

    //! Returns a sanitized display name by removing the path, extension, and replacing special characters in a filename
    //! @param path File path that will be converted into a display name
    //! @returns the display name generated from the file path
    AZStd::string GetDisplayNameFromPath(const AZStd::string& path);

    //! Prompts the user to select one more strings from a dialog with a list widget.
    //! @param selectedStrings Input for the currently selected set of strings. Output for the newly selected strings.
    //! @param availableStrings List of strings that will populate the dialog with available choices.
    //! @param title Text that will be displayed at the top of the dialog.
    //! @param multiSelect Flag that determines whether the user will be able to select one or multiple strings.
    //! @returns True if the user accepted the selected strings. Otherwise false.
    bool GetStringListFromDialog(
        AZStd::vector<AZStd::string>& selectedStrings,
        const AZStd::vector<AZStd::string>& availableStrings,
        const AZStd::string& title = "Select Values",
        const bool multiSelect = false);

    //! Build a file dialog filter string by combining all of the entries in the supported extensions table. .
    //! @param supportedExtensions Table of descriptions and extensions use to configure file filters.
    //! @returns The generated string that can be fed directly into a file dialog filter.
    AZStd::string GetFileFilterFromSupportedExtensions(const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions);

    //! Returns the first non empty string found in a container of supported extensions.
    //! @param supportedExtensions Table of descriptions and extensions use to configure file filters.
    //! @returns The first non empty extension founded in container
    AZStd::string GetFirstValidSupportedExtension(const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions);

    //! Returns the first extension matching a path.
    //! @param supportedExtensions Table of descriptions and extensions use to configure file filters.
    //! @param path Path or file name that will be compared against supported extensions.
    //! @returns The first occurrence of an extension matching the path.
    AZStd::string GetFirstMatchingSupportedExtension(
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions, const AZStd::string& path);

    //! Opens a dialog to prompt the user to select a save file path
    //! @param initialPath File path initially selected in the dialog
    //! @param supportedExtensions Table of descriptions and extensions use to configure file filters.
    //! @param title Description of the filetype being opened that's displayed at the top of the dialog
    //! @returns Absolute path of the selected file, or an empty string if nothing was selected
    AZStd::string GetSaveFilePathFromDialog(
        const AZStd::string& initialPath,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions,
        const AZStd::string& title);

    //! Opens a dialog to prompt the user to select one or more files to open 
    //! @param selectedFilePaths A list of file paths that will be selected in the dialog
    //! @param supportedExtensions Table of descriptions and extensions use to configure file filters.
    //! @param title Description of the filetype being opened that's displayed at the top of the dialog
    //! @param multiSelect If true, the file picker will allow selecting multiple files
    //! @returns Container of selected files matching the filter 
    AZStd::vector<AZStd::string> GetOpenFilePathsFromDialog(
        const AZStd::vector<AZStd::string>& selectedFilePaths,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions,
        const AZStd::string& title,
        const bool multiSelect);

    //! Converts an input file path to a unique file path by appending a unique number
    //! @param initialPath The starting path that will be compared to other existing files and modified until it is unique
    //! @returns A unique file path based on the initial file path
    AZStd::string GetUniqueFilePath(const AZStd::string& initialPath);

    //! Generates a unique, untitled file path in the project asset folder
    //! @param Extension Extension of the file path to be generated
    //! @returns Absolute file path with a unique filename
    AZStd::string GetUniqueUntitledFilePath(const AZStd::string& extension);

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
    AZStd::string GetPathToExteralReference(const AZStd::string& exportPath, const AZStd::string& referencePath);

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

    //! Helper function to get a value from the settings registry
    //! @param path Path of the setting to be retrieved
    //! @param defaultValue Value returned if the setting does not exist in the registry
    //! @returns The value of the setting if it was found, otherwise defaultValue
    template<typename T>
    T GetSettingsValue(AZStd::string_view path, const T& defaultValue = {})
    {
        T result;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return (settingsRegistry && settingsRegistry->Get(result, path)) ? result : defaultValue;
    }

    //! Helper function to set a value in the settings registry
    //! @param path Path of the setting to be assigned
    //! @param value Value to be assigned in the registry
    //! @returns True if the value was successfully assigned, otherwise false
    template<typename T>
    bool SetSettingsValue(AZStd::string_view path, const T& value)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return settingsRegistry && settingsRegistry->Set(path, value);
    }

    //! Helper function to get an object from the settings registry
    //! @param path Path of the setting to be retrieved
    //! @param defaultValue Value returned if the setting does not exist in the registry
    //! @returns The value of the setting if it was found, otherwise defaultValue
    template<typename T>
    T GetSettingsObject(AZStd::string_view path, const T& defaultValue = {})
    {
        T result;
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return (settingsRegistry && settingsRegistry->GetObject<T>(result, path)) ? result : defaultValue;
    }

    //! Helper function to set an object in the settings registry
    //! @param path Path of the setting to be assigned
    //! @param value Value to be assigned in the registry
    //! @returns True if the value was successfully assigned, otherwise false
    template<typename T>
    bool SetSettingsObject(AZStd::string_view path, const T& value)
    {
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        return settingsRegistry && settingsRegistry->SetObject<T>(path, value);
    }

    //! Saves registry settings matching a filter
    //! @param savePath File where registry settings will be saved
    //! @param filters Container of substrains used to filter registry settings by path
    //! @returns True if the settings were saved, otherwise false
    bool SaveSettingsToFile(const AZ::IO::FixedMaxPath& savePath, const AZStd::vector<AZStd::string>& filters);

    //! Helper function to convert a path containing an alias into a full path
    AZStd::string GetPathWithoutAlias(const AZStd::string& path);

    //! Helper function to convert a full path into one containing an alias
    AZStd::string GetPathWithAlias(const AZStd::string& path);

    //! Collect a set of file paths contained within asset browser entry or URL mime data
    AZStd::set<AZStd::string> GetPathsFromMimeData(const QMimeData* mimeData);

    //! Invokes a visitor function on every file contained in the initial folder, optionally recursing through subfolders and visiting those files as well.
    void VisitFilesInFolder(const AZStd::string& folder, const AZStd::function<bool(const AZStd::string&)> visitorFn, bool recurse);

    //! Invokes a visitor function on all files contained in asset scan folders.
    void VisitFilesInScanFolders(const AZStd::function<bool(const AZStd::string&)> visitorFn);

    // Visits all scan folders asynchronously, gathering all of the paths matching the filter. 
    AZStd::vector<AZStd::string> GetPathsInSourceFoldersMatchingFilter(const AZStd::function<bool(const AZStd::string&)> filterFn);

    //! Collect a set of file paths from all project safe folders matching a wild card
    AZStd::vector<AZStd::string> GetPathsInSourceFoldersMatchingWildcard(const AZStd::string& wildcard);

    //! Add menu actions for scripts specified in the settings registry
    //! @param menu The menu where the actions will be inserted
    //! @param registryKey The path to the registry setting where script categories are registered
    //! @param arguments The list of arguments passed into the script when executed
    void AddRegisteredScriptToMenu(QMenu* menu, const AZStd::string& registryKey, const AZStd::vector<AZStd::string>& arguments);

    //! Reflect utility functions to behavior context
    void ReflectUtilFunctions(AZ::ReflectContext* context);

} // namespace AtomToolsFramework

