/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "AssetImporterManager.h"

// Qt
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

// AzFramework
#include <AzFramework/Asset/AssetSystemBus.h>

// AzToolsFramework
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

// Editor
#include "AssetImporter/UI/FilesAlreadyExistDialog.h"
#include "AssetImporter/UI/ProcessingAssetsDialog.h"

namespace AssetImporterManagerPrivate
{
    const char* g_selectFilesPath = "AssetImporter/SelectFilesPath";
    const char* g_selectDestinationFilesPath = "AssetImporter/SelectDestinationFilesPath";
    const char* g_errorMessageBoxTitle = "File failed to process.";
    const char* g_crateError = "Crate files cannot be imported.";
    static const char* s_crateFileExtension = "crate";
};

AssetImporterManager::AssetImporterManager(QWidget* parent)
    : QObject(parent)
{
}

AssetImporterManager::~AssetImporterManager()
{
}

void AssetImporterManager::Exec()
{
    // tell the AssetImporterDragAndDropHandler that the Asset Importer now is running
    Q_EMIT StartAssetImporter();
    bool success = OnBrowseFiles();

    // prevent users from selecting crate files from the File Explorer and open the Asset Importer.
    if (!success)
    {
        reject();
    }
    else
    {
        OnOpenSelectDestinationDialog();
    }
}

void AssetImporterManager::Exec(const QStringList& dragAndDropFileList)
{
    // note: dragging and dropping an empty folder can also trigger this condition
    if (!dragAndDropFileList.isEmpty())
    {
        OnDragAndDropFiles(&dragAndDropFileList);

        // only open the Asset Importer when the folder contains correct type files
        if (!m_pathMap.isEmpty())
        {
            // tell the AssetImporterDragAndDropHandler that the Asset Importer now is running
            Q_EMIT StartAssetImporter();
            OnOpenSelectDestinationDialog();
        }
        else
        {
            reject();
        }
    }
}

// used to cancel actions and close the dialog
void AssetImporterManager::reject()
{
    m_pathMap.clear();
    m_destinationRootDirectory = "";
    Q_EMIT StopAssetImporter();
}

void AssetImporterManager::OnDragAndDropFiles(const QStringList* fileList)
{
    for (int i = 0; i < fileList->size(); ++i)
    {
        // if the list contains a crate file,
        // the whole process should stop
        if (!GetAndCheckAllFilesInFolder(fileList->at(i)))
        {
            QMessageBox::warning(AzToolsFramework::GetActiveWindow(), AssetImporterManagerPrivate::g_errorMessageBoxTitle, AssetImporterManagerPrivate::g_crateError);
            reject();
            return;
        }
    }
}

