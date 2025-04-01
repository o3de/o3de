/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectUtils.h>
#include <ProjectManagerDefs.h>
#include <ProjectManager_Traits_Platform.h>
#include <PythonBindingsInterface.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/chrono/chrono.h>

#include <QFileDialog>
#include <QDir>
#include <QtMath>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QGuiApplication>
#include <QProgressDialog>
#include <QSpacerItem>
#include <QStandardPaths>
#include <QGridLayout>
#include <QTextEdit>
#include <QByteArray>
#include <QScrollBar>
#include <QProgressBar>
#include <QLabel>
#include <QStandardPaths>


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

        static bool SkipFilePaths(const QString& curPath, QStringList& skippedPaths, QStringList& deeperSkippedPaths)
        {
            bool skip = false;
            for (const QString& skippedPath : skippedPaths)
            {
                QString nativeSkippedPath = QDir::toNativeSeparators(skippedPath);
                QString firstSectionSkippedPath = nativeSkippedPath.section(QDir::separator(), 0, 0);
                if (curPath == firstSectionSkippedPath)
                {
                    // We are at the end of the path to skip, so skip it
                    if (nativeSkippedPath == firstSectionSkippedPath)
                    {
                        skippedPaths.removeAll(skippedPath);
                        skip = true;
                        break;
                    }
                    // Append the next section of the skipped path
                    else
                    {
                        deeperSkippedPaths.append(nativeSkippedPath.section(QDir::separator(), 1));
                    }
                }
            }

            return skip;
        }

        typedef AZStd::function<void(/*fileCount=*/int, /*totalSizeInBytes=*/int)> StatusFunction;
        static void RecursiveGetAllFiles(const QDir& directory, QStringList& skippedPaths, int& outFileCount, qint64& outTotalSizeInBytes, StatusFunction statusCallback)
        {
            const QStringList entries = directory.entryList(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
            for (const QString& entryPath : entries)
            {
                const QString filePath = QDir::toNativeSeparators(QString("%1/%2").arg(directory.path()).arg(entryPath));

                QStringList deeperSkippedPaths;
                if (SkipFilePaths(entryPath, skippedPaths, deeperSkippedPaths))
                {
                    continue;
                }

                QFileInfo fileInfo(filePath);

                if (fileInfo.isDir())
                {
                    QDir subDirectory(filePath);
                    RecursiveGetAllFiles(subDirectory, deeperSkippedPaths, outFileCount, outTotalSizeInBytes, statusCallback);
                }
                else
                {
                    ++outFileCount;
                    outTotalSizeInBytes += fileInfo.size();

                    const int updateStatusEvery = 64;
                    if (outFileCount % updateStatusEvery == 0)
                    {
                        statusCallback(outFileCount, static_cast<int>(outTotalSizeInBytes));
                    }
                }
            }
        }

        static bool CopyDirectory(QProgressDialog* progressDialog,
            const QString& origPath,
            const QString& newPath,
            QStringList& skippedPaths,
            int filesToCopyCount,
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

            for (const QString& directory : original.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                if (progressDialog && progressDialog->wasCanceled())
                {
                    return false;
                }

                QStringList deeperSkippedPaths;
                if (SkipFilePaths(directory, skippedPaths, deeperSkippedPaths))
                {
                    continue;
                }

                QString newDirectoryPath = newPath + QDir::separator() + directory;
                original.mkpath(newDirectoryPath);

                if (!CopyDirectory(progressDialog, origPath + QDir::separator() + directory, newDirectoryPath, deeperSkippedPaths,
                    filesToCopyCount, outNumCopiedFiles, totalSizeToCopy, outCopiedFileSize, showIgnoreFileDialog))
                {
                    return false;
                }
            }

            QLocale locale;
            const float progressDialogRangeHalf = progressDialog ? static_cast<float>(qFabs(progressDialog->maximum() - progressDialog->minimum()) * 0.5f) : 0.f;
            for (const QString& file : original.entryList(QDir::Files))
            {
                if (progressDialog && progressDialog->wasCanceled())
                {
                    return false;
                }

                // Unused by this function but neccesary to pass in to SkipFilePaths
                QStringList deeperSkippedPaths;
                if (SkipFilePaths(file, skippedPaths, deeperSkippedPaths))
                {
                    continue;
                }

                // Progress window update
                if (progressDialog)
                {
                    // Weight in the number of already copied files as well as the copied bytes to get a better progress indication
                    // for cases combining many small files and some really large files.
                    const float normalizedNumFiles = static_cast<float>(outNumCopiedFiles) / filesToCopyCount;
                    const float normalizedFileSize = static_cast<float>(outCopiedFileSize) / totalSizeToCopy;
                    const int progress = static_cast<int>(normalizedNumFiles * progressDialogRangeHalf + normalizedFileSize * progressDialogRangeHalf);
                    progressDialog->setValue(progress);

                    const QString copiedFileSizeString = locale.formattedDataSize(outCopiedFileSize);
                    const QString totalFileSizeString = locale.formattedDataSize(totalSizeToCopy);
                    progressDialog->setLabelText(QString("Copying file %1 of %2 (%3 of %4) ...").arg(QString::number(outNumCopiedFiles),
                        QString::number(filesToCopyCount),
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

        static bool ClearProjectBuildArtifactsAndCache(const QString& origPath, const QString& newPath, QWidget* parent)
        {
            QDir buildDirectory = QDir(newPath);
            if ((!buildDirectory.cd(ProjectBuildDirectoryName) || !DeleteProjectFiles(buildDirectory.path(), true))
                && QDir(origPath).cd(ProjectBuildDirectoryName))
            {
                QMessageBox::warning(
                    parent,
                    QObject::tr("Clear Build Artifacts"),
                    QObject::tr("Build artifacts failed to delete for moved project. Please manually delete build directory at \"%1\"")
                        .arg(buildDirectory.path()),
                    QMessageBox::Close);

                return false;
            }

            QDir cacheDirectory = QDir(newPath);
            if ((!cacheDirectory.cd(ProjectCacheDirectoryName) || !DeleteProjectFiles(cacheDirectory.path(), true))
                && QDir(origPath).cd(ProjectCacheDirectoryName))
            {
                QMessageBox::warning(
                    parent,
                    QObject::tr("Clear Asset Cache"),
                    QObject::tr("Asset cache failed to delete for moved project. Please manually delete cache directory at \"%1\"")
                        .arg(cacheDirectory.path()),
                    QMessageBox::Close);

                return false;
            }

            return false;
        }

        bool RegisterProject(const QString& path, QWidget* parent)
        {
            auto incompatibleObjectsResult = PythonBindingsInterface::Get()->GetProjectEngineIncompatibleObjects(path);

            AZStd::string errorTitle, generalError, detailedError;
            if (!incompatibleObjectsResult)
            {
                errorTitle = "Failed to check project compatibility";
                generalError = incompatibleObjectsResult.GetError().first;
                generalError.append("\nDo you still want to add this project?");
                detailedError = incompatibleObjectsResult.GetError().second;
            }
            else if (const auto& incompatibleObjects = incompatibleObjectsResult.GetValue(); !incompatibleObjects.isEmpty())
            {
                // provide a couple more user friendly error messages for uncommon cases
                if (incompatibleObjects.at(0).contains(EngineJsonFilename.data(), Qt::CaseInsensitive))
                {
                    errorTitle = errorTitle.format("Failed to read %s", EngineJsonFilename.data());
                    generalError = "The projects compatibility with this engine could not be checked because the engine.json could not be read";
                }
                else if (incompatibleObjects.at(0).contains(ProjectJsonFilename.data(), Qt::CaseInsensitive))
                {
                    errorTitle = errorTitle.format("Invalid project, failed to read %s", ProjectJsonFilename.data());
                    generalError = "The projects compatibility with this engine could not be checked because the project.json could not be read.";
                }
                else
                {
                    // could be gems, apis or both
                    errorTitle = "Project may not be compatible with this engine";
                    generalError = incompatibleObjects.join("\n").toUtf8().constData();
                    generalError.append("\nDo you still want to add this project?");
                }
            }

            if (!generalError.empty())
            {
                QMessageBox warningDialog(QMessageBox::Warning, errorTitle.c_str(), generalError.c_str(), QMessageBox::Yes | QMessageBox::No, parent);
                warningDialog.setDetailedText(detailedError.c_str());
                if(warningDialog.exec() == QMessageBox::No)
                {
                    return false;
                }
                AZ_Warning("ProjectManager", false, "Proceeding with project registration after compatibility check failed.");
            }

            if (auto addProjectResult = PythonBindingsInterface::Get()->AddProject(path, /*force=*/true); !addProjectResult)
            {
                DisplayDetailedError(QObject::tr("Failed to add project"), addProjectResult, parent);
                AZ_Error("ProjectManager", false, "Failed to register project at path '%s'", path.toUtf8().constData());
                return false;
            }

            return true;
        }

        bool UnregisterProject(const QString& path, QWidget* parent)
        {
            if (auto result = PythonBindingsInterface::Get()->RemoveProject(path); !result)
            {
                DisplayDetailedError("Failed to unregister project", result, parent);
                return false;
            }
            return true;
        }

        bool CopyProjectDialog(const QString& origPath, ProjectInfo& newProjectInfo, QWidget* parent)
        {
            bool copyResult = false;

            QDir parentOrigDir(origPath);
            parentOrigDir.cdUp();
            QString newPath = QDir::toNativeSeparators(
                QFileDialog::getExistingDirectory(parent, QObject::tr("Select New Project Directory"), parentOrigDir.path()));
            if (!newPath.isEmpty())
            {
                newProjectInfo.m_path = newPath;

                if (!WarnDirectoryOverwrite(newPath, parent))
                {
                    return false;
                }

                copyResult = CopyProject(origPath, newPath, parent);
            }

            return copyResult;
        }

        bool CopyProject(const QString& origPath, const QString& newPath, QWidget* parent, bool skipRegister, bool showProgress)
        {
            // Disallow copying from or into subdirectory
            if (IsDirectoryDescedent(origPath, newPath) || IsDirectoryDescedent(newPath, origPath))
            {
                return false;
            }

            int filesToCopyCount = 0;
            qint64 totalSizeInBytes = 0;
            QStringList skippedPaths
            {
                ProjectBuildDirectoryName,
                ProjectCacheDirectoryName
            };

            QProgressDialog* progressDialog = nullptr;
            if (showProgress)
            {
                progressDialog = new QProgressDialog(parent);
                progressDialog->setAutoClose(true);
                progressDialog->setValue(0);
                progressDialog->setRange(0, 1000);
                progressDialog->setModal(true);
                progressDialog->setWindowTitle(QObject::tr("Copying project ..."));
                progressDialog->show();
            }

            QLocale locale;
            QStringList getFilesSkippedPaths(skippedPaths);
            RecursiveGetAllFiles(origPath, getFilesSkippedPaths, filesToCopyCount, totalSizeInBytes, [=](int fileCount, int sizeInBytes)
                {
                    if (progressDialog)
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
                    }
                });

            int numFilesCopied = 0;
            qint64 copiedFileSize = 0;

            // Phase 1: Copy files
            bool showIgnoreFileDialog = true;
            QStringList copyFilesSkippedPaths(skippedPaths);
            bool success = CopyDirectory(progressDialog, origPath, newPath, copyFilesSkippedPaths, filesToCopyCount, numFilesCopied,
                totalSizeInBytes, copiedFileSize, showIgnoreFileDialog);
            if (success && !skipRegister)
            {
                // Phase 2: Register project
                success = RegisterProject(newPath);
            }

            if (!success)
            {
                if (progressDialog)
                {
                    progressDialog->setLabelText(QObject::tr("Duplicating project failed/cancelled, removing already copied files ..."));
                    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
                }

                DeleteProjectFiles(newPath, true);
            }

            if (progressDialog)
            {
                progressDialog->deleteLater();
            }
            return success;
        }

        bool DeleteProjectFiles(const QString& path, bool force)
        {
            QDir projectDirectory(path);
            if (projectDirectory.exists())
            {
                // Check if there is an actual project here or just force it
                auto pythonBindingsPtr = PythonBindingsInterface::Get();
                if (pythonBindingsPtr)
                {
                    // if we can obtain the python interface, then we will ONLY delete the folder
                    // if its a real project, unless force is specified.

                    AZ::Outcome<ProjectInfo> pInfo = pythonBindingsPtr->GetProject(path);
                    if (force || pInfo.IsSuccess())
                    {
                        if (pInfo.IsSuccess())
                        {
                            //determine if we have a restricted directory to worry about
                            if (!pInfo.GetValue().m_restricted.isEmpty())
                            {
                                QDir restrictedDirectory(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
                        
                                if (restrictedDirectory.cd("O3DE/Restricted/Projects") &&
                                    restrictedDirectory.cd(pInfo.GetValue().m_restricted) &&
                                    !restrictedDirectory.isEmpty())
                                {
                                    restrictedDirectory.removeRecursively();
                                }
                            }
                        }
                        return projectDirectory.removeRecursively();
                    }
                }
                else
                {
                    // we don't have any python bindings available, we're likely in test mode.
                    return projectDirectory.removeRecursively();
                }
            }

            return false;
        }

        bool MoveProject(QString origPath, QString newPath, QWidget* parent, bool skipRegister, bool showProgress)
        {
            origPath = QDir::toNativeSeparators(origPath);
            newPath = QDir::toNativeSeparators(newPath);

            if (!WarnDirectoryOverwrite(newPath, parent) || (!skipRegister && !UnregisterProject(origPath)))
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
                if (!CopyProject(origPath, newPath, parent, skipRegister, showProgress))
                {
                    return false;
                }

                DeleteProjectFiles(origPath, true);
            }
            else
            {
                // If directoy rename succeeded then build and cache directories need to be deleted seperately
                ClearProjectBuildArtifactsAndCache(origPath, newPath, parent);
            }

            if (!skipRegister && !RegisterProject(newPath))
            {
                return false;
            }

            return true;
        }

        bool ReplaceProjectFile(const QString& origFile, const QString& newFile, QWidget* parent, bool interactive)
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

        bool FindSupportedCompiler(const ProjectInfo& projectInfo, QWidget* parent)
        {
            auto findCompilerResult = FindSupportedCompilerForPlatform(projectInfo);

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

        AZ::Outcome<QString, QString> ExecuteCommandResultModalDialog(
            const QString& cmd,
            const QStringList& arguments,
            const QString& title)
        {
            QString resultOutput;
            QProcess execProcess;
            execProcess.setProcessChannelMode(QProcess::MergedChannels);

            QProgressDialog dialog(title, QObject::tr("Cancel"), /*minimum=*/0, /*maximum=*/0);
            dialog.setMinimumWidth(500);
            dialog.setAutoClose(false);

            QProgressBar* bar = new QProgressBar(&dialog);
            bar->setTextVisible(false);
            bar->setMaximum(0); // infinite
            dialog.setBar(bar);

            QLabel* progressLabel = new QLabel(&dialog);
            QVBoxLayout* layout = new QVBoxLayout();

            // pre-fill the field with the title and command
            const QString commandOutput = QString("%1<br>%2 %3<br>").arg(title).arg(cmd).arg(arguments.join(' '));

            // replace the label with a scrollable text edit
            QTextEdit* detailTextEdit = new QTextEdit(commandOutput, &dialog);
            detailTextEdit->setReadOnly(true);
            layout->addWidget(detailTextEdit);
            layout->setMargin(0);
            progressLabel->setLayout(layout);
            progressLabel->setMinimumHeight(150);
            dialog.setLabel(progressLabel);

            auto readConnection = QObject::connect(&execProcess, &QProcess::readyReadStandardOutput,
                [&]()
                {
                    QScrollBar* scrollBar = detailTextEdit->verticalScrollBar();
                    bool autoScroll = scrollBar->value() == scrollBar->maximum();

                    QString output = execProcess.readAllStandardOutput();
                    detailTextEdit->append(output);
                    resultOutput.append(output);

                    if (autoScroll)
                    {
                        scrollBar->setValue(scrollBar->maximum());
                    }
                });

            auto exitConnection = QObject::connect(&execProcess,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [&](int exitCode, [[maybe_unused]] QProcess::ExitStatus exitStatus)
                {
                    QScrollBar* scrollBar = detailTextEdit->verticalScrollBar();
                    dialog.setMaximum(100);
                    dialog.setValue(dialog.maximum());
                    if (exitCode == 0 && scrollBar->value() == scrollBar->maximum())
                    {
                        dialog.close();
                    }
                    else
                    {
                        // keep the dialog open so the user can look at the output
                        dialog.setCancelButtonText(QObject::tr("Continue"));
                    }
                });

            execProcess.start(cmd, arguments);

            dialog.exec();

            QObject::disconnect(readConnection);
            QObject::disconnect(exitConnection);

            if (execProcess.state() == QProcess::Running)
            {
                execProcess.kill();
                return AZ::Failure(QObject::tr("Process for command '%1' was canceled").arg(cmd));
            }

            int resultCode = execProcess.exitCode();
            if (resultCode != 0)
            {
                return AZ::Failure(QObject::tr("Process for command '%1' failed (result code %2").arg(cmd).arg(resultCode));
            }

            return AZ::Success(resultOutput);
        }

        AZ::Outcome<QString, QString> ExecuteCommandResult(
            const QString& cmd,
            const QStringList& arguments,
            int commandTimeoutSeconds /*= ProjectCommandLineTimeoutSeconds*/)
        {
            QProcess execProcess;
            execProcess.setProcessChannelMode(QProcess::MergedChannels);
            execProcess.start(cmd, arguments);
            if (!execProcess.waitForStarted())
            {
                return AZ::Failure(QObject::tr("Unable to start process for command '%1'").arg(cmd));
            }

            if (!execProcess.waitForFinished(commandTimeoutSeconds * 1000 /* Milliseconds per second */))
            {
                return AZ::Failure(QObject::tr("Process for command '%1' timed out at %2 seconds").arg(cmd).arg(commandTimeoutSeconds));
            }
            int resultCode = execProcess.exitCode();
            QString resultOutput = execProcess.readAllStandardOutput();
            if (resultCode != 0)
            {
                return AZ::Failure(QObject::tr("Process for command '%1' failed (result code %2) %3").arg(cmd).arg(resultCode).arg(resultOutput));
            }
            return AZ::Success(resultOutput);
        }

        AZ::Outcome<QString, QString> GetProjectBuildPath(const QString& projectPath)
        {
            auto registry = AZ::SettingsRegistry::Get();

            // the project_build_path should be in the user settings registry inside the project folder
            AZ::IO::FixedMaxPath projectUserPath(projectPath.toUtf8().constData());
            projectUserPath /= AZ::SettingsRegistryInterface::DevUserRegistryFolder;
            if (!QDir(projectUserPath.c_str()).exists())
            {
                return AZ::Failure(QObject::tr("Failed to find the user registry folder %1").arg(projectUserPath.c_str()));
            }

            AZ::SettingsRegistryInterface::Specializations specializations;
            if(!registry->MergeSettingsFolder(projectUserPath.Native(), specializations, AZ_TRAIT_OS_PLATFORM_CODENAME))
            {
                return AZ::Failure(QObject::tr("Failed to merge registry settings in user registry folder %1").arg(projectUserPath.c_str()));
            }

            AZ::IO::FixedMaxPath projectBuildPath;
            if (!registry->Get(projectBuildPath.Native(), AZ::SettingsRegistryMergeUtils::ProjectBuildPath))
            {
                return AZ::Failure(QObject::tr("No project build path setting was found in the user registry folder %1").arg(projectUserPath.c_str()));
            }

            return AZ::Success(QString(projectBuildPath.c_str()));
        }

        QString GetPythonExecutablePath(const QString& enginePath)
        {
            // append lib path to Python paths
            AZ::IO::FixedMaxPath libPath = enginePath.toUtf8().constData();
            libPath /= AZ::IO::FixedMaxPathString(AZ_TRAIT_PROJECT_MANAGER_PYTHON_EXECUTABLE_SUBPATH);
            libPath = libPath.LexicallyNormal();
            return QString(libPath.String().c_str());
        }

        QString GetDefaultProjectPath()
        {
            QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
            if (engineInfoResult.IsSuccess())
            {
                QDir path(QDir::toNativeSeparators(engineInfoResult.GetValue().m_defaultProjectsFolder));
                if (path.exists())
                {
                    defaultPath = path.absolutePath();
                }
            }
            return defaultPath;
        }

        QString GetDefaultTemplatePath()
        {
            QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            AZ::Outcome<EngineInfo> engineInfoResult = PythonBindingsInterface::Get()->GetEngineInfo();
            if (engineInfoResult.IsSuccess())
            {
                QDir path(QDir::toNativeSeparators(engineInfoResult.GetValue().m_defaultTemplatesFolder));
                if (path.exists())
                {
                    defaultPath = path.absolutePath();
                }
            }
            return defaultPath;
        }

        int DisplayDetailedError(
            const QString& title, const AZ::Outcome<void, AZStd::pair<AZStd::string, AZStd::string>>& outcome, QWidget* parent)
        {
            return DisplayDetailedError(title, outcome.GetError().first, outcome.GetError().second, parent);
        }

        int DisplayDetailedError(
            const QString& title,
            const AZStd::string& generalError,
            const AZStd::string& detailedError,
            QWidget* parent,
            QMessageBox::StandardButtons buttons)
        {
            if (!detailedError.empty())
            {
                QMessageBox errorDialog(parent);
                errorDialog.setIcon(QMessageBox::Critical);
                errorDialog.setWindowTitle(title);
                errorDialog.setText(generalError.c_str());
                errorDialog.setDetailedText(detailedError.c_str());
                errorDialog.setStandardButtons(buttons);
                return errorDialog.exec();
            }
            else
            {
                return QMessageBox::critical(parent, title, generalError.c_str(), buttons);
            }
        }

        int VersionCompare(const QString& a, const QString&b)
        {
            auto outcomeA = AZ::SemanticVersion::ParseFromString(a.toUtf8().constData());
            auto outcomeB = AZ::SemanticVersion::ParseFromString(b.toUtf8().constData());

            auto versionA = outcomeA ? outcomeA.GetValue() : AZ::SemanticVersion(0, 0, 0);
            auto versionB = outcomeB ? outcomeB.GetValue() : AZ::SemanticVersion(0, 0, 0);

            return AZ::SemanticVersion::Compare(versionA, versionB);

        }

        QString GetDependencyString(const QString& dependencyString)
        {
            using Dependency = AZ::Dependency<AZ::SemanticVersion::parts_count>;
            using Comparison = Dependency::Bound::Comparison;
            Dependency dependency;

            QString result;
            if(auto parseOutcome = dependency.ParseVersions({ dependencyString.toUtf8().constData() }); parseOutcome)
            {
                // dependency name
                result.append(dependency.GetName().c_str());

                if (const auto& bounds = dependency.GetBounds(); !bounds.empty())
                {
                    // we only support a single specifier
                    const auto& bound = bounds[0];
                    Comparison comparison = bound.GetComparison();
                    if (comparison == Comparison::GreaterThan)
                    {
                        result.append(QObject::tr(" versions greater than"));
                    }
                    else if (comparison == Comparison::LessThan)
                    {
                        result.append(QObject::tr(" versions less than"));
                    }
                    else if ((comparison& Comparison::TwiddleWakka) != Comparison::None)
                    {
                        // don't try to explain the twiddle wakka in short form
                        result.append(QObject::tr(" versions ~="));
                    }

                    result.append(" ");
                    result.append(bound.GetVersion().ToString().c_str());

                    if ((comparison & Comparison::EqualTo) != Comparison::None)
                    {
                        if ((comparison & Comparison::GreaterThan) != Comparison::None)
                        {
                            result.append(QObject::tr(" or higher "));
                        }
                        else if ((comparison & Comparison::LessThan) != Comparison::None)
                        {
                            result.append(QObject::tr(" or lower "));
                        }
                    }
                }
            }
            return result;
        }


        void GetDependencyNameAndVersion(const QString& dependencyString, QString& objectName, Comparison& comparator, QString& version)
        {
            Dependency dependency;
            if (auto parseOutcome = dependency.ParseVersions({ dependencyString.toUtf8().constData() }); parseOutcome)
            {
                objectName = dependency.GetName().c_str();
                if (const auto& bounds = dependency.GetBounds(); !bounds.empty())
                {
                    comparator = dependency.GetBounds().at(0).GetComparison();
                    version = dependency.GetBounds().at(0).GetVersion().ToString().c_str();
                }
            }
            else
            {
                objectName = dependencyString;
            }
        }


        QString GetDependencyName(const QString& dependency)
        {
            QString objectName, version;
            ProjectUtils::Comparison comparator;
            GetDependencyNameAndVersion(dependency, objectName, comparator, version);
            return objectName; 
        }
    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
