/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/ImageProcessing/ImageObject.h>
#include <Atom/ImageProcessing/ImageProcessingBus.h>
#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <AtomToolsFramework/AtomToolsFrameworkSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Jobs/Algorithms.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntryUtils.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetMimeDataContainer.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QTimer>
#include <QVBoxLayout>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    void LoadImageAsync(const AZStd::string& path, LoadImageAsyncCallback callback)
    {
        AZ::Job* job = AZ::CreateJobFunction(
            [path, callback]()
            {
                ImageProcessingAtom::IImageObjectPtr imageObject;
                ImageProcessingAtom::ImageProcessingRequestBus::BroadcastResult(
                    imageObject, &ImageProcessingAtom::ImageProcessingRequests::LoadImagePreview, path);

                if (imageObject)
                {
                    AZ::u8* imageBuf = nullptr;
                    AZ::u32 pitch = 0;
                    AZ::u32 mip = 0;
                    imageObject->GetImagePointer(mip, imageBuf, pitch);
                    const AZ::u32 width = imageObject->GetWidth(mip);
                    const AZ::u32 height = imageObject->GetHeight(mip);

                    QImage image(imageBuf, width, height, pitch, QImage::Format_RGBA8888);

                    if (callback)
                    {
                        callback(image);
                    }
                }
            },
            true);
        job->Start();
    }

    QWidget* GetToolMainWindow()
    {
        QWidget* mainWindow = QApplication::activeWindow();
        AzToolsFramework::EditorWindowRequestBus::BroadcastResult(
            mainWindow, &AzToolsFramework::EditorWindowRequestBus::Events::GetAppMainWindow);
        return mainWindow;
    }

    AZStd::string GetFirstNonEmptyString(const AZStd::vector<AZStd::string>& values, const AZStd::string& defaultValue)
    {
        for (const auto& value : values)
        {
            if (!value.empty())
            {
                return value;
            }
        }

        return defaultValue;
    }

    void ReplaceSymbolsInContainer(const AZStd::string& findText, const AZStd::string& replaceText, AZStd::vector<AZStd::string>& container)
    {
        const AZStd::regex findRegex(findText);
        for (auto& sourceText : container)
        {
            sourceText = AZStd::regex_replace(sourceText, findRegex, replaceText);
        }
    }

    void ReplaceSymbolsInContainer(
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& substitutionSymbols, AZStd::vector<AZStd::string>& container)
    {
        for (const auto& substitutionSymbolPair : substitutionSymbols)
        {
            ReplaceSymbolsInContainer(substitutionSymbolPair.first, substitutionSymbolPair.second, container);
        }
    }

    AZStd::string GetSymbolNameFromText(const AZStd::string& text)
    {
        QString symbolName(text.c_str());
        // Remove all leading whitespace
        symbolName.replace(QRegExp("^\\s+"), "");
        // Remove all trailing whitespace
        symbolName.replace(QRegExp("\\s+$"), "");
        // Replace non alphanumeric characters with _
        symbolName.replace(QRegExp("[^a-zA-Z\\d]"), "_");
        // Insert a _ between a lowercase or numeric character followed by an uppercase character
        symbolName.replace(QRegExp("([a-z\\d])([A-Z])"), "\\1_\\2");
        // Insert an underscore at the beginning of the string if it starts with a digit
        symbolName.replace(QRegExp("^(\\d)"), "_\\1");
        // Replace every sequence of whitespace characters with underscores
        symbolName.replace(QRegExp("\\s+"), "_");
        // Replace Sequences of _ with a single _
        symbolName.replace(QRegExp("_+"), "_");
        return symbolName.toLower().toUtf8().constData();
    }

    AZStd::string GetDisplayNameFromText(const AZStd::string& text)
    {
        QString displayName(text.c_str());
        // Remove all leading whitespace
        displayName.replace(QRegExp("^\\s+"), "");
        // Remove all trailing whitespace
        displayName.replace(QRegExp("\\s+$"), "");
        // Replace non alphanumeric characters with space
        displayName.replace(QRegExp("[^a-zA-Z\\d]"), " ");
        // Insert a space between a lowercase or numeric character followed by an uppercase character
        displayName.replace(QRegExp("([a-z\\d])([A-Z])"), "\\1 \\2");
        // Tokenize the string where separated by whitespace
        QStringList displayNameParts = displayName.split(QRegExp("\\s"), Qt::SkipEmptyParts);
        for (QString& part : displayNameParts)
        {
            // Capitalize the first character of every token
            part.replace(0, 1, part[0].toUpper());
        }
        // Recombine all of the strings separated by a single space
        return displayNameParts.join(" ").toUtf8().constData();
    }

    AZStd::string GetDisplayNameFromPath(const AZStd::string& path)
    {
        QFileInfo fileInfo(path.c_str());
        return GetDisplayNameFromText(fileInfo.baseName().toUtf8().constData());
    }

    bool GetStringListFromDialog(
        AZStd::vector<AZStd::string>& selectedStrings,
        const AZStd::vector<AZStd::string>& availableStrings,
        const AZStd::string& title,
        const bool multiSelect)
    {
        // Create a dialog that will display a list of string options and prompt the user for input.
        QDialog dialog(GetToolMainWindow());
        dialog.setModal(true);
        dialog.setWindowTitle(title.c_str());
        dialog.setLayout(new QVBoxLayout());

        // Fill the list widget with all of the available strings for the user to select.
        QListWidget listWidget(&dialog);
        listWidget.setSelectionMode(multiSelect ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);

        for (const auto& availableString : availableStrings)
        {
            listWidget.addItem(availableString.c_str());
        }

        listWidget.sortItems();

        // The selected strings vector already has items then attempt to select those in the list.
        for (const auto& selection : selectedStrings)
        {
            for (auto item : listWidget.findItems(selection.c_str(), Qt::MatchExactly))
            {
                item->setSelected(true);
            }
        }

        // Create the button box that will provide default dialog buttons to allow the user to accept or reject their selections.
        QDialogButtonBox buttonBox(&dialog);
        buttonBox.setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
        QObject::connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        // Add the list widget and button box to the layout so they appear and the dialog.
        dialog.layout()->addWidget(&listWidget);
        dialog.layout()->addWidget(&buttonBox);

        // Temporarily forcing a fixed size before showing it to compensate for window management centering and resizing the dialog.
        dialog.setFixedSize(400, 200);
        dialog.show();
        dialog.setMinimumSize(0, 0);
        dialog.setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

        // If the user accepts their selections then the selected strings director will be cleared and refilled with them.
        if (dialog.exec() == QDialog::Accepted)
        {
            selectedStrings.clear();
            for (auto item : listWidget.selectedItems())
            {
                selectedStrings.push_back(item->text().toUtf8().constData());
            }
            return true;
        }
        return false;
    }

    AZStd::string GetFileFilterFromSupportedExtensions(const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions)
    {
        // Build an ordered table of all of the supported extensions and their display names. This will be transformed into the file filter
        // that is displayed in the dialog.
        AZStd::map<AZStd::string, AZStd::set<AZStd::string>> orderedExtensions;
        for (const auto& extensionPair : supportedExtensions)
        {
            if (!extensionPair.second.empty())
            {
                // Sift all of the extensions into display name groups. If no display name was provided then we will use a default one.
                const auto& group = !extensionPair.first.empty() ? extensionPair.first : "Supported";

                // Convert the extension into a wild card.
                orderedExtensions[group].insert("*." + extensionPair.second);
            }
        }

        // Transform each of the ordered extension groups into individual file dialog filters that represent one or more extensions.
        AZStd::vector<AZStd::string> individualFilters;
        for (const auto& orderedExtensionPair : orderedExtensions)
        {
            AZStd::string combinedExtensions;
            AZ::StringFunc::Join(combinedExtensions, orderedExtensionPair.second, " ");
            individualFilters.push_back(orderedExtensionPair.first + " (" + combinedExtensions + ")");
        }

        // Combine all of the individual filters into a single expression that can be used directly with the file dialog.
        AZStd::string combinedFilters;
        AZ::StringFunc::Join(combinedFilters, individualFilters, ";;");
        return combinedFilters;
    }

    AZStd::string GetFirstValidSupportedExtension(const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions)
    {
        for (const auto& extensionPair : supportedExtensions)
        {
            if (!extensionPair.second.empty())
            {
                return extensionPair.second;
            }
        }

        return AZStd::string();
    }

    AZStd::string GetFirstMatchingSupportedExtension(
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions, const AZStd::string& path)
    {
        for (const auto& extensionPair : supportedExtensions)
        {
            if (!extensionPair.second.empty() && path.ends_with(extensionPair.second))
            {
                return extensionPair.second;
            }
        }

        return AZStd::string();
    }

    AZStd::string GetSaveFilePathFromDialog(
        const AZStd::string& initialPath,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions,
        const AZStd::string& title)
    {
        // Build the file dialog filter from all of the supported extensions.
        const auto& combinedFilters = GetFileFilterFromSupportedExtensions(supportedExtensions);

        // If no valid extensions were provided then return immediately.
        if (combinedFilters.empty())
        {
            QMessageBox::critical(GetToolMainWindow(), "Error", QString("No supported extensions were specified."));
            return AZStd::string();
        }

        // Remove any aliasing from the initial path to feed to the file dialog.
        AZStd::string displayedPath = GetPathWithoutAlias(initialPath);

        // If the display name is empty or invalid then build a unique default display name using the first supported extension. 
        if (displayedPath.empty())
        {
            displayedPath = GetUniqueUntitledFilePath(GetFirstValidSupportedExtension(supportedExtensions));
        }

        // Prompt the user to select a save file name using the input path and the list of filtered extensions.
        const QFileInfo selectedFileInfo(AzQtComponents::FileDialog::GetSaveFileName(
            GetToolMainWindow(), QObject::tr("Save %1").arg(title.c_str()), displayedPath.c_str(), combinedFilters.c_str()));

        // If the returned path is empty this means that the user cancelled or escaped from the dialog and is canceling the operation.
        if (selectedFileInfo.absoluteFilePath().isEmpty())
        {
            return AZStd::string();
        }

        // Find the supported extension corresponding to the user selection.
        const auto& selectedExtension =
            GetFirstMatchingSupportedExtension(supportedExtensions, selectedFileInfo.absoluteFilePath().toUtf8().constData());

        // If the selected path does not match any of the supported extensions consider it invalid and return. 
        if (selectedExtension.empty())
        {
            QMessageBox::critical(GetToolMainWindow(), "Error", QString("File name does not match supported extension."));
            return AZStd::string();
        }

        // Reconstruct the path to compensate for known problems with the file dialog and complex extensions containing multiple "." like
        // *.lightingpreset.azasset
        return QFileInfo(QString("%1/%2.%3").arg(selectedFileInfo.absolutePath()).arg(selectedFileInfo.baseName()).arg(selectedExtension.c_str()))
            .absoluteFilePath().toUtf8().constData();
    }

    AZStd::vector<AZStd::string> GetOpenFilePathsFromDialog(
        const AZStd::vector<AZStd::string>& selectedFilePaths,
        const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& supportedExtensions,
        const AZStd::string& title,
        const bool multiSelect)
    {
        // Removing aliases from all incoming paths because the asset selection model does not recognize them.
        AZStd::vector<AZStd::string> selectedFilePathsWithoutAliases = selectedFilePaths;
        for (auto& path : selectedFilePathsWithoutAliases)
        {
            path = GetPathWithoutAlias(path);
        }

        // Create a custom filter function to plug into the asset selection model. The filter function will only display source assets
        // matching one of the supported extensions. It will also ignore files in the cache folder, usually intermediate assets. This is
        // much faster than the previous iteration using regular expressions.
        auto filterFn = [&](const AssetBrowserEntry* entry)
        {
            if (entry->GetEntryType() != AssetBrowserEntry::AssetEntryType::Source)
            {
                return false;
            }

            const auto& path = entry->GetFullPath();
            return !IsPathIgnored(path) &&
                AZStd::any_of(
                    supportedExtensions.begin(),
                    supportedExtensions.end(),
                    [&](const auto& extensionPair)
                    {
                        return path.ends_with(AZStd::fixed_string<32>::format(".%s", extensionPair.second.c_str()));
                    });
        };

        AssetSelectionModel selection;
        selection.SetDisplayFilter(FilterConstType(new CustomFilter(filterFn)));
        selection.SetSelectionFilter(FilterConstType(new CustomFilter(filterFn)));
        selection.SetTitle(title.c_str());
        selection.SetMultiselect(multiSelect);
        selection.SetSelectedFilePaths(selectedFilePathsWithoutAliases);

        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::PickAssets, selection, GetToolMainWindow());

        // Return absolute paths for all results.
        AZStd::vector<AZStd::string> results;
        results.reserve(selection.GetResults().size());
        for (const auto& result : selection.GetResults())
        {
            results.push_back(result->GetFullPath());
        }
        return results;
    }

    AZStd::string GetUniqueFilePath(const AZStd::string& initialPath)
    {
        int counter = 0;
        QFileInfo fileInfo(initialPath.c_str());
        const QString extension = "." + fileInfo.completeSuffix();
        const QString basePathAndName = fileInfo.absoluteFilePath().left(fileInfo.absoluteFilePath().size() - extension.size());
        while (fileInfo.exists())
        {
            fileInfo = QString("%1_%2%3").arg(basePathAndName).arg(++counter).arg(extension);
        }
        return fileInfo.absoluteFilePath().toUtf8().constData();
    }

    AZStd::string GetUniqueUntitledFilePath(const AZStd::string& extension)
    {
        return GetUniqueFilePath(AZStd::string::format("%s/Assets/untitled.%s", AZ::Utils::GetProjectPath().c_str(), extension.c_str()));
    }

    bool ValidateDocumentPath(AZStd::string& path)
    {
        if (path.empty())
        {
            return false;
        }

        path = GetPathWithoutAlias(path);

        if (!AzFramework::StringFunc::Path::Normalize(path))
        {
            return false;
        }

        if (AzFramework::StringFunc::Path::IsRelative(path.c_str()))
        {
            return false;
        }

        if (!IsDocumentPathInSupportedFolder(path))
        {
            return false;
        }

        if (!IsDocumentPathEditable(path))
        {
            return false;
        }

        return true;
    }

    bool IsDocumentPathInSupportedFolder(const AZStd::string& path)
    {
        const auto& fullPath = GetPathWithoutAlias(path);
        const AZ::IO::FixedMaxPath assetPath = AZ::IO::PathView(fullPath).LexicallyNormal();
        for (const auto& assetFolder : GetSupportedSourceFolders())
        {
            // Check if the path is relative to the asset folder
            if (assetPath.IsRelativeTo(AZ::IO::PathView(assetFolder)))
            {
                return true;
            }
        }

        return false;
    }

    bool IsDocumentPathEditable(const AZStd::string& path)
    {
        bool result = true;
        AtomToolsFrameworkSystemRequestBus::BroadcastResult(result, &AtomToolsFrameworkSystemRequestBus::Events::IsPathEditable, path);
        return result;
    }

    bool IsDocumentPathPreviewable(const AZStd::string& path)
    {
        bool result = true;
        AtomToolsFrameworkSystemRequestBus::BroadcastResult(result, &AtomToolsFrameworkSystemRequestBus::Events::IsPathPreviewable, path);
        return result;
    }

    bool LaunchTool(const QString& baseName, const QStringList& arguments)
    {
        AZ::IO::FixedMaxPath engineRoot = AZ::Utils::GetEnginePath();
        AZ_Assert(!engineRoot.empty(), "Cannot query Engine Path");

        AZ::IO::FixedMaxPath launchPath =
            AZ::IO::FixedMaxPath(AZ::Utils::GetExecutableDirectory()) / (baseName + AZ_TRAIT_OS_EXECUTABLE_EXTENSION).toUtf8().constData();

        return QProcess::startDetached(launchPath.c_str(), arguments, engineRoot.c_str());
    }

    AZStd::string GetWatchFolder(const AZStd::string& sourcePath)
    {
        bool relativePathFound = false;
        AZStd::string relativePath;
        AZStd::string relativePathFolder;

        // GenerateRelativeSourcePath is necessary when saving new files because it allows us to get the info for files that may not exist yet. 
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            relativePathFound,
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GenerateRelativeSourcePath,
            sourcePath,
            relativePath,
            relativePathFolder);

        return relativePathFolder;
    }

    AZStd::string GetPathToExteralReference(const AZStd::string& exportPath, const AZStd::string& referencePath)
    {
        // An empty reference path signifies that there is no external reference and we can return immediately.
        if (referencePath.empty())
        {
            return {};
        }

        // Path aliases should be supported wherever possible to allow referencing files between different gems and projects. De-alias the
        // paths to compare them and attempt to generate a relative path.
        AZ::IO::FixedMaxPath exportPathWithoutAlias;
        AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(exportPathWithoutAlias, AZ::IO::PathView{ exportPath });
        const AZ::IO::PathView exportFolder = exportPathWithoutAlias.ParentPath();

        AZ::IO::FixedMaxPath referencePathWithoutAlias;
        AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(referencePathWithoutAlias, AZ::IO::PathView{ referencePath });

        // If both paths are contained underneath the same watch folder hierarchy then attempt to construct a relative path between them.
        if (GetWatchFolder(exportPath) == GetWatchFolder(referencePath))
        {
            const auto relativePath = referencePathWithoutAlias.LexicallyRelative(exportFolder);
            if (!relativePath.empty())
            {
                return relativePath.StringAsPosix();
            }
        }

        // If a relative path could not be constructed from the export path to the reference path then return the aliased path for the
        // reference.
        return GetPathWithAlias(referencePath);
    }

    bool SaveSettingsToFile(const AZ::IO::FixedMaxPath& savePath, const AZStd::vector<AZStd::string>& filters)
    {
        auto registry = AZ::SettingsRegistry::Get();
        if (registry == nullptr)
        {
            AZ_Warning("AtomToolsFramework", false, "Unable to access global settings registry.");
            return false;
        }

        AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
        dumperSettings.m_prettifyOutput = true;
        dumperSettings.m_includeFilter = [filters](AZStd::string_view path)
        {
            for (const auto& filter : filters)
            {
                if (filter.starts_with(path.substr(0, filter.size())))
                {
                    return true;
                }
            }
            return false;
        };

        AZStd::string stringBuffer;
        AZ::IO::ByteContainerStream stringStream(&stringBuffer);
        if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(*registry, "", stringStream, dumperSettings))
        {
            AZ_Warning("AtomToolsFramework", false, R"(Unable to save changes to the registry file at "%s"\n)", savePath.c_str());
            return false;
        }

        if (stringBuffer.empty())
        {
            return false;
        }

        bool saved = false;
        constexpr auto configurationMode =
            AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY;
        if (AZ::IO::SystemFile outputFile; outputFile.Open(savePath.c_str(), configurationMode))
        {
            saved = outputFile.Write(stringBuffer.c_str(), stringBuffer.size()) == stringBuffer.size();
        }

        AZ_Warning("AtomToolsFramework", saved, R"(Unable to save registry file to path "%s"\n)", savePath.c_str());
        return saved;
    }

    AZStd::string GetPathWithoutAlias(const AZStd::string& path)
    {
        AZ::IO::FixedMaxPath pathWithoutAlias;
        AZ::IO::FileIOBase::GetInstance()->ReplaceAlias(pathWithoutAlias, AZ::IO::PathView{ path });
        return pathWithoutAlias.StringAsPosix();
    }

    AZStd::string GetPathWithAlias(const AZStd::string& path)
    {
        AZ::IO::FixedMaxPath pathWithAlias;
        AZ::IO::FileIOBase::GetInstance()->ConvertToAlias(pathWithAlias, AZ::IO::PathView{ path });
        return pathWithAlias.StringAsPosix();
    }

    AZStd::set<AZStd::string> GetPathsFromMimeData(const QMimeData* mimeData)
    {
        AZStd::set<AZStd::string> paths;
        if (!mimeData)
        {
            return paths;
        }

        if (mimeData->hasFormat(AzToolsFramework::EditorAssetMimeDataContainer::GetMimeType()))
        {
            AzToolsFramework::EditorAssetMimeDataContainer container;
            if (container.FromMimeData(mimeData))
            {
                for (const auto& asset : container.m_assets)
                {
                    AZStd::string path = AZ::RPI::AssetUtils::GetSourcePathByAssetId(asset.m_assetId);
                    if (ValidateDocumentPath(path))
                    {
                        paths.insert(path);
                    }
                }
            }
        }

        AZStd::vector<const AzToolsFramework::AssetBrowser::AssetBrowserEntry*> entries;
        if (AzToolsFramework::AssetBrowser::Utils::FromMimeData(mimeData, entries))
        {
            for (const auto entry : entries)
            {
                AZStd::string path = entry->GetFullPath();
                if (ValidateDocumentPath(path))
                {
                    paths.insert(path);
                }
            }
        }

        for (const auto& url : mimeData->urls())
        {
            if (url.isLocalFile())
            {
                AZStd::string path = url.toLocalFile().toUtf8().constData();
                if (ValidateDocumentPath(path))
                {
                    paths.insert(path);
                }
            }
        }

        return paths;
    }

    AZStd::string GetAbsolutePathForSourceAsset(const AZStd::string& path)
    {
        bool found = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string rootFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            found, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, path.c_str(), sourceInfo, rootFolder);

        if (found)
        {
            const AZ::IO::Path result = AZ::IO::Path(rootFolder) / sourceInfo.m_relativePath;
            if (!result.empty())
            {
                return result.LexicallyNormal().String();
            }
        }

        return path;
    }

    AZStd::vector<AZStd::string> GetPathsForAssetSourceDependencies(const AZ::Data::AssetInfo& sourceInfo)
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
        assetDatabaseConnection.QuerySourceBySourceName(
            sourceInfo.m_relativePath.c_str(),
            [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
            {
                sourceEntry = entry;
                return false;
            });

        if (sourceEntry.m_sourceGuid.IsNull())
        {
            return {};
        }

        AZStd::vector<AZStd::string> sourcePaths;
        assetDatabaseConnection.QueryDependsOnSourceBySourceDependency(
            sourceEntry.m_sourceGuid,
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&sourcePaths, &assetDatabaseConnection](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry)
            {
                AZStd::string dependencyName = entry.m_dependsOnSource.GetPath();

                if (entry.m_dependsOnSource.IsUuid())
                {
                    assetDatabaseConnection.QuerySourceBySourceGuid(
                        entry.m_dependsOnSource.GetUuid(),
                        [&dependencyName](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                        {
                            dependencyName = entry.m_sourceName;
                            return false;
                        });
                }

                if (!dependencyName.empty())
                {
                    sourcePaths.emplace_back(GetAbsolutePathForSourceAsset(dependencyName));
                }
                return true;
            });

        assetDatabaseConnection.CloseDatabase();

        AZStd::sort(sourcePaths.begin(), sourcePaths.end());
        sourcePaths.erase(AZStd::unique(sourcePaths.begin(), sourcePaths.end()), sourcePaths.end());
        return sourcePaths;
    }

    AZStd::vector<AZStd::string> GetPathsForAssetSourceDependenciesById(const AZ::Data::AssetId& assetId)
    {
        bool found = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            found, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, assetId.m_guid, sourceInfo, watchFolder);

        return GetPathsForAssetSourceDependencies(sourceInfo);
    }

    AZStd::vector<AZStd::string> GetPathsForAssetSourceDependenciesByPath(const AZStd::string& sourcePath)
    {
        bool found = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            found,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
            GetPathWithoutAlias(sourcePath).c_str(),
            sourceInfo,
            watchFolder);

        return GetPathsForAssetSourceDependencies(sourceInfo);
    }

    AZStd::vector<AZStd::string> GetPathsForAssetSourceDependents(const AZ::Data::AssetInfo& sourceInfo)
    {
        AzToolsFramework::AssetDatabase::AssetDatabaseConnection assetDatabaseConnection;
        assetDatabaseConnection.OpenDatabase();

        AzToolsFramework::AssetDatabase::SourceDatabaseEntry sourceEntry;
        assetDatabaseConnection.QuerySourceBySourceName(
            sourceInfo.m_relativePath.c_str(),
            [&sourceEntry](const AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
            {
                sourceEntry = entry;
                return false;
            });

        if (sourceEntry.m_sourceGuid.IsNull())
        {
            return {};
        }

        AZStd::string scanFolderPath;
        assetDatabaseConnection.QueryScanFolderByScanFolderID(
            sourceEntry.m_scanFolderPK,
            [&scanFolderPath](const AzToolsFramework::AssetDatabase::ScanFolderDatabaseEntry& entry)
            {
                scanFolderPath = entry.m_scanFolder;
                return false;
            });

        auto absolutePath = AZ::IO::Path(scanFolderPath) / sourceEntry.m_sourceName;

        AZStd::vector<AZStd::string> sourcePaths;
        assetDatabaseConnection.QuerySourceDependencyByDependsOnSource(
            sourceEntry.m_sourceGuid,
            sourceEntry.m_sourceName.c_str(),
            absolutePath.FixedMaxPathStringAsPosix().c_str(),
            AzToolsFramework::AssetDatabase::SourceFileDependencyEntry::DEP_Any,
            [&sourcePaths, &assetDatabaseConnection](AzToolsFramework::AssetDatabase::SourceFileDependencyEntry& entry)
            {
                AZStd::string sourceName;
                assetDatabaseConnection.QuerySourceBySourceGuid(
                    entry.m_sourceGuid,
                    [&sourceName](AzToolsFramework::AssetDatabase::SourceDatabaseEntry& entry)
                    {
                        sourceName = entry.m_sourceName;
                        return false;
                    });

                if (!sourceName.empty())
                {
                    sourcePaths.emplace_back(GetAbsolutePathForSourceAsset(sourceName));
                }
                return true;
            });

        assetDatabaseConnection.CloseDatabase();

        AZStd::sort(sourcePaths.begin(), sourcePaths.end());
        sourcePaths.erase(AZStd::unique(sourcePaths.begin(), sourcePaths.end()), sourcePaths.end());
        return sourcePaths;
    }

    AZStd::vector<AZStd::string> GetPathsForAssetSourceDependentsById(const AZ::Data::AssetId& assetId)
    {
        bool found = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            found, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourceUUID, assetId.m_guid, sourceInfo, watchFolder);

        return GetPathsForAssetSourceDependents(sourceInfo);
    }

    AZStd::vector<AZStd::string> GetPathsForAssetSourceDependentsByPath(const AZStd::string& sourcePath)
    {
        bool found = false;
        AZ::Data::AssetInfo sourceInfo;
        AZStd::string watchFolder;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            found,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath,
            GetPathWithoutAlias(sourcePath).c_str(),
            sourceInfo,
            watchFolder);

        return GetPathsForAssetSourceDependents(sourceInfo);
    }

    void VisitFilesInFolder(
        const AZStd::string& folder, const AZStd::function<bool(const AZStd::string&)> visitorFn, bool recurse)
    {
        if (!visitorFn || IsPathIgnored(folder))
        {
            return;
        }

        AZStd::string fullFilter = folder + AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING + "*";
        AZ::StringFunc::Replace(fullFilter, "\\", "/");

        AZStd::string fullPath;
        AZ::IO::SystemFile::FindFiles(
            fullFilter.c_str(),
            [&](const char* item, bool is_file)
            {
                // Skip the '.' and '..' folders
                if ((azstricmp(".", item) == 0) || (azstricmp("..", item) == 0))
                {
                    return true;
                }

                // Continue if we can
                fullPath.clear();
                if (!AzFramework::StringFunc::Path::Join(folder.c_str(), item, fullPath))
                {
                    return false;
                }
                AZ::StringFunc::Replace(fullPath, "\\", "/");

                if (is_file)
                {
                    return visitorFn(fullPath);
                }

                VisitFilesInFolder(fullPath, visitorFn, recurse);
                return true;
            });
    }

    void VisitFilesInScanFolders(const AZStd::function<bool(const AZStd::string&)> visitorFn)
    {
        if (visitorFn)
        {
            for (const AZStd::string& scanFolder : GetSupportedSourceFolders())
            {
                VisitFilesInFolder(scanFolder, visitorFn, true);
            }
        }
    }

    AZStd::vector<AZStd::string> GetPathsInSourceFoldersMatchingFilter(const AZStd::function<bool(const AZStd::string&)> filterFn)
    {
        const auto& scanFolders = GetSupportedSourceFolders();

        AZStd::vector<AZStd::string> results;
        results.reserve(scanFolders.size());

        AZStd::for_each(
            scanFolders.begin(),
            scanFolders.end(),
            [&](const AZStd::string& scanFolder)
            {
                VisitFilesInFolder(
                    scanFolder,
                    [&](const AZStd::string& path)
                    {
                        if (!filterFn || filterFn(path))
                        {
                            results.emplace_back(path);
                        }
                        return true;
                    },
                    true);
            });

        // Sorting the container and removing duplicate paths to ensure uniqueness in case of nested or overlapping scan folders.
        // This was previously done automatically with a set but using a vector for compatibility with behavior context and Python.
        AZStd::sort(results.begin(), results.end());
        results.erase(AZStd::unique(results.begin(), results.end()), results.end());
        return results;
    }

    AZStd::vector<AZStd::string> GetPathsInSourceFoldersMatchingExtension(const AZStd::string& extension)
    {
        if (extension.empty())
        {
            return {};
        }

        const AZStd::string& extensionWithDot = (extension[0] == '.') ? extension : AZStd::string::format(".%s", extension.c_str());
        return GetPathsInSourceFoldersMatchingFilter(
            [&](const AZStd::string& path)
            {
                return path.ends_with(extensionWithDot) && IsDocumentPathEditable(path);
            });
    }

    bool IsPathIgnored(const AZStd::string& path)
    {
        bool result = false;
        AtomToolsFrameworkSystemRequestBus::BroadcastResult(result, &AtomToolsFrameworkSystemRequestBus::Events::IsPathIgnored, path);
        return result;
    }

    AZStd::vector<AZStd::string> GetSupportedSourceFolders()
    {
        AZStd::vector<AZStd::string> scanFolders;
        scanFolders.reserve(100);

        AzToolsFramework::AssetSystemRequestBus::Broadcast(
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetSafeFolders, scanFolders);

        AZStd::erase_if(scanFolders, [](const AZStd::string& path){ return IsPathIgnored(path); });
        return scanFolders;
    }

    void AddRegisteredScriptToMenu(QMenu* menu, const AZStd::string& registryKey, const AZStd::vector<AZStd::string>& arguments)
    {
        // Map containing vectors of script file paths organized by category
        using ScriptsSettingsMap = AZStd::map<AZStd::string, AZStd::vector<AZStd::string>>;

        // Retrieve and iterate over all of the registered script settings to add them to the menu
        for (const auto& [scriptCategoryName, scriptPathVec] : GetSettingsObject(registryKey, ScriptsSettingsMap()))
        {
            // Create a parent category menu group to contain all of the individual script menu actions.
            QMenu* scriptCategoryMenu = menu;
            if (!scriptCategoryName.empty())
            {
                scriptCategoryMenu = menu->findChild<QMenu*>(scriptCategoryName.c_str());
                if (!scriptCategoryMenu)
                {
                    scriptCategoryMenu = menu->addMenu(scriptCategoryName.c_str());
                }
            }

            // Create menu actions for executing the individual scripts.
            for (AZStd::string scriptPath : scriptPathVec)
            {
                // Removing the alias for the path so that we can check for its existence and add it to the menu.
                scriptPath = GetPathWithoutAlias(scriptPath);
                if (QFile::exists(scriptPath.c_str()))
                {
                    // Use the file name instead of the full path as the display name for the menu action.
                    AZStd::string filename;
                    AZ::StringFunc::Path::GetFullFileName(scriptPath.c_str(), filename);

                    scriptCategoryMenu->addAction(filename.c_str(), [scriptPath, arguments]() {
                        // Delay execution of the script until the next frame.
                        AZ::SystemTickBus::QueueFunction([scriptPath, arguments]() {
                            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
                                scriptPath,
                                AZStd::vector<AZStd::string_view>(arguments.begin(), arguments.end()));
                        });
                    });
                }
            }
        }

        // Create menu action for running arbitrary Python script.
        menu->addAction(QObject::tr("&Run Python Script..."), [arguments]() {
            const QString scriptPath = QFileDialog::getOpenFileName(
                GetToolMainWindow(), QObject::tr("Run Python Script"), QString(AZ::Utils::GetProjectPath().c_str()), QString("*.py"));
            if (!scriptPath.isEmpty())
            {
                // Delay execution of the script until the next frame.
                AZ::SystemTickBus::QueueFunction([scriptPath, arguments]() {
                    AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                        &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
                        scriptPath.toUtf8().constData(),
                        AZStd::vector<AZStd::string_view>(arguments.begin(), arguments.end()));
                });
            }
        });
    }

    void ReflectUtilFunctions(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // This will put these methods into the 'azlmbr.atomtools.util' module
            auto addUtilFunc = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "atomtools.util");
            };

            addUtilFunc(behaviorContext->Method("GetSymbolNameFromText", GetSymbolNameFromText, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetDisplayNameFromText", GetDisplayNameFromText, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetDisplayNameFromPath", GetDisplayNameFromPath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetStringListFromDialog", GetStringListFromDialog, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetFileFilterFromSupportedExtensions", GetFileFilterFromSupportedExtensions, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetFirstValidSupportedExtension", GetFirstValidSupportedExtension, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetFirstMatchingSupportedExtension", GetFirstMatchingSupportedExtension, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetSaveFilePathFromDialog", GetSaveFilePathFromDialog, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetOpenFilePathsFromDialog", GetOpenFilePathsFromDialog, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetUniqueFilePath", GetUniqueFilePath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetUniqueUntitledFilePath", GetUniqueUntitledFilePath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("ValidateDocumentPath", ValidateDocumentPath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("IsDocumentPathInSupportedFolder", IsDocumentPathInSupportedFolder, nullptr, ""));
            addUtilFunc(behaviorContext->Method("IsDocumentPathEditable", IsDocumentPathEditable, nullptr, ""));
            addUtilFunc(behaviorContext->Method("IsDocumentPathPreviewable", IsDocumentPathPreviewable, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathToExteralReference", GetPathToExteralReference, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathWithoutAlias", GetPathWithoutAlias, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathWithAlias", GetPathWithAlias, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetAbsolutePathForSourceAsset", GetAbsolutePathForSourceAsset, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsForAssetSourceDependencies", GetPathsForAssetSourceDependencies, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsForAssetSourceDependenciesById", GetPathsForAssetSourceDependenciesById, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsForAssetSourceDependenciesByPath", GetPathsForAssetSourceDependenciesByPath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsForAssetSourceDependents", GetPathsForAssetSourceDependents, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsForAssetSourceDependentsById", GetPathsForAssetSourceDependentsById, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsForAssetSourceDependentsByPath", GetPathsForAssetSourceDependentsByPath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsInSourceFoldersMatchingExtension", GetPathsInSourceFoldersMatchingExtension, nullptr, ""));
            addUtilFunc(behaviorContext->Method("IsPathIgnored", IsPathIgnored, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetSupportedSourceFolders", GetSupportedSourceFolders, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetSettingsValue_bool", GetSettingsValue<bool>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("SetSettingsValue_bool", SetSettingsValue<bool>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetSettingsValue_s64", GetSettingsValue<AZ::s64>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("SetSettingsValue_s64", SetSettingsValue<AZ::s64>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetSettingsValue_u64", GetSettingsValue<AZ::u64>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("SetSettingsValue_u64", SetSettingsValue<AZ::u64>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetSettingsValue_double", GetSettingsValue<double>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("SetSettingsValue_double", SetSettingsValue<double>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetSettingsValue_string", GetSettingsValue<AZStd::string>, nullptr, ""));
            addUtilFunc(behaviorContext->Method("SetSettingsValue_string", SetSettingsValue<AZStd::string>, nullptr, ""));
        }
    }
} // namespace AtomToolsFramework
