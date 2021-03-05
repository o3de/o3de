/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

    static void ProcessDragEnter(QDragEnterEvent* event);
    static QStringList GetFileList(QDropEvent* event);

Q_SIGNALS:
    void OpenAssetImporterManager(const QStringList& fileList);

public Q_SLOTS:
    void OnStartAssetImporter();
    void OnStopAssetImporter();

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