bool AssetImporterManager::OnBrowseFiles()
{
    QFileDialog fileDialog;
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setWindowModality(Qt::WindowModality::ApplicationModal);
    fileDialog.setViewMode(QFileDialog::Detail);
    fileDialog.setWindowTitle(tr("Select files to import"));
    fileDialog.setLabelText(QFileDialog::Accept, "Select");

    QSettings settings;
    QString currentAbsolutePath = settings.value(AssetImporterManagerPrivate::g_selectFilesPath).toString();

    QDir gameRoot(Path::GetEditingGameDataFolder().c_str());
    QString gameRootAbsPath = gameRoot.absolutePath();

    // Case 1: if currentAbsolutePath is empty at this point, that means this is the first time
    //         users using the Asset Importer, set the default directory to be users' PC's desktop.
    // Case 2: if the current folder directory stored in the registry doesn't exist anymore,
    //         that means users have removed the directory already (deleted or use the Move feature).
    // Case 3: if it's a directory under the game root folder, then in general,
    //         users have modified the folder directory in the registry. It should not be happening.
    if (currentAbsolutePath.isEmpty() || !QFile(currentAbsolutePath).exists() || currentAbsolutePath.startsWith(gameRootAbsPath, Qt::CaseInsensitive))
    {
        currentAbsolutePath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }

    fileDialog.setDirectory(currentAbsolutePath);

    if (!fileDialog.exec())
    {
        return false;
    }

    bool encounteredCrate = false;
    QStringList invalidFiles;

    for (const QString& path : fileDialog.selectedFiles())
    {
        QString fileName = GetFileName(path);
        QFileInfo info(path);
        QString extension = info.completeSuffix(); // extension without '.'

        if (QString(AssetImporterManagerPrivate::s_crateFileExtension).compare(extension, Qt::CaseInsensitive) != 0)
        {
            // prevent users from importing files under the game root directory
            if (path.startsWith(gameRootAbsPath, Qt::CaseInsensitive))
            {
                invalidFiles << fileName;
            }
            else
            {
                // store paths into the map.
                m_pathMap[path] = fileName;
            }
        }
        else
        {
            encounteredCrate = true;
        }
    }

    if (invalidFiles.size() > 0)
    {
        QString fileWarning = QString("Files cannot be imported into their own project. The following files will not be moved or copied:\n");
        fileWarning.append(invalidFiles.join(", "));
        fileWarning.append('.');
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), AssetImporterManagerPrivate::g_errorMessageBoxTitle, fileWarning);
    }
    if (encounteredCrate)
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(), AssetImporterManagerPrivate::g_errorMessageBoxTitle, AssetImporterManagerPrivate::g_crateError);
    }

    currentAbsolutePath = fileDialog.directory().absolutePath();
    settings.setValue(AssetImporterManagerPrivate::g_selectFilesPath, currentAbsolutePath);

    // prevent users from selecting crate files from the File Explorer and open the Asset Importer.
    return (m_pathMap.size() > 0);
}

void AssetImporterManager::OnBrowseDestinationFilePath(QLineEdit* destinationLineEdit)
{
    QFileDialog fileDialog;
    fileDialog.setOption(QFileDialog::ShowDirsOnly, true);
    fileDialog.setViewMode(QFileDialog::List);
    fileDialog.setWindowModality(Qt::WindowModality::ApplicationModal);
    fileDialog.setWindowTitle(tr("Select import destination"));

    QSettings settings;
    QString currentDestination = settings.value(AssetImporterManagerPrivate::g_selectDestinationFilesPath).toString();

    QDir gameRoot(Path::GetEditingGameDataFolder().c_str());
    QString gameRootAbsPath = gameRoot.absolutePath();

    // Case 1: if currentDestination is empty at this point, that means this is the first time
    //         users using the Asset Importer, set the default directory to be the current game project's root folder
    // Case 2: if the current folder directory stored in the registry doesn't exist anymore,
    //         that means users have removed the directory already (deleted or use the Move feature).
    // Case 3: if it's a directory outside of the game root folder, then in general,
    //         users have modified the folder directory in the registry. It should not be happening.
    if (currentDestination.isEmpty() || !QDir(currentDestination).exists() || !currentDestination.startsWith(gameRootAbsPath, Qt::CaseInsensitive))
    {
        currentDestination = gameRootAbsPath;
    }

    fileDialog.setDirectory(currentDestination);

    // The default file path is the game project root folder.
    // After that, the default file path will be the previous opened folder path.
    connect(&fileDialog, &QFileDialog::directoryEntered, this, [&fileDialog, &gameRoot, &gameRootAbsPath](const QString& path)
        {
            // get current relative path
            QString relativePath = gameRoot.relativeFilePath(path);

            // Guard against navigating outside of the project folder. Lambda used as the dialog had to be captured.
            // checking the directory and prevent users from changing the directory outside of the game root
            if (!path.startsWith(gameRootAbsPath, Qt::CaseInsensitive) || (relativePath.length() > 2 && relativePath[0] == '.' && relativePath[1] == '.'))
            {
                fileDialog.setDirectory(gameRoot);
            }
        });

    if (!fileDialog.exec())
    {
        return;
    }

    // users can only select one folder at a time, so the index is always 0.
    // This fixes the issue that QFileDialog does not select the highlighted folder
    QString destinationDirectory = fileDialog.selectedFiles().at(0);

    OnSetDestinationDirectory(destinationDirectory);

    destinationLineEdit->setText(destinationDirectory);
}

