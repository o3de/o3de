/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemCatalogScreen.h>
#include <PythonBindingsInterface.h>
#include <GemCatalog/GemListHeaderWidget.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <GemCatalog/GemRequirementDialog.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <PythonBindingsInterface.h>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    GemCatalogScreen::GemCatalogScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_gemModel = new GemModel(this);
        m_proxModel = new GemSortFilterProxyModel(m_gemModel, this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        m_headerWidget = new GemCatalogHeaderWidget(m_gemModel, m_proxModel);
        vLayout->addWidget(m_headerWidget);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        vLayout->addLayout(hLayout);

        m_gemListView = new GemListView(m_proxModel, m_proxModel->GetSelectionModel(), this);
        m_gemInspector = new GemInspector(m_gemModel, this);
        m_gemInspector->setFixedWidth(240);

        QWidget* filterWidget = new QWidget(this);
        filterWidget->setFixedWidth(240);
        m_filterWidgetLayout = new QVBoxLayout();
        m_filterWidgetLayout->setMargin(0);
        m_filterWidgetLayout->setSpacing(0);
        filterWidget->setLayout(m_filterWidgetLayout);

        GemListHeaderWidget* listHeaderWidget = new GemListHeaderWidget(m_proxModel);

        QVBoxLayout* middleVLayout = new QVBoxLayout();
        middleVLayout->setMargin(0);
        middleVLayout->setSpacing(0);
        middleVLayout->addWidget(listHeaderWidget);
        middleVLayout->addWidget(m_gemListView);

        hLayout->addWidget(filterWidget);
        hLayout->addLayout(middleVLayout);
        hLayout->addWidget(m_gemInspector);
    }

    void GemCatalogScreen::ReinitForProject(const QString& projectPath)
    {
        m_gemModel->clear();
        FillModel(projectPath);

        if (m_filterWidget)
        {
            m_filterWidget->hide();
            m_filterWidget->deleteLater();
        }

        m_proxModel->ResetFilters();
        m_filterWidget = new GemFilterWidget(m_proxModel);
        m_filterWidgetLayout->addWidget(m_filterWidget);

        m_headerWidget->ReinitForProject();

        connect(m_gemModel, &GemModel::dataChanged, m_filterWidget, &GemFilterWidget::ResetGemStatusFilter);

        // Select the first entry after everything got correctly sized
        QTimer::singleShot(200, [=]{
            QModelIndex firstModelIndex = m_gemListView->model()->index(0,0);
            m_gemListView->selectionModel()->select(firstModelIndex, QItemSelectionModel::ClearAndSelect);
            });
    }

    void GemCatalogScreen::FillModel(const QString& projectPath)
    {
        AZ::Outcome<QVector<GemInfo>, AZStd::string> allGemInfosResult = PythonBindingsInterface::Get()->GetAllGemInfos(projectPath);
        if (allGemInfosResult.IsSuccess())
        {
            // Add all available gems to the model.
            const QVector<GemInfo> allGemInfos = allGemInfosResult.GetValue();
            for (const GemInfo& gemInfo : allGemInfos)
            {
                m_gemModel->AddGem(gemInfo);
            }

            m_gemModel->UpdateGemDependencies();

            // Gather enabled gems for the given project.
            auto enabledGemNamesResult = PythonBindingsInterface::Get()->GetEnabledGemNames(projectPath);
            if (enabledGemNamesResult.IsSuccess())
            {
                const QVector<AZStd::string> enabledGemNames = enabledGemNamesResult.GetValue();
                for (const AZStd::string& enabledGemName : enabledGemNames)
                {
                    const QModelIndex modelIndex = m_gemModel->FindIndexByNameString(enabledGemName.c_str());
                    if (modelIndex.isValid())
                    {
                        GemModel::SetWasPreviouslyAdded(*m_gemModel, modelIndex, true);
                        GemModel::SetIsAdded(*m_gemModel, modelIndex, true);
                    }
                    else
                    {
                        AZ_Warning("ProjectManager::GemCatalog", false,
                            "Cannot find entry for gem with name '%s'. The CMake target name probably does not match the specified name in the gem.json.",
                            enabledGemName.c_str());
                    }
                }
            }
            else
            {
                QMessageBox::critical(nullptr, tr("Operation failed"), QString("Cannot retrieve enabled gems for project %1.\n\nError:\n%2").arg(projectPath, enabledGemNamesResult.GetError().c_str()));
            }
        }
        else
        {
            QMessageBox::critical(nullptr, tr("Operation failed"), QString("Cannot retrieve gems for %1.\n\nError:\n%2").arg(projectPath, allGemInfosResult.GetError().c_str()));
        }
    }

    bool GemCatalogScreen::EnableDisableGemsForProject(const QString& projectPath)
    {
        IPythonBindings* pythonBindings = PythonBindingsInterface::Get();
        QVector<QModelIndex> toBeAdded = m_gemModel->GatherGemsToBeAdded();
        QVector<QModelIndex> toBeRemoved = m_gemModel->GatherGemsToBeRemoved();

        if (m_gemModel->DoGemsToBeAddedHaveRequirements())
        {
            GemRequirementDialog* confirmRequirementsDialog = new GemRequirementDialog(m_gemModel, toBeAdded, this);
            confirmRequirementsDialog->exec();

            if (confirmRequirementsDialog->GetButtonResult() != QDialogButtonBox::ApplyRole)
            {
                return false;
            }
        }

        for (const QModelIndex& modelIndex : toBeAdded)
        {
            const QString gemPath = GemModel::GetPath(modelIndex);
            const AZ::Outcome<void, AZStd::string> result = pythonBindings->AddGemToProject(gemPath, projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(nullptr, "Operation failed",
                    QString("Cannot add gem %1 to project.\n\nError:\n%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return false;
            }
        }

        for (const QModelIndex& modelIndex : toBeRemoved)
        {
            const QString gemPath = GemModel::GetPath(modelIndex);
            const AZ::Outcome<void, AZStd::string> result = pythonBindings->RemoveGemFromProject(gemPath, projectPath);
            if (!result.IsSuccess())
            {
                QMessageBox::critical(nullptr, "Operation failed",
                    QString("Cannot remove gem %1 from project.\n\nError:\n%2").arg(GemModel::GetDisplayName(modelIndex), result.GetError().c_str()));

                return false;
            }
        }

        return true;
    }

    ProjectManagerScreen GemCatalogScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemCatalog;
    }
} // namespace O3DE::ProjectManager
