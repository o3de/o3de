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
#include <QMessageBox>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QGuiApplication>

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

        static bool CopyDirectory(const QString& origPath, const QString& newPath)
        {
            QDir original(origPath);
            if (!original.exists())
            {
                return false;
            }

            for (QString directory : original.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                QString newDirectoryPath = newPath + QDir::separator() + directory;
                original.mkpath(newDirectoryPath);

                if (!CopyDirectory(origPath + QDir::separator() + directory, newDirectoryPath))
                {
                    return false;
                }
            }

            for (QString file : original.entryList(QDir::Files))
            {
                if (!QFile::copy(origPath + QDir::separator() + file, newPath + QDir::separator() + file))
                    return false;
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

                QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                copyResult = CopyProject(origPath, newPath);
                QGuiApplication::restoreOverrideCursor();

            }

            return copyResult;
        }

        bool CopyProject(const QString& origPath, const QString& newPath)
        {
            // Disallow copying from or into subdirectory
            if (IsDirectoryDescedent(origPath, newPath) || IsDirectoryDescedent(newPath, origPath))
            {
                return false;
            }

            if (!CopyDirectory(origPath, newPath))
            {
                // Cleanup whatever mess was made
                DeleteProjectFiles(newPath, true);
                return false;
            }

            if (!RegisterProject(newPath))
            {
                DeleteProjectFiles(newPath, true);
            }

            return true;
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
                if (!CopyProject(origPath, newPath))
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

        static bool IsVS2019Installed_internal()
        {
            QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
            QString programFilesPath = environment.value("ProgramFiles(x86)");
            QString vsWherePath = programFilesPath + "\\Microsoft Visual Studio\\Installer\\vswhere.exe";

            QFileInfo vsWhereFile(vsWherePath);
            if (vsWhereFile.exists() && vsWhereFile.isFile())
            {
                QProcess vsWhereProcess;
                vsWhereProcess.setProcessChannelMode(QProcess::MergedChannels);

                vsWhereProcess.start(
                    vsWherePath,
                    QStringList{ "-version", "16.0", "-latest", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                 "-property", "isComplete" });

                if (!vsWhereProcess.waitForStarted())
                {
                    return false;
                }

                while (vsWhereProcess.waitForReadyRead())
                {
                }

                QString vsWhereOutput(vsWhereProcess.readAllStandardOutput());
                if (vsWhereOutput.startsWith("1"))
                {
                    return true;
                }
            }

            return false;
        }

        bool IsVS2019Installed()
        {
            static bool vs2019Installed = IsVS2019Installed_internal();

            return vs2019Installed;
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
