/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>
#include <PythonBindingsInterface.h>

#include <QFileDialog>
#include <QDir>
#include <QtMath>
#include <QMessageBox>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QGuiApplication>
#include <QProgressDialog>
#include <QSpacerItem>
#include <QGridLayout>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        static bool WarnDirectoryOverwrite(const QString& path, QWidget* parent)
        {
            if (!QDir(path).isEmpty())
            {
                QMessageBox::StandardButton warningResult = QMessageBox::warning(
                    parent, QObject::tr("Overwrite Directory"),
                    QObject::tr("Directory is not empty! Are you sure you want to overwrite it?"), QMessageBox::No | QMessageBox::Yes);

                if (warningResult != QMessageBox::Yes)
                {
                    return false;
                }
            }

            return true;
        }

        static bool IsDirectoryDescedent(const QString& possibleAncestorPath, const QString& possibleDecedentPath)
        {
            QDir ancestor(possibleAncestorPath);
            QDir descendent(possibleDecedentPath);

            do
            {
                if (ancestor == descendent)
                {
                    return true;
                }

                descendent.cdUp();
            } while (!descendent.isRoot());

            return false;
        }

        typedef AZStd::function<void(/*fileCount=*/int, /*totalSizeInBytes=*/int)> StatusFunction;
        static void RecursiveGetAllFiles(const QDir& directory, QStringList& outFileList, qint64& outTotalSizeInBytes, StatusFunction statusCallback)
        {
            const QStringList entries = directory.entryList(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            for (const QString& entryPath : entries)
            {
                const QString filePath = QDir::toNativeSeparators(QString("%1/%2").arg(directory.path()).arg(entryPath));
                QFileInfo fileInfo(filePath);

                if (fileInfo.isDir())
                {
                    QDir subDirectory(filePath);
                    RecursiveGetAllFiles(subDirectory, outFileList, outTotalSizeInBytes, statusCallback);
                }
                else
                {
                    outFileList.push_back(filePath);
                    outTotalSizeInBytes += fileInfo.size();

                    const int updateStatusEvery = 64;
                    if (outFileList.size() % updateStatusEvery == 0)
                    {
                        statusCallback(outFileList.size(), outTotalSizeInBytes);
                    }
                }
            }
        }

        static bool CopyDirectory(QProgressDialog* progressDialog,
            const QString& origPath,
            const QString& newPath,
            QStringList& filesToCopy,
            int& outNumCopiedFiles,
            qint64 totalSizeToCopy,
            qint64& outCopiedFileSize,
            bool& showIgnoreFileDialog)
        {
            QDir original(origPath);
            if (!original.exists())
            {
                return false;
            }

            for (QString directory : original.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                if (progressDialog->wasCanceled())
                {
                    return false;
                }

                QString newDirectoryPath = newPath + QDir::separator() + directory;
                original.mkpath(newDirectoryPath);

                if (!CopyDirectory(progressDialog, origPath + QDir::separator() + directory,
                    newDirectoryPath, filesToCopy, outNumCopiedFiles, totalSizeToCopy, outCopiedFileSize, showIgnoreFileDialog))
                {
                    return false;
                }
            }

            QLocale locale;
            const float progressDialogRangeHalf = qFabs(progressDialog->maximum() - progressDialog->minimum()) * 0.5f;
            for (QString file : original.entryList(QDir::Files))
            {
                if (progressDialog->wasCanceled())
                {
                    return false;
                }

                // Progress window update
                {
                    // Weight in the number of already copied files as well as the copied bytes to get a better progress indication
                    // for cases combining many small files and some really large files.
                    const float normalizedNumFiles = static_cast<float>(outNumCopiedFiles) / filesToCopy.count();
                    const float normalizedFileSize = static_cast<float>(outCopiedFileSize) / totalSizeToCopy;
                    const int progress = normalizedNumFiles * progressDialogRangeHalf + normalizedFileSize * progressDialogRangeHalf;
                    progressDialog->setValue(progress);

                    const QString copiedFileSizeString = locale.formattedDataSize(outCopiedFileSize);
                    const QString totalFileSizeString = locale.formattedDataSize(totalSizeToCopy);
                    progressDialog->setLabelText(QString("Coping file %1 of %2 (%3 of %4) ...").arg(QString::number(outNumCopiedFiles),
                        QString::number(filesToCopy.count()),
                        copiedFileSizeString,
                        totalFileSizeString));
                    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
                }

                const QString toBeCopiedFilePath = origPath + QDir::separator() + file;
                const QString copyToFilePath = newPath + QDir::separator() + file;
                if (!QFile::copy(toBeCopiedFilePath, copyToFilePath))
                {
                    // Let the user decide to ignore files that failed to copy or cancel the whole operation.
                    if (showIgnoreFileDialog)
                    {
                        QMessageBox ignoreFileMessageBox;
                        const QString text = QString("Cannot copy <b>%1</b>.<br><br>"
                            "Source: %2<br>"
                            "Destination: %3<br><br>"
                            "Press <b>Yes</b> to ignore the file, <b>YesToAll</b> to ignore all upcoming non-copyable files or "
                            "<b>Cancel</b> to abort duplicating the project.").arg(file, toBeCopiedFilePath, copyToFilePath);

                        ignoreFileMessageBox.setModal(true);
                        ignoreFileMessageBox.setWindowTitle("Cannot copy file");
                        ignoreFileMessageBox.setText(text);
                        ignoreFileMessageBox.setIcon(QMessageBox::Question);
                        ignoreFileMessageBox.setStandardButtons(QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::Cancel);

                        int ignoreFile = ignoreFileMessageBox.exec();
                        if (ignoreFile == QMessageBox::YesToAll)
                        {
                            showIgnoreFileDialog = false;
                            continue;
                        }
                        else if (ignoreFile == QMessageBox::Yes)
                        {
                            continue;
                        }
                        else
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    outNumCopiedFiles++;

                    QFileInfo fileInfo(toBeCopiedFilePath);
                    outCopiedFileSize += fileInfo.size();
                }
            }

            return true;
        }

        bool AddProjectDialog(QWidget* parent)
        {
            QString path = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(parent, QObject::tr("Select Project Directory")));
            if (!path.isEmpty())
            {
                return RegisterProject(path);
            }

            return false;
        }

        bool RegisterProject(const QString& path)
        {
            return PythonBindingsInterface::Get()->AddProject(path);
        }

        bool UnregisterProject(const QString& path)
        {
            return PythonBindingsInterface::Get()->RemoveProject(path);
        }

        bool CopyProjectDialog(const QString& origPath, QWidget* parent)
        {
            bool copyResult = false;

            QDir parentOrigDir(origPath);
            parentOrigDir.cdUp();
            QString newPath = QDir::toNativeSeparators(
                QFileDialog::getExistingDirectory(parent, QObject::tr("Select New Project Directory"), parentOrigDir.path()));
            if (!newPath.isEmpty())
            {
                if (!WarnDirectoryOverwrite(newPath, parent))
                {
                    return false;
                }

                copyResult = CopyProject(origPath, newPath, parent);
            }

            return copyResult;
        }

        bool CopyProject(const QString& origPath, const QString& newPath, QWidget* parent)
        {
            // Disallow copying from or into subdirectory
            if (IsDirectoryDescedent(origPath, newPath) || IsDirectoryDescedent(newPath, origPath))
            {
                return false;
            }

            QStringList filesToCopy;
            qint64 totalSizeInBytes = 0;

            QProgressDialog* progressDialog = new QProgressDialog(parent);
            progressDialog->setAutoClose(true);
            progressDialog->setValue(0);
            progressDialog->setRange(0, 1000);
            progressDialog->setModal(true);
            progressDialog->setWindowTitle(QObject::tr("Copying project ..."));
            progressDialog->show();

            QLocale locale;
            RecursiveGetAllFiles(origPath, filesToCopy, totalSizeInBytes, [=](int fileCount, int sizeInBytes)
                {
                    // Create a human-readable version of the file size.
                    const QString fileSizeString = locale.formattedDataSize(sizeInBytes);

                    progressDialog->setLabelText(QString("%1 ... %2 %3, %4 %5.")
                        .arg(QObject::tr("Indexing files"))
                        .arg(QString::number(fileCount))
                        .arg(QObject::tr("files found"))
                        .arg(fileSizeString)
                        .arg(QObject::tr("to copy")));
                    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
                });

            int numFilesCopied = 0;
            qint64 copiedFileSize = 0;

            // Phase 1: Copy files
            bool showIgnoreFileDialog = true;
            bool success = CopyDirectory(progressDialog, origPath, newPath, filesToCopy, numFilesCopied, totalSizeInBytes, copiedFileSize, showIgnoreFileDialog);
            if (success)
            {
                // Phase 2: Register project
                success = RegisterProject(newPath);
            }

            if (!success)
            {
                progressDialog->setLabelText(QObject::tr("Duplicating project failed/cancelled, removing already copied files ..."));
                qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

                DeleteProjectFiles(newPath, true);
            }

            progressDialog->deleteLater();
            return success;
        }

        bool DeleteProjectFiles(const QString& path, bool force)
        {
            QDir projectDirectory(path);
            if (projectDirectory.exists())
            {
                // Check if there is an actual project hereor just force it
                if (force || PythonBindingsInterface::Get()->GetProject(path).IsSuccess())
                {
                    return projectDirectory.removeRecursively();
                }
            }

            return false;
        }

        bool MoveProject(QString origPath, QString newPath, QWidget* parent, bool ignoreRegister)
        {
            origPath = QDir::toNativeSeparators(origPath);
            newPath = QDir::toNativeSeparators(newPath);

            if (!WarnDirectoryOverwrite(newPath, parent) || (!ignoreRegister && !UnregisterProject(origPath)))
            {
                return false;
            }

            QDir newDirectory(newPath);
            if (!newDirectory.removeRecursively())
            {
                return false;
            }
            if (!newDirectory.rename(origPath, newPath))
            {
                // Likely failed because trying to move to another partition, try copying
                if (!CopyProject(origPath, newPath, parent))
                {
                    return false;
                }

                DeleteProjectFiles(origPath, true);
            }

            if (!ignoreRegister && !RegisterProject(newPath))
            {
                return false;
            }

            return true;
        }

        bool ReplaceFile(const QString& origFile, const QString& newFile, QWidget* parent, bool interactive)
        {
            QFileInfo original(origFile);
            if (original.exists())
            {
                if (interactive)
                {
                    QMessageBox::StandardButton warningResult = QMessageBox::warning(
                        parent,
                        QObject::tr("Overwrite File?"),
                        QObject::tr("Replacing this will overwrite the current file on disk. Are you sure?"),
                        QMessageBox::No | QMessageBox::Yes);

                    if (warningResult == QMessageBox::No)
                    {
                        return false;
                    }
                }

                if (!QFile::remove(origFile))
                {
                    return false;
                }
            }

            if (!QFile::copy(newFile, origFile))
            {
                return false;
            }

            return true;
        }

        bool FindSupportedCompiler(QWidget* parent)
        {
            auto findCompilerResult = FindSupportedCompilerForPlatform();

            if (!findCompilerResult.IsSuccess())
            {
                QMessageBox vsWarningMessage(parent);
                vsWarningMessage.setIcon(QMessageBox::Warning);
                vsWarningMessage.setWindowTitle(QObject::tr("Create Project"));
                // Makes link clickable
                vsWarningMessage.setTextFormat(Qt::RichText);
                vsWarningMessage.setText(findCompilerResult.GetError());
                vsWarningMessage.setStandardButtons(QMessageBox::Close);

                QSpacerItem* horizontalSpacer = new QSpacerItem(600, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
                QGridLayout* layout = reinterpret_cast<QGridLayout*>(vsWarningMessage.layout());
                layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
                vsWarningMessage.exec();
            }

            return findCompilerResult.IsSuccess();
        }

        ProjectManagerScreen GetProjectManagerScreen(const QString& screen)
        {
            auto iter = s_ProjectManagerStringNames.find(screen);
            if (iter != s_ProjectManagerStringNames.end())
            {
                return iter.value();
            }

            return ProjectManagerScreen::Invalid;
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
