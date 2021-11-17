/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoScreen.h>
#include <GemRepo/GemRepoItemDelegate.h>
#include <GemRepo/GemRepoListView.h>
#include <GemRepo/GemRepoModel.h>
#include <GemRepo/GemRepoAddDialog.h>
#include <GemRepo/GemRepoInspector.h>
#include <PythonBindingsInterface.h>
#include <ProjectManagerDefs.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QMessageBox>
#include <QLabel>
#include <QHeaderView>
#include <QTableWidget>
#include <QFrame>
#include <QStackedWidget>
#include <QMessageBox>

namespace O3DE::ProjectManager
{
    GemRepoScreen::GemRepoScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        m_gemRepoModel = new GemRepoModel(this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        setLayout(vLayout);

        m_contentStack = new QStackedWidget(this);

        m_noRepoContent = CreateNoReposContent();
        m_contentStack->addWidget(m_noRepoContent);

        m_repoContent = CreateReposContent();
        m_contentStack->addWidget(m_repoContent);

        vLayout->addWidget(m_contentStack);

        Reinit();
    }

    void GemRepoScreen::NotifyCurrentScreen()
    {
        Reinit();
    }

    void GemRepoScreen::Reinit()
    {
        m_gemRepoModel->clear();
        FillModel();

        // If model contains any data show the repos
        if (m_gemRepoModel->rowCount())
        {
            m_contentStack->setCurrentWidget(m_repoContent);
        }
        else
        {
            m_contentStack->setCurrentWidget(m_noRepoContent);
        }

        // Select the first entry after everything got correctly sized
        QTimer::singleShot(200, [=]{
            QModelIndex firstModelIndex = m_gemRepoListView->model()->index(0,0);
            m_gemRepoListView->selectionModel()->setCurrentIndex(firstModelIndex, QItemSelectionModel::ClearAndSelect);
        });
    }

    void GemRepoScreen::HandleAddRepoButton()
    {
        GemRepoAddDialog* repoAddDialog = new GemRepoAddDialog(this);

        if (repoAddDialog->exec() == QDialog::DialogCode::Accepted)
        {
            QString repoUri = repoAddDialog->GetRepoPath();
            if (repoUri.isEmpty())
            {
                QMessageBox::warning(this, tr("No Input"), tr("Please provide a repo Uri."));
                return;
            }

            AZ::Outcome < void,
                AZStd::pair<AZStd::string, AZStd::string>> addGemRepoResult = PythonBindingsInterface::Get()->AddGemRepo(repoUri);
            if (addGemRepoResult.IsSuccess())
            {
                Reinit();
                emit OnRefresh();
            }
            else
            {
                QString failureMessage = tr("Failed to add gem repo: %1.").arg(repoUri);
                if (!addGemRepoResult.GetError().second.empty())
                {
                    QMessageBox addRepoError;
                    addRepoError.setIcon(QMessageBox::Critical);
                    addRepoError.setWindowTitle(failureMessage);
                    addRepoError.setText(addGemRepoResult.GetError().first.c_str());
                    addRepoError.setDetailedText(addGemRepoResult.GetError().second.c_str());
                    addRepoError.exec();
                }
                else
                {
                    QMessageBox::critical(this, failureMessage, addGemRepoResult.GetError().first.c_str());
                }

                AZ_Error("Project Manager", false, failureMessage.toUtf8());
            }
        }
    }

    void GemRepoScreen::HandleRemoveRepoButton(const QModelIndex& modelIndex)
    {
        QString repoName = m_gemRepoModel->GetName(modelIndex);

        QMessageBox::StandardButton warningResult = QMessageBox::warning(
            this, tr("Remove Repo"), tr("Are you sure you would like to remove gem repo: %1?").arg(repoName),
            QMessageBox::No | QMessageBox::Yes);

        if (warningResult == QMessageBox::Yes)
        {
            QString repoUri = m_gemRepoModel->GetRepoUri(modelIndex);
            bool removeGemRepoResult = PythonBindingsInterface::Get()->RemoveGemRepo(repoUri);
            if (removeGemRepoResult)
            {
                Reinit();
                emit OnRefresh();
            }
            else
            {
                QString failureMessage = tr("Failed to remove gem repo: %1.").arg(repoUri);
                QMessageBox::critical(this, tr("Operation failed"), failureMessage);
                AZ_Error("Project Manger", false, failureMessage.toUtf8());
            }
        }
    }

