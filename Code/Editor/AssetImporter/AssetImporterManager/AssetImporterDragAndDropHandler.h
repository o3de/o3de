/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once


#if !defined(Q_MOC_RUN)

#include <QObject>
#include <QFileInfo>
#include <QString>

#include <AzCore/EBus/EBus.h>
#include <AzQtComponents/Buses/DragAndDrop.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzCore/std/smart_ptr/scoped_ptr.h>
#endif

class MainWindow;
class AssetImporterManager;


namespace AzToolsFramework
{
    namespace AssetDatabase
    {
        class AssetDatabaseConnection;
    }
}

class AssetImporterDragAndDropHandler
    : public QObject
    , public AzQtComponents::DragAndDropEventsBus::Handler
{
    Q_OBJECT

public:
    explicit AssetImporterDragAndDropHandler(QObject* parent, AssetImporterManager* const assetImporterManager);
    ~AssetImporterDragAndDropHandler();

    void DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void DropAtLocation(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context, QString& path) override;

    static void ProcessDragEnter(QDragEnterEvent* event);
    static QStringList GetFileList(QDropEvent* event);

Q_SIGNALS:
    void OpenAssetImporterManager(const QStringList& fileList);
    void OpenAssetImporterManagerWithSuggestedPath(const QStringList& fileList, const QString& destinationPath);

public Q_SLOTS:
    void OnStartAssetImporter();
    void OnStopAssetImporter();
    void OnAssetImportingComplete();

private:
    bool m_isAssetImporterRunning = false;
    AssetImporterManager* m_assetImporterManager;

    static bool ContainCrateFiles(QString path);
    static bool isCrateFile(QFileInfo fileInfo);

    // it is used because MainWindow's dropEvent will ask the Ebus to call the Drop() function in AssetImporterDragAndDropHandler.
    // That will cause the problem that even the crate objects are blocked by the DragEnter() in AssetImporterDragAndDropHandler,
    // it will still open the Asset Importer
    static bool m_dragAccepted;
};
