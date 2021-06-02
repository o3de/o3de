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

#include <ProjectUtils.h>
#include <PythonBindingsInterface.h>

#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>

namespace O3DE::ProjectManager
{
    namespace ProjectUtils
    {
        static bool WarnDirectoryOverwrite(const QString& path, QWidget* parent)
        {
            if (!QDir(path).isEmpty())
            {
                QMessageBox::StandardButton warningResult = QMessageBox::warning(
                    parent,
                    QObject::tr("Overwrite Directory"),
                    QObject::tr("Directory is not empty! Are you sure you want to overwrite it?"),
                    QMessageBox::No | QMessageBox::Yes
                );

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
                    return false;
                }

                descendent.cdUp();
            }
            while (!descendent.isRoot());

            return true;
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

                // TODO: Block UX and Notify User they need to wait

                copyResult = CopyProject(origPath, newPath);
            }

            return copyResult;
        }

        bool CopyProject(const QString& origPath, const QString& newPath)
        {
            // Disallow copying from or into subdirectory
            if (!IsDirectoryDescedent(origPath, newPath) || !IsDirectoryDescedent(newPath, origPath))
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

        bool MoveProject(const QString& origPath, const QString& newPath, QWidget* parent)
        {
            if (!WarnDirectoryOverwrite(newPath, parent) || !UnregisterProject(origPath))
            {
                return false;
            }

            QDir directory;
            if (directory.rename(origPath, newPath))
            {
                return directory.rename(origPath, newPath);
            }

            if (!RegisterProject(newPath))
            {
                return false;
            }

            return true;
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