    void GemRepoScreen::HandleRefreshAllButton()
    {
        bool refreshResult = PythonBindingsInterface::Get()->RefreshAllGemRepos();
        Reinit();
        emit OnRefresh();

        if (!refreshResult)
        {
            QMessageBox::critical(
                this, tr("Operation failed"), QString("Some repos failed to refresh."));
        }
    }

    void GemRepoScreen::HandleRefreshRepoButton(const QModelIndex& modelIndex)
    {
        const QString repoUri = m_gemRepoModel->GetRepoUri(modelIndex);

        AZ::Outcome<void, AZStd::string> refreshResult = PythonBindingsInterface::Get()->RefreshGemRepo(repoUri);
        if (refreshResult.IsSuccess())
        {
            Reinit();
            emit OnRefresh();
        }
        else
        {
            QMessageBox::critical(
                this, tr("Operation failed"),
                QString("Failed to refresh gem repo %1<br>Error:<br>%2")
                    .arg(m_gemRepoModel->GetName(modelIndex), refreshResult.GetError().c_str()));
        }
    }

    void GemRepoScreen::FillModel()
    {
        AZ::Outcome<QVector<GemRepoInfo>, AZStd::string> allGemRepoInfosResult = PythonBindingsInterface::Get()->GetAllGemRepoInfos();
        if (allGemRepoInfosResult.IsSuccess())
        {
            // Add all available repos to the model
            const QVector<GemRepoInfo> allGemRepoInfos = allGemRepoInfosResult.GetValue();
            QDateTime oldestRepoUpdate;
            if (!allGemRepoInfos.isEmpty())
            {
                oldestRepoUpdate = allGemRepoInfos[0].m_lastUpdated;
            }
            for (const GemRepoInfo& gemRepoInfo : allGemRepoInfos)
            {
                m_gemRepoModel->AddGemRepo(gemRepoInfo);

                // Find least recently updated repo
                if (gemRepoInfo.m_lastUpdated < oldestRepoUpdate)
                {
                    oldestRepoUpdate = gemRepoInfo.m_lastUpdated;
                }
            }

            if (!allGemRepoInfos.isEmpty())
            {
                m_lastAllUpdateLabel->setText(tr("Last Updated: %1").arg(oldestRepoUpdate.toString(RepoTimeFormat)));
            }
            else
            {
                m_lastAllUpdateLabel->setText(tr("Last Updated: Never"));
            }
        }
        else
        {
            QMessageBox::critical(this, tr("Operation failed"), QString("Cannot retrieve gem repos for engine.<br>Error:<br>%2").arg(allGemRepoInfosResult.GetError().c_str()));
        }
    }

    QFrame* GemRepoScreen::CreateNoReposContent()
    {
        QFrame* contentFrame = new QFrame(this);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setAlignment(Qt::AlignHCenter);
        vLayout->setMargin(0);
        vLayout->setSpacing(0);
        contentFrame->setLayout(vLayout);

        vLayout->addStretch();

        QLabel* noRepoLabel = new QLabel(tr("No repositories have been added yet."), this);
        noRepoLabel->setObjectName("gemRepoNoReposLabel");
        vLayout->addWidget(noRepoLabel);
        vLayout->setAlignment(noRepoLabel, Qt::AlignHCenter);

        vLayout->addSpacing(20);

        // Size hint for button is wrong so horizontal layout with stretch is used to center it
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        hLayout->setSpacing(0);

        hLayout->addStretch();

        QPushButton* addRepoButton = new QPushButton(tr("Add Repository"), this);
        addRepoButton->setObjectName("gemRepoAddButton");
        addRepoButton->setMinimumWidth(120);
        hLayout->addWidget(addRepoButton);

        connect(addRepoButton, &QPushButton::clicked, this, &GemRepoScreen::HandleAddRepoButton);

        hLayout->addStretch();

        vLayout->addLayout(hLayout);

        vLayout->addStretch();

        return contentFrame;
    }

