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

#include <filesystem>

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

        static bool IsDirectoryDecedent(const QString& possibleAncestorPath, const QString& possibleDecedentPath)
        {
            QDir ancestor(possibleAncestorPath);
            QDir decendent(possibleDecedentPath);

            do
            {
                if (ancestor == decendent)
                {
                    return false;
                }

                decendent.cdUp();
            }
            while (!decendent.isRoot());

            return true;
        }

        bool AddProjectDialog(QWidget* parent)
        {
            QString path = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(parent, QObject::tr("Select Project Directory")));
            if (!path.isEmpty())
            {
                return AddProject(path);
            }

            return false;
        }

        bool AddProject(const QString& path)
        {
            return PythonBindingsInterface::Get()->AddProject(path);
        }

        bool RemoveProject(const QString& path)
        {
            return PythonBindingsInterface::Get()->RemoveProject(path);
        }

        bool CopyProjectDialog(const QString& origPath, QWidget* parent)
        {
            bool copyResult = false;
            QString newPath =
                QDir::toNativeSeparators(QFileDialog::getExistingDirectory(parent, QObject::tr("Select New Project Directory"), origPath));
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
            if (IsDirectoryDecedent(origPath, newPath) || IsDirectoryDecedent(newPath, origPath))
            {
                return false;
            }

            std::filesystem::path origFilesytemPath(origPath.toStdString());
            std::filesystem::path newFilesytemPath(newPath.toStdString());

            // Use Filesystem because its much better at recursive directory copies than Qt
            try
            {
                std::filesystem::copy(
                    origFilesytemPath,
                    newFilesytemPath,
                    std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive
                );
            }
            catch ([[maybe_unused]] std::exception& e)
            {
                // Cleanup whatever mess was made
                DeleteProject(newPath);
                return false;
            }

            if (!AddProject(newPath))
            {
                DeleteProject(newPath);
            }

            return true;
        }

        bool DeleteProject(const QString& path)
        {
            QDir projectDirectory(path);
            if (projectDirectory.exists())
            {
                return projectDirectory.removeRecursively();
            }

            return false;
        }

        bool MoveProject(const QString& origPath, const QString& newPath, QWidget* parent)
        {
            if (!WarnDirectoryOverwrite(newPath, parent) || !RemoveProject(origPath))
            {
                return false;
            }

            QDir directory;
            if (directory.rename(origPath, newPath))
            {
                return directory.rename(origPath, newPath);
            }

            if (!AddProject(newPath))
            {
                return false;
            }

            return true;
        }

    } // namespace ProjectUtils
} // namespace O3DE::ProjectManager