// Copy + Paste
void AssetImporterManager::OnCopyFiles()
{
    m_importMethod = ImportFilesMethod::CopyFiles;
    ProcessCopyFiles();
}

// Cut + Paste
void AssetImporterManager::OnMoveFiles()
{
    m_importMethod = ImportFilesMethod::MoveFiles;
    ProcessMoveFiles();
}

bool AssetImporterManager::OnOverwriteFiles(QString relativePath, QString oldAbsolutePath)
{
    // this is the absolute path in the destination folder
    QString destinationAbsolutePath = GenerateAbsolutePath(relativePath);

    return Overwrite(relativePath, oldAbsolutePath, destinationAbsolutePath);
}

bool AssetImporterManager::OnKeepBothFiles(QString relativePath, QString oldAbsolutePath)
{
    // this is the absolute path in the destination folder
    QString destinationAbsolutePath = GenerateAbsolutePath(relativePath);

    QString subPath = QFileInfo(destinationAbsolutePath).absoluteDir().absolutePath();

    QFileInfo info(destinationAbsolutePath);
    QString extension = info.completeSuffix(); // extension without '.'
    QString fileName = info.baseName(); //file name without extension

    int number = 1;
    int index = destinationAbsolutePath.indexOf(extension);

    QString newFileName = CreateFileNameWithNumber(number, fileName, index, extension);
    QString newDestinationAbsolutePath = subPath + '/' + newFileName;

    while (QFile(newDestinationAbsolutePath).exists())
    {
        number++;
        newFileName = CreateFileNameWithNumber(number, fileName, index, extension);
        newDestinationAbsolutePath = subPath + '/' + newFileName;
    }

    if (m_importMethod == ImportFilesMethod::CopyFiles)
    {
        return Copy(relativePath, oldAbsolutePath, newDestinationAbsolutePath);
    }
    else if (m_importMethod == ImportFilesMethod::MoveFiles)
    {
        return Move(relativePath, oldAbsolutePath, newDestinationAbsolutePath);
    }

    return false;
}

void AssetImporterManager::OnOpenLogDialog()
{
    AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::ShowAssetProcessor);
    reject();
}

void AssetImporterManager::OnSetDestinationDirectory(QString destinationDirectory)
{
    QSettings settings;
    QString currentDestination = settings.value(AssetImporterManagerPrivate::g_selectDestinationFilesPath).toString();

    m_destinationRootDirectory = (!destinationDirectory.isEmpty()) ? destinationDirectory : currentDestination;
    settings.setValue(AssetImporterManagerPrivate::g_selectDestinationFilesPath, destinationDirectory);
}

void AssetImporterManager::OnOpenSelectDestinationDialog()
{
    QWidget* mainWindow = nullptr;
    AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);

    QString numberOfFilesMessage = m_pathMap.size() == 1 ? QString(tr("Importing 1 asset")) : QString(tr("Importing %1 assets").arg(m_pathMap.size()));

    SelectDestinationDialog selectDestinationDialog(numberOfFilesMessage, mainWindow);

    // Browse Destination File Path
    connect(&selectDestinationDialog, &SelectDestinationDialog::BrowseDestinationPath, this, &AssetImporterManager::OnBrowseDestinationFilePath);
    connect(&selectDestinationDialog, &SelectDestinationDialog::DoCopyFiles, this, &AssetImporterManager::OnCopyFiles);
    connect(&selectDestinationDialog, &SelectDestinationDialog::DoMoveFiles, this, &AssetImporterManager::OnMoveFiles);
    connect(&selectDestinationDialog, &SelectDestinationDialog::Cancel, this, &AssetImporterManager::reject);
    connect(&selectDestinationDialog, &SelectDestinationDialog::SetDestinationDirectory, this, &AssetImporterManager::OnSetDestinationDirectory);
    selectDestinationDialog.exec();
}

