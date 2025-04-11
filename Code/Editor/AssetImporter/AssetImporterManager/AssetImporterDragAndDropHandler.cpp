/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "AssetImporterDragAndDropHandler.h"

// Qt
#include <QMimeData>

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>

// AzQtComponents
#include <AzQtComponents/DragAndDrop/MainWindowDragAndDrop.h>

// Editor
#include "AssetImporter/AssetImporterManager/AssetImporterManager.h"

bool AssetImporterDragAndDropHandler::m_dragAccepted = false;

AssetImporterDragAndDropHandler::AssetImporterDragAndDropHandler(QObject* parent, AssetImporterManager* const assetImporterManager)
    : QObject(parent)
    , m_assetImporterManager(assetImporterManager)
{
    AzQtComponents::DragAndDropEventsBus::Handler::BusConnect(AzQtComponents::DragAndDropContexts::EditorMainWindow);

    // They are used to prevent opening the Asset Importer by dragging and dropping files and folders to the Main Window when it is already running
    connect(m_assetImporterManager, &AssetImporterManager::StartAssetImporter, this, &AssetImporterDragAndDropHandler::OnStartAssetImporter);
    connect(m_assetImporterManager, &AssetImporterManager::StopAssetImporter, this, &AssetImporterDragAndDropHandler::OnStopAssetImporter);
    connect(m_assetImporterManager, &AssetImporterManager::AssetImportingComplete, this, &AssetImporterDragAndDropHandler::OnAssetImportingComplete);
}

AssetImporterDragAndDropHandler::~AssetImporterDragAndDropHandler()
{
    AzQtComponents::DragAndDropEventsBus::Handler::BusDisconnect(AzQtComponents::DragAndDropContexts::EditorMainWindow);
}

void AssetImporterDragAndDropHandler::DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& /*context*/)
{
    if (!m_isAssetImporterRunning)
    {
        ProcessDragEnter(event);
    }
}

void AssetImporterDragAndDropHandler::DropAtLocation(QDropEvent* event, [[maybe_unused]] AzQtComponents::DragAndDropContextBase& context, QString& path)
{
    if (!m_dragAccepted)
    {
        return;
    }

    QStringList fileList = GetFileList(event);

    if (!fileList.isEmpty())
    {
        Q_EMIT OpenAssetImporterManagerWithSuggestedPath(fileList, path);
    }

    // reset
    m_dragAccepted = false;
}

void AssetImporterDragAndDropHandler::Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& /*context*/)
{
    if (!m_dragAccepted)
    {
        return;
    }

    QStringList fileList = GetFileList(event);

    if (!fileList.isEmpty())
    {
        Q_EMIT OpenAssetImporterManager(fileList);
    }

    // reset
    m_dragAccepted = false;
}

void AssetImporterDragAndDropHandler::ProcessDragEnter(QDragEnterEvent* event)
{
    m_dragAccepted = false;

    const QMimeData* mimeData = event->mimeData();

    // if the event hasn't been accepted already and the mimeData hasUrls()
    if (event->isAccepted() || !mimeData->hasUrls())
    {
        return;
    }

    // prevent users from dragging and dropping files from the Asset Browser
    if (mimeData->hasFormat(AzToolsFramework::AssetBrowser::AssetBrowserEntry::GetMimeType()))
    {
        return;
    }

    QList<QUrl> urlList = mimeData->urls();
    int urlListSize = urlList.size();

    // runs through the file list first and checks for any "crate" files - if it finds ANY, return (and don't accept the event)
    for (int i = 0; i < urlListSize; ++i)
    {
        QUrl currentUrl = urlList.at(i);

        if (currentUrl.isLocalFile())
        {
            QString path = urlList.at(i).toLocalFile();

            if (ContainCrateFiles(path))
            {
                return;
            }
        }
    }

    for (int i = 0; i < urlListSize; ++i)
    {
        // Get the local file path
        QString path = urlList.at(i).toLocalFile();

        QDir dir(path);
        QString relativePath = dir.relativeFilePath(path);
        QString absPath = dir.absolutePath();

        // check if the files/folders are under the game root directory
        QDir gameRoot(Path::GetEditingGameDataFolder().c_str());
        QString gameRootAbsPath = gameRoot.absolutePath();

        if (absPath.startsWith(gameRootAbsPath, Qt::CaseInsensitive))
        {
            return;
        }

        QDirIterator it(absPath, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

        QFileInfo info(absPath);
        QString extension = info.completeSuffix();

        // if it's not an empty folder directory or if it's a file,
        // then allow the drag and drop process.
        // Otherwise, prevent users from dragging and dropping empty folders
        if (it.hasNext() || !extension.isEmpty())
        {
            // this is used in Drop()
            m_dragAccepted = true;
        }
    }

    // at this point, all files should be legal to be imported
    // since they are not in the database
    if (m_dragAccepted)
    {
        event->acceptProposedAction();
    }
}

QStringList AssetImporterDragAndDropHandler::GetFileList(QDropEvent* event)
{
    QStringList fileList;
    const QMimeData* mimeData = event->mimeData();

    QList<QUrl> urlList = mimeData->urls();

    for (int i = 0; i < urlList.size(); ++i)
    {
        QUrl currentUrl = urlList.at(i);

        if (currentUrl.isLocalFile())
        {
            QString path = urlList.at(i).toLocalFile();

            if (!ContainCrateFiles(path))
            {
                fileList.append(path);
            }
        }
    }

    return fileList;
}

void AssetImporterDragAndDropHandler::OnStartAssetImporter()
{
    m_isAssetImporterRunning = true;
}

void AssetImporterDragAndDropHandler::OnStopAssetImporter()
{
    m_isAssetImporterRunning = false;
}

void AssetImporterDragAndDropHandler::OnAssetImportingComplete()
{
    m_isAssetImporterRunning = false;
}

bool AssetImporterDragAndDropHandler::ContainCrateFiles(QString path)
{
    QFileInfo fileInfo(path);

    if (fileInfo.isFile())
    {
        return isCrateFile(fileInfo);
    }
    else
    {
        QDirIterator it(path, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);

        while (it.hasNext())
        {
            QString str = it.next();
            QFileInfo info(str);

            if (isCrateFile(info))
            {
                return true;
            }
        }
    }

    return false;
}

bool AssetImporterDragAndDropHandler::isCrateFile(QFileInfo fileInfo)
{
    return QStringLiteral("crate").compare(fileInfo.suffix(), Qt::CaseInsensitive) == 0;
}

#include <AssetImporter/AssetImporterManager/moc_AssetImporterDragAndDropHandler.cpp>
