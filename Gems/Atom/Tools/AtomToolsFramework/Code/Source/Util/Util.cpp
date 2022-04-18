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
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EditorWindowRequestBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
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

    AZStd::string GetDisplayNameFromPath(const AZStd::string& path)
    {
        QFileInfo fileInfo(path.c_str());
        QString fileName = fileInfo.baseName();
        fileName.replace(QRegExp("[^a-zA-Z\\d]"), " ");
        QStringList fileNameParts = fileName.split(' ', Qt::SkipEmptyParts);
        for (QString& part : fileNameParts)
        {
            part.replace(0, 1, part[0].toUpper());
        }
        return fileNameParts.join(" ").toUtf8().constData();
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

        AZ::IO::FixedMaxPath assetPath = AZ::IO::PathView(path).LexicallyNormal();
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
        for (const auto& [storedPath, flag] :
             GetSettingsObject<AZStd::unordered_map<AZStd::string, bool>>("/O3DE/Atom/Tools/EditablePathSettings"))
        {
            if (auto resolveResult = AZ::IO::FileIOBase::GetInstance()->ResolvePath(AZ::IO::PathView{ storedPath }))
            {
                if (resolveResult->Compare(path) == 0)
                {
                    return flag;
                }
            }
        }
        return true;
    }

    bool IsDocumentPathPreviewable(const AZStd::string& path)
    {
        for (const auto& [storedPath, flag] :
             GetSettingsObject<AZStd::unordered_map<AZStd::string, bool>>("/O3DE/Atom/Tools/PreviewablePathSettings"))
        {
            if (auto resolveResult = AZ::IO::FileIOBase::GetInstance()->ResolvePath(AZ::IO::PathView{ storedPath }))
            {
                if (resolveResult->Compare(path) == 0)
                {
                    return flag;
                }
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

    AZStd::string GetExteralReferencePath(
        const AZStd::string& exportPath, const AZStd::string& referencePath, const bool relativeToExportPath)
    {
        if (referencePath.empty())
        {
            return {};
        }

        if (!relativeToExportPath)
        {
            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;
            bool sourceInfoFound = false;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(
                sourceInfoFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, referencePath.c_str(),
                assetInfo, watchFolder);
            if (sourceInfoFound)
            {
                return assetInfo.m_relativePath;
            }
        }

        AZ::IO::BasicPath<AZStd::string> exportFolder(exportPath);
        exportFolder.RemoveFilename();
        return AZ::IO::PathView(referencePath).LexicallyRelative(exportFolder).StringAsPosix();
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
} // namespace AtomToolsFramework