ProcessFilesMethod AssetImporterManager::OnOpenFilesAlreadyExistDialog(QString message, int numberOfFiles)
{
    ProcessFilesMethod processMethod = ProcessFilesMethod::Default;

    // make sure the dialog is opened in front of the Editor main window
    QWidget* mainWindow = nullptr;
    AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);

    FilesAlreadyExistDialog filesAlreadyExistDialog(message, numberOfFiles, mainWindow);

    bool applyToAll = false;

    connect(&filesAlreadyExistDialog, &FilesAlreadyExistDialog::ApplyActionToAllFiles, this, [&applyToAll](bool result)
        {
            applyToAll = result;
        });

    connect(&filesAlreadyExistDialog, &FilesAlreadyExistDialog::OverWriteFiles, this, [this, &processMethod, &applyToAll]()
        {
            processMethod = UpdateProcessFileMethod(ProcessFilesMethod::OverwriteFile, applyToAll);
        });

    connect(&filesAlreadyExistDialog, &FilesAlreadyExistDialog::KeepBothFiles, this, [this, &processMethod, &applyToAll]()
        {
            processMethod = UpdateProcessFileMethod(ProcessFilesMethod::KeepBothFile, applyToAll);
        });

    connect(&filesAlreadyExistDialog, &FilesAlreadyExistDialog::SkipCurrentProcess, this, [this, &processMethod, &applyToAll]()
        {
            processMethod = UpdateProcessFileMethod(ProcessFilesMethod::SkipProcessingFile, applyToAll);
        });

    connect(&filesAlreadyExistDialog, &FilesAlreadyExistDialog::CancelAllProcesses, this, [&processMethod]()
        {
            processMethod = ProcessFilesMethod::Cancel;
        });

    if (!applyToAll && processMethod != ProcessFilesMethod::Cancel)
    {
        filesAlreadyExistDialog.exec();
    }

    return processMethod;
}

ProcessFilesMethod AssetImporterManager::UpdateProcessFileMethod(ProcessFilesMethod processMethod, bool applyToAll)
{
    if (applyToAll)
    {
        switch (processMethod)
        {
        case ProcessFilesMethod::OverwriteFile:
            processMethod = ProcessFilesMethod::OverwriteAllFiles;
            break;
        case ProcessFilesMethod::KeepBothFile:
            processMethod = ProcessFilesMethod::KeepBothAllFiles;
            break;
        case ProcessFilesMethod::SkipProcessingFile:
            processMethod = ProcessFilesMethod::SkipProcessingAllFiles;
        }
    }

    return processMethod;
}

bool AssetImporterManager::ProcessFileMethod(ProcessFilesMethod processMethod, QString relativePath, QString oldAbsolutePath)
{
    switch (processMethod)
    {
    case ProcessFilesMethod::OverwriteFile:
    case ProcessFilesMethod::OverwriteAllFiles:
        return OnOverwriteFiles(relativePath, oldAbsolutePath);
    case ProcessFilesMethod::KeepBothFile:
    case ProcessFilesMethod::KeepBothAllFiles:
        return OnKeepBothFiles(relativePath, oldAbsolutePath);
    case ProcessFilesMethod::SkipProcessingAllFiles:
        return false;
    }

    return false;
}

void AssetImporterManager::OnOpenProcessingAssetsDialog(int numberOfProcessedFiles)
{
    // make sure the dialog is opened in front of the Editor main window
    QWidget* mainWindow = nullptr;
    AzToolsFramework::EditorRequestBus::BroadcastResult(mainWindow, &AzToolsFramework::EditorRequests::GetMainWindow);

    ProcessingAssetsDialog processingAssetsDialog(numberOfProcessedFiles, mainWindow);
    connect(&processingAssetsDialog, &ProcessingAssetsDialog::OpenLogDialog, this, &AssetImporterManager::OnOpenLogDialog);
    connect(&processingAssetsDialog, &ProcessingAssetsDialog::CloseProcessingAssetsDialog, this, &AssetImporterManager::reject);

    processingAssetsDialog.exec();
}

