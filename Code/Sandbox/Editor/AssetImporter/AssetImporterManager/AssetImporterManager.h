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
#include <QMainWindow>
#include <AssetImporter/UI/SelectDestinationDialog.h>
#endif

class QStringList;
class QFile;

enum class ImportFilesMethod
{
    CopyFiles,
    MoveFiles
};

enum class ProcessFilesMethod
{
    OverwriteFile,
    KeepBothFile,
    SkipProcessingFile,
    OverwriteAllFiles,
    KeepBothAllFiles,
    SkipProcessingAllFiles,
    Cancel,

    Default
};

class AssetImporterManager
    : public QObject
{
    Q_OBJECT

public:
    explicit AssetImporterManager(QWidget* parent = nullptr);
    ~AssetImporterManager();

    // Modal, but blocking.
    void Exec(); // for browsing files
    void Exec(const QStringList& dragAndDropFileList); // for drag and drop

Q_SIGNALS:
    void StartAssetImporter();
    void StopAssetImporter();

private Q_SLOTS:
    void reject();
    void OnDragAndDropFiles(const QStringList* fileList);
    bool OnBrowseFiles();
    void OnBrowseDestinationFilePath(QLineEdit* destinationLineEdit);
    void OnCopyFiles();
    void OnMoveFiles();
    bool OnOverwriteFiles(QString relativePath, QString oldAbsolutePath);
    bool OnKeepBothFiles(QString relativePath, QString oldAbsolutePath);
    void OnOpenLogDialog();
    void OnSetDestinationDirectory(QString destinationDirectory);

private:
    void OnOpenSelectDestinationDialog();

    ProcessFilesMethod OnOpenFilesAlreadyExistDialog(QString message, int numberOfFiles);
    ProcessFilesMethod UpdateProcessFileMethod(ProcessFilesMethod processMethod, bool applyToAll);
    bool ProcessFileMethod(ProcessFilesMethod processMethod, QString relativePath, QString oldAbsolutePath);
    
    void OnOpenProcessingAssetsDialog(int numberOfProcessedFiles);

    void ProcessCopyFiles();
    void ProcessMoveFiles();

    bool Copy(QString relativePath, QString oldAbsolutePath, QString destinationAbsolutePath);
    bool Move(QString relativePath, QString oldAbsolutePath, QString destinationAbsolutePath);
    bool Overwrite(QString relativePath, QString oldAbsolutePath, QString destinationAbsolutePath);

    bool GetAndCheckAllFilesInFolder(QString path);
    void RemoveOldPath(QString oldAbsolutePath, QString oldRelativePath);
    void SetDestinationFileWritable(QFile& destinationFile);

    QString CreateFileNameWithNumber(int number, QString fileName, int index, QString extension);
    QString GenerateAbsolutePath(QString relativePath);
    QString GetFileName(QString path);

    ImportFilesMethod m_importMethod;

    // Key = absolute path, Value = relative path
    QMap<QString, QString> m_pathMap;
    QString m_destinationRootDirectory;
};
