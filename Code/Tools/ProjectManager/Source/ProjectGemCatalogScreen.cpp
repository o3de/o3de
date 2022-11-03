/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProjectGemCatalogScreen.h>
#include <PythonBindingsInterface.h>
#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <GemCatalog/GemDependenciesDialog.h>
#include <GemCatalog/GemFilterWidget.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemRequirementDialog.h>

#include <QDir>
#include <QMessageBox>
#include <QTimer>

namespace O3DE::ProjectManager
{
    ProjectGemCatalogScreen::ProjectGemCatalogScreen(DownloadController* downloadController, QWidget* parent)
        : GemCatalogScreen(downloadController , /*readOnly = */ false, parent)
    {

    }

    ProjectManagerScreen ProjectGemCatalogScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::ProjectGemCatalog;
    }

    ProjectGemCatalogScreen::ConfiguredGemsResult ProjectGemCatalogScreen::ConfigureGemsForProject(const QString& projectPath)
    {
        IPythonBindings* pythonBindings = PythonBindingsInterface::Get();
        QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded();
        QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved();

        if (m_gemModel->DoGemsToBeAddedHaveRequirements())
        {
            GemRequirementDialog* confirmRequirementsDialog = new GemRequirementDialog(m_gemModel, this);
            if(confirmRequirementsDialog->exec() == QDialog::Rejected)
            {
                return ConfiguredGemsResult::Cancel;
            }
        }

        if (m_gemModel->HasDependentGemsToRemove())
        {
            GemDependenciesDialog* dependenciesDialog = new GemDependenciesDialog(m_gemModel, this);
            if(dependenciesDialog->exec() == QDialog::Rejected)
            {
                return ConfiguredGemsResult::Cancel;
            }

            toBeAdded = m_gemModel->GatherGemsToBeAdded();
            toBeRemoved = m_gemModel->GatherGemsToBeRemoved();
        }

        for (const QModelIndex& modelIndex : toBeAdded)
        {
            const QString& gemPath = GemModel::GetPath(modelIndex);

            // make sure any remote gems we added were downloaded successfully 
            const GemInfo::DownloadStatus status = GemModel::GetDownloadStatus(modelIndex);
            if (GemModel::GetGemOrigin(modelIndex) == GemInfo::Remote &&
                !(status == GemInfo::Downloaded || status == GemInfo::DownloadSuccessful))
            {
                QMessageBox::critical(
                    nullptr, "Cannot add gem that isn't downloaded",
                    tr("Cannot add gem %1 to project because it isn't downloaded yet or failed to download.")
                        .arg(GemModel::GetDisplayName(modelIndex)));

                return ConfiguredGemsResult::Failed;
            }

            const AZ::Outcome<void, AZStd::string> result = pythonBindings->AddGemToProject(gemPath, projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(nullptr, "Failed to add gem to project",
                    tr("Cannot add gem %1 to project.<br><br>Error:<br>%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return ConfiguredGemsResult::Failed;
            }

            // register external gems that were added with relative paths
            if (m_gemsToRegisterWithProject.contains(gemPath))
            {
                pythonBindings->RegisterGem(QDir(projectPath).relativeFilePath(gemPath), projectPath);
            }
        }

        for (const QModelIndex& modelIndex : toBeRemoved)
        {
            const QString gemPath = GemModel::GetPath(modelIndex);
            const AZ::Outcome<void, AZStd::string> result = pythonBindings->RemoveGemFromProject(gemPath, projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(nullptr, "Failed to remove gem from project",
                    tr("Cannot remove gem %1 from project.<br><br>Error:<br>%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return ConfiguredGemsResult::Failed;
            }
        }

        return ConfiguredGemsResult::Success;
    }

    bool ProjectGemCatalogScreen::IsTab()
    {
        return false;
    }
} // namespace O3DE::ProjectManager
