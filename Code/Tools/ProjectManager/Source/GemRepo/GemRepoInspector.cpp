/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoInspector.h>
#include <GemRepo/GemRepoItemDelegate.h>
#include <PythonBindingsInterface.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelectionModel>

namespace O3DE::ProjectManager
{
    GemRepoInspector::GemRepoInspector(GemRepoModel* model, QItemSelectionModel* selectionModel,QWidget* parent)
        : QScrollArea(parent)
        , m_model(model)
        , m_selectionModel(selectionModel)
    {
        setObjectName("gemRepoInspector");
        setWidgetResizable(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_mainWidget = new QWidget();
        setWidget(m_mainWidget);

        m_mainLayout = new QVBoxLayout();
        m_mainLayout->setMargin(15);
        m_mainLayout->setAlignment(Qt::AlignTop);
        m_mainWidget->setLayout(m_mainLayout);

        InitMainWidget();

        connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &GemRepoInspector::OnSelectionChanged);
        Update({});
    }

    void GemRepoInspector::OnSelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        const QModelIndexList selectedIndices = selected.indexes();
        if (selectedIndices.empty())
        {
            Update({});
            return;
        }

        Update(selectedIndices[0]);
    }

    void GemRepoInspector::Update(const QModelIndex& modelIndex)
    {
        m_curModelIndex = modelIndex;

        if (!modelIndex.isValid())
        {
            m_mainWidget->hide();
        }

        // Repo name and url link
        m_nameLabel->setText(m_model->GetName(modelIndex));

        const QString repoUri = m_model->GetRepoUri(modelIndex);
        // ideally we would use Qt::TextWrapAnywhere to wrap and display the full URL
        // but QLabel only supports word-break wrapping so elide the text
        // clicking on the link will display the full URL and ask the user
        // to confirm they want to visit it
        m_repoLinkLabel->SetText(repoUri);
        m_repoLinkLabel->SetUrl(repoUri);

        // Repo summary
        m_summaryLabel->setText(m_model->GetSummary(modelIndex));
        m_summaryLabel->adjustSize();

        // Additional information
        if (m_model->HasAdditionalInfo(modelIndex))
        {
            m_addInfoTitleLabel->show();
            m_addInfoTextLabel->show();

            m_addInfoSpacer->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);

            m_addInfoTextLabel->setText(m_model->GetAdditionalInfo(modelIndex));
        }
        else
        {
            m_addInfoTitleLabel->hide();
            m_addInfoTextLabel->hide();

            m_addInfoSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        // Included Gems
        const QVector<Tag>& gemTags = m_model->GetIncludedGemTags(modelIndex);
        m_includedGems->setVisible(!gemTags.isEmpty());
        if (!gemTags.empty())
        {
            m_includedGems->Update(tr("Included Gems"), "", gemTags);
        }

        const QVector<Tag>& projectTags = m_model->GetIncludedProjectTags(modelIndex);
        m_includedProjects->setVisible(!projectTags.isEmpty());
        if (!projectTags.empty())
        {
            m_includedProjects->Update(tr("Included Projects"), "", projectTags);
        }

        const QVector<Tag>& templateTags = m_model->GetIncludedProjectTemplateTags(modelIndex);
        m_includedTemplates->setVisible(!templateTags.isEmpty());
        if (!templateTags.empty())
        {
            m_includedTemplates->Update(tr("Included Project Templates"), "", templateTags);
        }

        m_mainWidget->adjustSize();
        m_mainWidget->show();
    }

    void GemRepoInspector::InitMainWidget()
    {
        // Repo name and url link
        m_nameLabel = new AzQtComponents::ElidingLabel();
        m_nameLabel->setObjectName("gemRepoInspectorNameLabel");
        m_nameLabel->setWordWrap(true);
        m_mainLayout->addWidget(m_nameLabel);

        m_repoLinkLabel = new LinkLabel(tr("Repo URL"), QUrl(), 12, this);
        m_mainLayout->addWidget(m_repoLinkLabel);
        m_copyDownloadLinkLabel = new LinkLabel(tr("Copy Repo URL"));
        m_mainLayout->addWidget(m_copyDownloadLinkLabel);
        connect(m_copyDownloadLinkLabel, &LinkLabel::clicked, this, &GemRepoInspector::OnCopyDownloadLinkClicked);
        m_mainLayout->addSpacing(5);

        // Repo summary
        m_summaryLabel = new QLabel();
        m_summaryLabel->setObjectName("gemRepoInspectorBodyLabel");
        m_summaryLabel->setWordWrap(true);
        m_summaryLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_summaryLabel->setOpenExternalLinks(true);
        m_mainLayout->addWidget(m_summaryLabel);
        m_mainLayout->addSpacing(20);

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setObjectName("horizontalSeparatingLine");
        m_mainLayout->addWidget(hLine);
        m_mainLayout->addSpacing(10);

        // Additional information
        m_addInfoTitleLabel = new QLabel();
        m_addInfoTitleLabel->setObjectName("gemRepoInspectorAddInfoTitleLabel");
        m_addInfoTitleLabel->setText(tr("Additional Information"));
        m_mainLayout->addWidget(m_addInfoTitleLabel);

        m_addInfoTextLabel = new QLabel();
        m_addInfoTextLabel->setObjectName("gemRepoInspectorBodyLabel");
        m_addInfoTextLabel->setWordWrap(true);
        m_addInfoTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_addInfoTextLabel->setOpenExternalLinks(true);
        m_mainLayout->addWidget(m_addInfoTextLabel);

        // Conditional spacing for additional info section
        m_addInfoSpacer = new QSpacerItem(0, 20, QSizePolicy::Fixed);
        m_mainLayout->addSpacerItem(m_addInfoSpacer);

        // Included Gems
        m_includedGems = new GemsSubWidget();
        m_mainLayout->addWidget(m_includedGems);

        m_includedProjects = new GemsSubWidget();
        m_mainLayout->addWidget(m_includedProjects);

        m_includedTemplates = new GemsSubWidget();
        m_mainLayout->addWidget(m_includedTemplates);

        m_mainLayout->addSpacing(20);

        m_removeRepoButton = new QPushButton(tr("Remove"));
        m_removeRepoButton->setProperty("danger", true);
        m_mainLayout->addWidget(m_removeRepoButton);
        connect(m_removeRepoButton, &QPushButton::clicked, this , [this]{ emit RemoveRepo(m_curModelIndex); });
    }

    void GemRepoInspector::OnCopyDownloadLinkClicked()
    {
        const auto& url = m_repoLinkLabel->GetUrl();

        if (!url.toString().isEmpty())
        {
            if(QClipboard* clipboard = QGuiApplication::clipboard(); clipboard != nullptr)
            {
                clipboard->setText(url.toString());
                emit ShowToastNotification(tr("%1 URL copied to clipboard").arg(m_nameLabel->text()));
            }
            else
            {
                emit ShowToastNotification("Failed to copy URL to clipboard");
            }
        }
    }
} // namespace O3DE::ProjectManager
