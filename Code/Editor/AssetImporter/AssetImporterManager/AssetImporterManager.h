/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <AssetImporter/UI/SelectDestinationDialog.h>
#endif

class QStringList;
class QFile;
class QFileDialog;

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
    void Exec(const QStringList& dragAndDropFileList, const QString& suggestedPath);

Q_SIGNALS:
    void StartAssetImporter();
    void StopAssetImporter();
    void AssetImportingComplete();

private Q_SLOTS:
    void reject();
    void CompleteAssetImporting(bool wasSuccessful = true);
    void OnDragAndDropFiles(const QStringList* fileList);
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
    QFileDialog* m_fileDialog{ nullptr };
    QString m_gameRootAbsPath;
    QString m_currentAbsolutePath;
    QString m_suggestedInitialPath;
};
