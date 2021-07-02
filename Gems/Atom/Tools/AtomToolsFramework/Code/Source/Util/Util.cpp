/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    QFileInfo GetSaveFileInfo(const QString& initialPath)
    {
        const QFileInfo initialFileInfo(initialPath);
        const QString initialExt(initialFileInfo.completeSuffix());

        const QFileInfo selectedFileInfo(QFileDialog::getSaveFileName(
            QApplication::activeWindow(),
            "Save File",
            initialFileInfo.absolutePath() +
            AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING +
            initialFileInfo.baseName(),
            QString("Files (*.%1)").arg(initialExt)));

        if (selectedFileInfo.absoluteFilePath().isEmpty())
        {
            // Cancelled operation
            return QFileInfo();
        }

        if (!selectedFileInfo.absoluteFilePath().endsWith(initialExt))
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("File name must have .%1 extension.").arg(initialExt));
            return QFileInfo();
        }

        return selectedFileInfo;
    }

    QFileInfo GetOpenFileInfo(const AZStd::vector<AZ::Data::AssetType>& assetTypes)
    {
        using namespace AZ::Data;
        using namespace AzToolsFramework::AssetBrowser;

        // [GFX TODO] Should this just be an open file dialog filtered to supported source data extensions?
        auto selection = AssetSelectionModel::AssetTypesSelection(assetTypes);

        // [GFX TODO] This is functional but UI is not as designed
        AssetBrowserComponentRequestBus::Broadcast(&AssetBrowserComponentRequests::PickAssets, selection, QApplication::activeWindow());
        if (!selection.IsValid())
        {
            return QFileInfo();
        }

        auto entry = selection.GetResult();
        const SourceAssetBrowserEntry* sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(entry);
        if (!sourceEntry)
        {
            const ProductAssetBrowserEntry* productEntry = azrtti_cast<const ProductAssetBrowserEntry*>(entry);
            if (productEntry)
            {
                sourceEntry = azrtti_cast<const SourceAssetBrowserEntry*>(productEntry->GetParent());
            }
        }

        if (!sourceEntry)
        {
            return QFileInfo();
        }

        return QFileInfo(sourceEntry->GetFullPath().c_str());
    }

    QFileInfo GetUniqueFileInfo(const QString& initialPath)
    {
        int counter = 0;
        QFileInfo fileInfo = initialPath;
        const QString extension = "." + fileInfo.completeSuffix();
        const QString basePathAndName = fileInfo.absoluteFilePath().left(fileInfo.absoluteFilePath().size() - extension.size());
        while (fileInfo.exists())
        {
            fileInfo = QString("%1_%2%3").arg(basePathAndName).arg(++counter).arg(extension);
        }
        return fileInfo;
    }

    QFileInfo GetDuplicationFileInfo(const QString& initialPath)
    {
        const QFileInfo initialFileInfo(initialPath);
        const QString initialExt(initialFileInfo.completeSuffix());

        const QFileInfo duplicateFileInfo(QFileDialog::getSaveFileName(
            QApplication::activeWindow(),
            "Duplicate File",
            GetUniqueFileInfo(initialPath).absoluteFilePath(),
            QString("Files (*.%1)").arg(initialExt)));

        if (duplicateFileInfo == initialFileInfo)
        {
            // Cancelled operation or selected same path
            return QFileInfo();
        }

        if (duplicateFileInfo.absoluteFilePath().isEmpty())
        {
            // Cancelled operation or selected same path
            return QFileInfo();
        }

        if (!duplicateFileInfo.absoluteFilePath().endsWith(initialExt))
        {
            QMessageBox::critical(QApplication::activeWindow(), "Error", QString("File name must have .%1 extension.").arg(initialExt));
            return QFileInfo();
        }

        return duplicateFileInfo;
    }

    bool LaunchTool(const QString& baseName, const QString& extension, const QStringList& arguments)
    {
        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        AZ_Assert(engineRoot != nullptr, "AzFramework::ApplicationRequests::GetEngineRoot failed");

        char binFolderName[AZ_MAX_PATH_LEN] = {};
        AZ::Utils::GetExecutablePathReturnType ret = AZ::Utils::GetExecutablePath(binFolderName, AZ_MAX_PATH_LEN);

        // If it contains the filename, zero out the last path separator character...
        if (ret.m_pathIncludesFilename)
        {
            char* lastSlash = strrchr(binFolderName, AZ_CORRECT_FILESYSTEM_SEPARATOR);
            if (lastSlash)
            {
                *lastSlash = '\0';
            }
        }

        const QString path = QString("%1%2%3%4")
            .arg(binFolderName)
            .arg(AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING)
            .arg(baseName)
            .arg(extension);

        return QProcess::startDetached(path, arguments, engineRoot);
    }
}