void AssetImporterManager::ProcessCopyFiles()
{
    int numberOfFiles = m_pathMap.size();
    int numberOfProcessedFiles = 0;

    ProcessFilesMethod processMethod = ProcessFilesMethod::Default;

    for (int i = 0; i < m_pathMap.size(); ++i)
    {
        QString relativePath = m_pathMap.values().at(i);
        QString oldAbsolutePath = m_pathMap.keys().at(i);

        // this is the absolute path in the destination folder
        QString destinationAbsolutePath = GenerateAbsolutePath(relativePath);

        // check if the file exists in the destination folder
        if (!QFile::exists(destinationAbsolutePath))
        {
            if (Copy(relativePath, oldAbsolutePath, destinationAbsolutePath))
            {
                numberOfProcessedFiles++;
            }
        }
        else
        {
            if (processMethod == ProcessFilesMethod::Default ||
                processMethod == ProcessFilesMethod::OverwriteFile ||
                processMethod == ProcessFilesMethod::KeepBothFile ||
                processMethod == ProcessFilesMethod::SkipProcessingFile)
            {
                QString fileName = GetFileName(oldAbsolutePath);
                QString message = QString("The destination already has a file named \"%1\". What would you like to do?").arg(fileName);

                processMethod = OnOpenFilesAlreadyExistDialog(message, numberOfFiles);
            }

            if (ProcessFileMethod(processMethod, relativePath, oldAbsolutePath))
            {
                numberOfProcessedFiles++;
            }
        }

        numberOfFiles--;
    }

    if (numberOfProcessedFiles > 0)
    {
        OnOpenProcessingAssetsDialog(numberOfProcessedFiles);
    }
    else
    {
        reject();
    }
}

void AssetImporterManager::ProcessMoveFiles()
{
    int numberOfFiles = m_pathMap.size();
    int numberOfProcessedFiles = 0;

    ProcessFilesMethod processMethod = ProcessFilesMethod::Default;

    for (int i = 0; i < m_pathMap.size(); ++i)
    {
        QString relativePath = m_pathMap.values().at(i);
        QString oldAbsolutePath = m_pathMap.keys().at(i);

        // this is the absolute path in the destination folder
        QString destinationAbsolutePath = GenerateAbsolutePath(relativePath);

        // check if the file exists in the destination folder
        if (!QFile::exists(destinationAbsolutePath))
        {
            if (Move(relativePath, oldAbsolutePath, destinationAbsolutePath))
            {
                numberOfProcessedFiles++;
            }
        }
        else
        {
            if (processMethod == ProcessFilesMethod::Default ||
                processMethod == ProcessFilesMethod::OverwriteFile ||
                processMethod == ProcessFilesMethod::KeepBothFile ||
                processMethod == ProcessFilesMethod::SkipProcessingFile)
            {
                QString fileName = GetFileName(oldAbsolutePath);
                QString message = QString("The destination already has a file named \"%1\". What would you like to do?").arg(fileName);

                processMethod = OnOpenFilesAlreadyExistDialog(message, numberOfFiles);
            }

            if (ProcessFileMethod(processMethod, relativePath, oldAbsolutePath))
            {
                numberOfProcessedFiles++;
            }
        }

        numberOfFiles--;
    }

    if (numberOfProcessedFiles > 0)
    {
        OnOpenProcessingAssetsDialog(numberOfProcessedFiles);
    }
    else
    {
        reject();
    }
}

bool AssetImporterManager::Copy(QString relativePath, QString oldAbsolutePath, QString destinationAbsolutePath)
{
    QString fileName = GetFileName(destinationAbsolutePath);
    QString subPath = QFileInfo(destinationAbsolutePath).absoluteDir().absolutePath();
    QDir dir;

    bool directoryExistedAlready = QDir(subPath).exists();
    if (!directoryExistedAlready)
    {
        dir.mkpath(subPath);
    }

    QString newDestinationAbsolutePath = subPath;
    newDestinationAbsolutePath = newDestinationAbsolutePath.append('/' + fileName);

    // Copy the file from the old path to the new path
    if (!QFile::copy(oldAbsolutePath, newDestinationAbsolutePath))
    {
        QString reason = tr("an unknown issue occurred.");

        if (!directoryExistedAlready)
        {
            dir.rmdir(subPath);
        }

        // if the original files got deleted at this condition
        if (!QFile(oldAbsolutePath).exists())
        {
            reason = tr("%1 no longer exists.").arg(fileName);
        }
        // if users manually copy the file into the destination folder at this condition
        if (QFile(newDestinationAbsolutePath).exists())
        {
            reason = tr("%1 already exists in the target directory.").arg(fileName);
        }

        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), AssetImporterManagerPrivate::g_errorMessageBoxTitle, QObject::tr("We're sorry, but the file failed to process because %1").arg(reason));
        return false;
    }

    // set the destination file to be writable by user himself/herself if it's read-only
    QFile destinationFile(newDestinationAbsolutePath);
    SetDestinationFileWritable(destinationFile);

    return true;
}