    QFrame* GemRepoScreen::CreateReposContent()
    {
        QFrame* contentFrame = new QFrame(this);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        hLayout->setSpacing(0);
        contentFrame->setLayout(hLayout);

        hLayout->addSpacing(60);

        QVBoxLayout* middleVLayout = new QVBoxLayout();
        middleVLayout->setMargin(0);
        middleVLayout->setSpacing(0);

        middleVLayout->addSpacing(30);

        QHBoxLayout* topMiddleHLayout = new QHBoxLayout();
        topMiddleHLayout->setMargin(0);
        topMiddleHLayout->setSpacing(0);

        m_lastAllUpdateLabel = new QLabel(tr("Last Updated: Never"), this);
        m_lastAllUpdateLabel->setObjectName("gemRepoHeaderLabel");
        topMiddleHLayout->addWidget(m_lastAllUpdateLabel);

        topMiddleHLayout->addSpacing(20);

        m_AllUpdateButton = new QPushButton(QIcon(":/Refresh.svg"), tr("Update All"), this);
        m_AllUpdateButton->setObjectName("gemRepoHeaderRefreshButton");
        topMiddleHLayout->addWidget(m_AllUpdateButton);

        connect(m_AllUpdateButton, &QPushButton::clicked, this, &GemRepoScreen::HandleRefreshAllButton);

        topMiddleHLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

        QPushButton* addRepoButton = new QPushButton(tr("Add Repository"), this);
        addRepoButton->setObjectName("gemRepoAddButton");
        topMiddleHLayout->addWidget(addRepoButton);

        connect(addRepoButton, &QPushButton::clicked, this, &GemRepoScreen::HandleAddRepoButton);

        topMiddleHLayout->addSpacing(30);

        middleVLayout->addLayout(topMiddleHLayout);

        middleVLayout->addSpacing(30);

        // Create a QTableWidget just for its header
        // Using a seperate model allows the setup  of a header exactly as needed
        m_gemRepoHeaderTable = new QTableWidget(this);
        m_gemRepoHeaderTable->setObjectName("gemRepoHeaderTable");
        m_gemRepoListHeader = m_gemRepoHeaderTable->horizontalHeader();
        m_gemRepoListHeader->setObjectName("gemRepoListHeader");
        m_gemRepoListHeader->setDefaultAlignment(Qt::AlignLeft);
        m_gemRepoListHeader->setSectionResizeMode(QHeaderView::ResizeMode::Fixed);

        // Insert columns so the header labels will show up
        m_gemRepoHeaderTable->insertColumn(0);
        m_gemRepoHeaderTable->insertColumn(1);
        m_gemRepoHeaderTable->insertColumn(2);
        m_gemRepoHeaderTable->setHorizontalHeaderLabels({ tr("Repository Name"), tr("Creator"), tr("Updated") });

        const int headerExtraMargin = 18;
        m_gemRepoListHeader->resizeSection(0, GemRepoItemDelegate::s_nameMaxWidth + GemRepoItemDelegate::s_contentSpacing + headerExtraMargin);
        m_gemRepoListHeader->resizeSection(1, GemRepoItemDelegate::s_creatorMaxWidth + GemRepoItemDelegate::s_contentSpacing);
        m_gemRepoListHeader->resizeSection(2, GemRepoItemDelegate::s_updatedMaxWidth + GemRepoItemDelegate::s_contentSpacing);

        // Required to set stylesheet in code as it will not be respected if set in qss
        m_gemRepoHeaderTable->horizontalHeader()->setStyleSheet("QHeaderView::section { background-color:transparent; color:white; font-size:12px; border-style:none; }");
        middleVLayout->addWidget(m_gemRepoHeaderTable);

        m_gemRepoListView = new GemRepoListView(m_gemRepoModel, m_gemRepoModel->GetSelectionModel(), this);
        middleVLayout->addWidget(m_gemRepoListView);

        connect(m_gemRepoListView, &GemRepoListView::RemoveRepo, this, &GemRepoScreen::HandleRemoveRepoButton);
        connect(m_gemRepoListView, &GemRepoListView::RefreshRepo, this, &GemRepoScreen::HandleRefreshRepoButton);

        hLayout->addLayout(middleVLayout);

        m_gemRepoInspector = new GemRepoInspector(m_gemRepoModel, this);
        m_gemRepoInspector->setFixedWidth(240);
        hLayout->addWidget(m_gemRepoInspector);

        return contentFrame;
    }

    ProjectManagerScreen GemRepoScreen::GetScreenEnum()
    {
        return ProjectManagerScreen::GemRepos;
    }
} // namespace O3DE::ProjectManager
