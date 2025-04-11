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
#include <ProjectUtils.h>

#include <QDir>
#include <QMessageBox>
#include <QTimer>

namespace O3DE::ProjectManager
{
    ProjectGemCatalogScreen::ProjectGemCatalogScreen(DownloadController* downloadController, QWidget* parent)
        : GemCatalogScreen(downloadController , /*readOnly = */ false, parent)
    {
        // We have to fetch the parent of our parent, because Project Manager Gem Catalog is usually embedded inside another workflow, like
        // CreateProjectScreen or UpdateProjectScreen
        // As such, it does not have direct access to ScreenControls
        SetUpScreensControl(parent->parentWidget());
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
            if (confirmRequirementsDialog->exec() == QDialog::Rejected)
            {
                return ConfiguredGemsResult::Cancel;
            }
        }

        if (m_gemModel->HasDependentGemsToRemove())
        {
            GemDependenciesDialog* dependenciesDialog = new GemDependenciesDialog(m_gemModel, this);
            if (dependenciesDialog->exec() == QDialog::Rejected)
            {
                return ConfiguredGemsResult::Cancel;
            }

            toBeAdded = m_gemModel->GatherGemsToBeAdded();
            toBeRemoved = m_gemModel->GatherGemsToBeRemoved();
        }

        if (!toBeAdded.isEmpty())
        {
            QStringList gemPaths, gemNames;
            for (const QModelIndex& modelIndex : toBeAdded)
            {
                // make sure any remote gems we added were downloaded successfully
                GemInfo gemInfo = GemModel::GetGemInfo(modelIndex);
                const GemInfo::DownloadStatus status = GemModel::GetDownloadStatus(modelIndex);
                if (gemInfo.m_gemOrigin == GemInfo::Remote &&
                    !(status == GemInfo::Downloaded || status == GemInfo::DownloadSuccessful))
                {
                    QMessageBox::critical(
                        nullptr,
                        "Cannot add gem that isn't downloaded",
                        tr("Cannot add gem %1 to project because it isn't downloaded yet or failed to download.")
                            .arg(GemModel::GetDisplayName(modelIndex)));

                    return ConfiguredGemsResult::Failed;
                }

                gemPaths.append(gemInfo.m_path);

                // use the version that was selected if available
                if (auto gemVersion = GemModel::GetNewVersion(modelIndex); !gemVersion.isEmpty())
                {
                    gemInfo.m_version = gemVersion;
                }

                gemNames.append(gemInfo.GetNameWithVersionSpecifier());
            }

            // check compatibility of all gems
            auto incompatibleResult = pythonBindings->GetIncompatibleProjectGems(gemPaths, gemNames, projectPath);
            if (incompatibleResult && !incompatibleResult.GetValue().empty())
            {
                const auto& incompatibleGems = incompatibleResult.GetValue();
                QString messageBoxQuestion =
                    gemNames.length() == 1 ? tr("Do you still want to add this gem?") : tr("Do you still want to add these gems?");
                QString messageBoxText = QString(tr("%1\n\n%2")).arg(incompatibleGems.join("\n")).arg(messageBoxQuestion);
                QMessageBox::StandardButton forceAddGems = QMessageBox::warning(this, tr("Gem compatibility issues found"), messageBoxText,
                    QMessageBox::Yes | QMessageBox::No);
                if (forceAddGems != QMessageBox::StandardButton::Yes)
                {
                    return ConfiguredGemsResult::Cancel;
                }
            }

            // we already checked compatibility, so bypass compatibility checks by using 'force'
            constexpr bool force = true;
            auto addGemsResult = pythonBindings->AddGemsToProject(gemPaths, gemNames, projectPath, force);
            if (!addGemsResult.IsSuccess())
            {
                QString failureMessage = gemNames.length() == 1 ? tr("Failed to activate gem") : tr("Failed to activate gems");
                ProjectUtils::DisplayDetailedError(failureMessage, addGemsResult, this);
                AZ_Error("Project Manager", false, failureMessage.toUtf8().constData());

                return ConfiguredGemsResult::Failed;
            }
            else
            {
                for (const QModelIndex& modelIndex : toBeAdded)
                {
                    GemModel::SetWasPreviouslyAdded(*m_gemModel, modelIndex, true);

                    // if the user selected a new version then make sure to show that version
                    const QString& newVersion = GemModel::GetNewVersion(modelIndex);
                    if (!newVersion.isEmpty())
                    {
                        GemModel::UpdateWithVersion(*m_gemModel, modelIndex, newVersion);
                        GemModel::SetNewVersion(*m_gemModel, modelIndex, "");
                    }
                    const auto& gemPath = GemModel::GetGemInfo(modelIndex).m_path;

                    // register external gems that were added with relative paths
                    if (m_gemsToRegisterWithProject.contains(gemPath))
                    {
                        pythonBindings->RegisterGem(QDir(projectPath).relativeFilePath(gemPath), projectPath);
                    }
                }
            }
        }

        for (const QModelIndex& modelIndex : toBeRemoved)
        {
            const auto result = pythonBindings->RemoveGemFromProject(GemModel::GetName(modelIndex), projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(
                    nullptr, "Failed to remove gem from project",
                    tr("Cannot remove gem %1 from project.<br><br>Error:<br>%2")
                        .arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return ConfiguredGemsResult::Failed;
            }
            else
            {
                GemModel::SetWasPreviouslyAdded(*m_gemModel, modelIndex, false);
            }
        }

        return ConfiguredGemsResult::Success;
    }

    bool ProjectGemCatalogScreen::IsTab()
    {
        return false;
    }
} // namespace O3DE::ProjectManager