bool AssetImporterManager::Move(QString relativePath, QString oldAbsolutePath, QString destinationAbsolutePath)
{
    QString fileName = GetFileName(destinationAbsolutePath);
    QString subPath = QFileInfo(destinationAbsolutePath).absoluteDir().absolutePath();
    QString newDestinationAbsolutePath = subPath;
    QDir dir;

    bool directoryExistedAlready = QDir(subPath).exists();
    if (!directoryExistedAlready)
    {
        dir.mkpath(subPath);
    }

    if (QFile::rename(oldAbsolutePath, newDestinationAbsolutePath.append('/' + fileName)))
    {
        QString oldFileName = GetFileName(oldAbsolutePath);

        // Only remove the old directory if the relative path is the file name itself.
        // That also means users are dragging and dropping files, but not a folder containing those files.
        if (oldFileName.compare(relativePath) != 0)
        {
            RemoveOldPath(oldAbsolutePath, relativePath);
        }

        // set the destination file to be writable by user himself/herself if it's read-only
        QFile destinationFile(newDestinationAbsolutePath);
        SetDestinationFileWritable(destinationFile);

        return true;
    }
    else
    {
        QString reason = tr("an unknown issue occurred.");
        QString oldFileName = GetFileName(oldAbsolutePath);

        if (!directoryExistedAlready)
        {
            dir.rmdir(subPath);
        }

        // if the original files got deleted at this condition
        if (!QFile(oldAbsolutePath).exists())
        {
            reason = tr("%1 no longer exists.").arg(oldFileName);
        }
        // if users manually copy the file into the destination folder at this condition
        if (QFile(newDestinationAbsolutePath).exists())
        {
            reason = tr("%1 already exists in the target directory.").arg(oldFileName);
        }

        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), AssetImporterManagerPrivate::g_errorMessageBoxTitle, QObject::tr("We're sorry, but the file failed to process because %1").arg(reason));
        return false;
    }
}

bool AssetImporterManager::Overwrite(QString relativePath, QString oldAbsolutePath, QString destinationAbsolutePath)
{
    QFile newFile(destinationAbsolutePath);
    QFile oldFile(oldAbsolutePath);

    // double check if paths are valid
    if ((!oldFile.open(QIODevice::ReadOnly)))
    {
        QString fileName = GetFileName(oldAbsolutePath);
        QString reason = tr("%1 no longer exists.").arg(fileName);
        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), AssetImporterManagerPrivate::g_errorMessageBoxTitle, QObject::tr("We're sorry, but the file failed to process because %1").arg(reason));
        return false;
    }

    if (!newFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QString reason = tr("We're sorry, but the file failed to process.");
        QString fileName = GetFileName(destinationAbsolutePath);

        if (!newFile.exists())
        {
            reason = tr("%1 from the destination directory is removed.").arg(fileName);
        }
        else
        {
            reason = tr("%1 from the destination directory cannot be overwritten.").arg(fileName);
        }

        QMessageBox::critical(AzToolsFramework::GetActiveWindow(), AssetImporterManagerPrivate::g_errorMessageBoxTitle, QObject::tr("We're sorry, but the file failed to process because %1").arg(reason));
        return false;
    }

    QDataStream dataStream(&oldFile);
    QDataStream out(&newFile);

    int bufferSize = 1024 * 1024;
    char* buffer = new char[bufferSize];

    while (!dataStream.atEnd())
    {
        int bytesRead = dataStream.readRawData(buffer, bufferSize);
        out.writeRawData(buffer, bytesRead);
    }

    delete[] buffer;
    oldFile.close();
    newFile.close();

    // if it's the move file method, got to remove the original files
    if (m_importMethod == ImportFilesMethod::MoveFiles)
    {
        QString fileName = GetFileName(oldAbsolutePath);
        QFile file(oldAbsolutePath);
        QDir absoluteDir = QFileInfo(oldAbsolutePath).absoluteDir();

        if (file.exists())
        {
            // if the original file is read-only,
            // then it got to be writable in order to be deleted successfully
            SetDestinationFileWritable(file);
            absoluteDir.remove(fileName);
        }

        // Only remove the old directory if the relative path is the file name itself.
        // That also means users are dragging and dropping files, but not a folder containing those files.
        if (fileName.compare(relativePath) != 0)
        {
            RemoveOldPath(oldAbsolutePath, relativePath);
        }
    }

    return true;
}

