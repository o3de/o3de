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
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/sort.h>
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
#include <AzToolsFramework/ToolsComponents/EditorAssetMimeDataContainer.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QTimer>
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

    AZStd::string GetSymbolNameFromText(const AZStd::string& text)
    {
        QString symbolName(text.c_str());
        symbolName.replace(QRegExp("[^a-zA-Z\\d]"), " ");
        symbolName.replace(QRegExp("([a-z\\d])([A-Z])"), "\\1 \\2");
        symbolName.replace(QRegExp("\\A(\\d)"), "_\\1");
        symbolName.replace(QRegExp("\\s+"), "_");
        return symbolName.toLower().toUtf8().constData();
    }

    AZStd::string GetDisplayNameFromText(const AZStd::string& text)
    {
        QString displayName(text.c_str());
        displayName.replace(QRegExp("[^a-zA-Z\\d]"), " ");
        displayName.replace(QRegExp("([a-z\\d])([A-Z])"), "\\1 \\2");
        QStringList displayNameParts = displayName.split(QRegExp("\\s"), Qt::SkipEmptyParts);
        for (QString& part : displayNameParts)
        {
            part.replace(0, 1, part[0].toUpper());
        }
        return displayNameParts.join(" ").toUtf8().constData();
    }

    AZStd::string GetDisplayNameFromPath(const AZStd::string& path)
    {
        QFileInfo fileInfo(path.c_str());
        return GetDisplayNameFromText(fileInfo.baseName().toUtf8().constData());
    }

    AZStd::string GetSaveFilePath(const AZStd::string& initialPath, const AZStd::string& title)
    {
        const QFileInfo initialFileInfo(initialPath.c_str());
        const QString initialExt(initialFileInfo.completeSuffix());

        const QFileInfo selectedFileInfo(AzQtComponents::FileDialog::GetSaveFileName(
            GetToolMainWindow(),
            QObject::tr("Save %1").arg(title.c_str()),
            initialFileInfo.absoluteFilePath(),
            QString("Files (*.%1)").arg(initialExt)));

        if (selectedFileInfo.absoluteFilePath().isEmpty())
        {
            // Cancelled operation
            return AZStd::string();
        }

        if (!selectedFileInfo.absoluteFilePath().endsWith(initialExt))
        {
            QMessageBox::critical(GetToolMainWindow(), "Error", QString("File name must have .%1 extension.").arg(initialExt));
            return AZStd::string();
        }

        // Reconstructing the file info from the absolute path and expected extension to compensate for an issue with the save file
        // dialog adding the extension multiple times if it contains "." like *.lightingpreset.azasset
        return QFileInfo(QString("%1/%2.%3").arg(selectedFileInfo.absolutePath()).arg(selectedFileInfo.baseName()).arg(initialExt))
            .absoluteFilePath().toUtf8().constData();
    }

    AZStd::vector<AZStd::string> GetOpenFilePaths(const QRegExp& filter, const AZStd::string& title)
    {
        auto selection = AzToolsFramework::AssetBrowser::AssetSelectionModel::SourceAssetTypeSelection(filter);
        selection.SetTitle(title.c_str());
        selection.SetMultiselect(true);

        AzToolsFramework::AssetBrowser::AssetBrowserComponentRequestBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetBrowserComponentRequests::PickAssets, selection, GetToolMainWindow());

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

    AZStd::string GetUniqueDefaultSaveFilePath(const AZStd::string& extension)
    {
        return GetUniqueFilePath(AZStd::string::format("%s/Assets/untitled.%s", AZ::Utils::GetProjectPath().c_str(), extension.c_str()));
    }

    AZStd::string GetUniqueDuplicateFilePath(const AZStd::string& initialPath)
    {
        return GetSaveFilePath(GetUniqueFilePath(initialPath), "Duplicate File");
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
        bool assetFoldersRetrieved = false;
        AZStd::vector<AZStd::string> assetFolders;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
            assetFoldersRetrieved, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders, assetFolders);

        AZ::IO::FixedMaxPath assetPath = AZ::IO::PathView(GetPathWithoutAlias(path)).LexicallyNormal();
        for (const auto& assetFolder : assetFolders)
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
        const AZStd::string pathWithoutAlias = GetPathWithoutAlias(path);

        for (const auto& [storedPath, flag] :
             GetSettingsObject<AZStd::unordered_map<AZStd::string, bool>>("/O3DE/Atom/Tools/EditablePathSettings"))
        {
            if (pathWithoutAlias == GetPathWithoutAlias(storedPath))
            {
                return flag;
            }
        }
        return true;
    }

    bool IsDocumentPathPreviewable(const AZStd::string& path)
    {
        const AZStd::string pathWithoutAlias = GetPathWithoutAlias(path);

        for (const auto& [storedPath, flag] :
             GetSettingsObject<AZStd::unordered_map<AZStd::string, bool>>("/O3DE/Atom/Tools/PreviewablePathSettings"))
        {
            if (pathWithoutAlias == GetPathWithoutAlias(storedPath))
            {
                return flag;
            }
        }
        return true;
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
            saved = outputFile.Write(stringBuffer.data(), stringBuffer.size()) == stringBuffer.size();
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

    AZStd::vector<AZStd::string> GetPathsInSourceFoldersMatchingWildcard(const AZStd::string& wildcard)
    {
        AZStd::vector<AZStd::string> results;
        AZStd::vector<AZStd::string> scanFolders;
        AzToolsFramework::AssetSystemRequestBus::Broadcast(
            &AzToolsFramework::AssetSystem::AssetSystemRequest::GetAssetSafeFolders, scanFolders);

        for (const AZStd::string& scanFolder : scanFolders)
        {
            if (const auto& findFilesResult = AzFramework::FileFunc::FindFileList(scanFolder, wildcard.c_str(), true))
            {
                for (AZStd::string path : findFilesResult.GetValue())
                {
                    if (ValidateDocumentPath(path))
                    {
                        results.push_back(path);
                    }
                }
            }
        }

        // Sorting the container and removing duplicate paths to ensure uniqueness in case of nested or overlapping scan folders.
        // This was previously done automatically with a set but using a vector for compatibility with behavior context and Python. 
        AZStd::sort(results.begin(), results.end());
        results.erase(AZStd::unique(results.begin(), results.end()), results.end());

        return results;
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
                        QTimer::singleShot(0, [scriptPath, arguments]() {
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
                QTimer::singleShot(0, [scriptPath, arguments]() {
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
            addUtilFunc(behaviorContext->Method("GetSaveFilePath", GetSaveFilePath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetUniqueFilePath", GetUniqueFilePath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetUniqueDefaultSaveFilePath", GetUniqueDefaultSaveFilePath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetUniqueDuplicateFilePath", GetUniqueDuplicateFilePath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("ValidateDocumentPath", ValidateDocumentPath, nullptr, ""));
            addUtilFunc(behaviorContext->Method("IsDocumentPathInSupportedFolder", IsDocumentPathInSupportedFolder, nullptr, ""));
            addUtilFunc(behaviorContext->Method("IsDocumentPathEditable", IsDocumentPathEditable, nullptr, ""));
            addUtilFunc(behaviorContext->Method("IsDocumentPathPreviewable", IsDocumentPathPreviewable, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathToExteralReference", GetPathToExteralReference, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathWithoutAlias", GetPathWithoutAlias, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathWithAlias", GetPathWithAlias, nullptr, ""));
            addUtilFunc(behaviorContext->Method("GetPathsInSourceFoldersMatchingWildcard", GetPathsInSourceFoldersMatchingWildcard, nullptr, ""));
        }
    }
} // namespace AtomToolsFramework