bool AssetImporterManager::GetAndCheckAllFilesInFolder(QString path)
{
    QString formattedPath = path;

    // Paths ending with '/' return from QFileInfo().fileName() with a value of ""
    // Strip a trailing slash so we can correctly get rootFolderName on all platforms
    if (formattedPath.endsWith("/"))
    {
        formattedPath.truncate(formattedPath.lastIndexOf(QChar('/')));
    }

    QString rootFolderName = GetFileName(formattedPath);
    QDirIterator it(formattedPath, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
    QFileInfo info(formattedPath);

    if (!info.isDir() && !it.hasNext() && info.exists())
    {
        m_pathMap[formattedPath] = rootFolderName;
        return true;
    }

    // Get the index of the last sub folder name in the path
    QStringList directoryNameList = formattedPath.split('/');
    int lastFolderIndex = directoryNameList.size() - 1;

    QString pathToBeRelativeTo = directoryNameList.mid(0, lastFolderIndex).join('/');

    while (it.hasNext())
    {
        QString absolutePath = it.next();
        QFileInfo absoluteInfo(absolutePath);
        QString extension = absoluteInfo.completeSuffix();

        if (QString(AssetImporterManagerPrivate::s_crateFileExtension).compare(extension, Qt::CaseInsensitive) == 0)
        {
            return false;
        }

        Q_ASSERT(absolutePath.startsWith(pathToBeRelativeTo));
        QString relativePath = absolutePath.mid(pathToBeRelativeTo.size() + 1);
        m_pathMap[absolutePath] = relativePath;
    }

    return true;
}

void AssetImporterManager::RemoveOldPath(QString oldAbsolutePath, QString oldRelativePath)
{
    QDir absoluteDir = QFileInfo(oldAbsolutePath).absoluteDir();
    QStringList directoryList = oldRelativePath.split('/');

    // remove each folder from the leave to the root, based on the relative path
    for (int i = 0; i < directoryList.size(); ++i)
    {
        QString currentDir = absoluteDir.path();
        absoluteDir.rmpath(currentDir);
        absoluteDir.cdUp();
    }
}

void AssetImporterManager::SetDestinationFileWritable(QFile& destinationFile)
{
    if (destinationFile.open(QIODevice::ReadOnly))
    {
        destinationFile.setPermissions(QFile::WriteOwner | destinationFile.permissions());
    }

    destinationFile.close();
}

QString AssetImporterManager::CreateFileNameWithNumber(int number, QString fileName, int index, QString extension)
{
    QString newFileName;

    newFileName = fileName.isEmpty() ? fileName : fileName.left(index);

    newFileName += "(" + QString::number(number) + ")";

    if (extension.size() > 0)
    {
        newFileName += "." + extension;
    }

    return newFileName;
}

QString AssetImporterManager::GenerateAbsolutePath(QString relativePath)
{
    return QDir(m_destinationRootDirectory).absoluteFilePath(relativePath);
}

QString AssetImporterManager::GetFileName(QString path)
{
    return QFileInfo(path).fileName();
}

#include <AssetImporter/AssetImporterManager/moc_AssetImporterManager.cpp>
