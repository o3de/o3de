/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemItemDelegate.h>
#include <ProjectManagerDefs.h>

#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QIcon>

namespace O3DE::ProjectManager
{
    GemInspector::GemInspector(GemModel* model, QWidget* parent)
        : QScrollArea(parent)
        , m_model(model)
    {
        setObjectName("GemCatalogInspector");
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

        connect(m_model->GetSelectionModel(), &QItemSelectionModel::selectionChanged, this, &GemInspector::OnSelectionChanged);
        Update({});
    }

    void GemInspector::OnSelectionChanged(const QItemSelection& selected, [[maybe_unused]] const QItemSelection& deselected)
    {
        const QModelIndexList selectedIndices = selected.indexes();
        if (selectedIndices.empty())
        {
            Update({});
            return;
        }

        Update(selectedIndices[0]);
    }

    void GemInspector::Update(const QModelIndex& modelIndex)
    {
        if (!modelIndex.isValid())
        {
            m_mainWidget->hide();
        }

        m_nameLabel->setText(m_model->GetName(modelIndex));
        m_creatorLabel->setText(m_model->GetCreator(modelIndex));

        m_summaryLabel->setText(m_model->GetSummary(modelIndex));
        m_summaryLabel->adjustSize();

        m_directoryLinkLabel->SetUrl(m_model->GetDirectoryLink(modelIndex));
        m_documentationLinkLabel->SetUrl(m_model->GetDocLink(modelIndex));

        if (QDir(GemModel::GetPath(modelIndex)).exists(ProjectPreviewImagePath))
        {
            m_previewImage->show();

            QString previewPath = QDir(GemModel::GetPath(modelIndex)).filePath(ProjectPreviewImagePath);
            m_previewImage->setPixmap(QPixmap(previewPath).scaled(
                QSize(GemPreviewImageMaxWidth, GemPreviewImageMaxWidth), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        else
        {
            m_previewImage->hide();
        }

        if (m_model->HasRequirement(modelIndex))
        {
            m_requirementsIcon->show();
            m_requirementsTitleLabel->show();
            m_requirementsTextLabel->show();

            m_requirementsTitleLabel->setText("Requirement");
            m_requirementsTextLabel->setText(m_model->GetRequirement(modelIndex));

            m_requirementsSpacer->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        else
        {
            m_requirementsIcon->hide();
            m_requirementsTitleLabel->hide();
            m_requirementsTextLabel->hide();

            m_requirementsSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        // Depending and conflicting gems
        m_dependingGems->Update("Depending Gems", "The following Gems will be automatically enabled with this Gem.", m_model->GetDependingGemNames(modelIndex));
        m_conflictingGems->Update("Conflicting Gems", "The following Gems will be automatically disabled with this Gem.", m_model->GetConflictingGemNames(modelIndex));

        // Additional information
        m_versionLabel->setText(QString("Gem Version: %1").arg(m_model->GetVersion(modelIndex)));
        m_lastUpdatedLabel->setText(QString("Last Updated: %1").arg(m_model->GetLastUpdated(modelIndex)));
        m_binarySizeLabel->setText(QString("Binary Size:  %1 KB").arg(QString::number(m_model->GetBinarySizeInKB(modelIndex))));

        m_mainWidget->adjustSize();
        m_mainWidget->show();
    }

    QLabel* GemInspector::CreateStyledLabel(QLayout* layout, int fontSize, const QString& colorCodeString)
    {
        QLabel* result = new QLabel();
        result->setStyleSheet(QString("font-size: %1px; color: %2;").arg(QString::number(fontSize), colorCodeString));
        layout->addWidget(result);
        return result;
    }

    void GemInspector::InitMainWidget()
    {
        // Gem name, creator and summary
        m_nameLabel = CreateStyledLabel(m_mainLayout, 18, s_headerColor);
        m_creatorLabel = CreateStyledLabel(m_mainLayout, 12, s_headerColor);
        m_mainLayout->addSpacing(5);

        // TODO: QLabel seems to have issues determining the right sizeHint() for our font with the given font size.
        // This results into squeezed elements in the layout in case the text is a little longer than a sentence.
        m_summaryLabel = CreateStyledLabel(m_mainLayout, 12, s_textColor);
        m_mainLayout->addWidget(m_summaryLabel);
        m_summaryLabel->setWordWrap(true);
        m_summaryLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_summaryLabel->setOpenExternalLinks(true);
        m_mainLayout->addSpacing(5);

        // Directory and documentation links
        {
            QHBoxLayout* linksHLayout = new QHBoxLayout();
            linksHLayout->setMargin(0);
            m_mainLayout->addLayout(linksHLayout);

            QSpacerItem* spacerLeft = new QSpacerItem(0, 0, QSizePolicy::Expanding);
            linksHLayout->addSpacerItem(spacerLeft);

            m_directoryLinkLabel = new LinkLabel("View in Directory");
            linksHLayout->addWidget(m_directoryLinkLabel);
            linksHLayout->addWidget(new QLabel("|"));
            m_documentationLinkLabel  = new LinkLabel("Read Documentation");
            linksHLayout->addWidget(m_documentationLinkLabel);

            QSpacerItem* spacerRight = new QSpacerItem(0, 0, QSizePolicy::Expanding);
            linksHLayout->addSpacerItem(spacerRight);

            m_mainLayout->addSpacing(8);
        }

        // Preview Image
        {
            QHBoxLayout* previewHLayout = new QHBoxLayout();
            previewHLayout->setMargin(0);

            QSpacerItem* spacerLeft = new QSpacerItem(0, 0, QSizePolicy::Expanding);
            previewHLayout->addSpacerItem(spacerLeft);

            m_previewImage = new QLabel(this);
            previewHLayout->addWidget(m_previewImage);

            QSpacerItem* spacerRight = new QSpacerItem(0, 0, QSizePolicy::Expanding);
            previewHLayout->addSpacerItem(spacerRight);

            m_mainLayout->addLayout(previewHLayout);

            m_mainLayout->addSpacing(8);
        }

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setStyleSheet("color: #666666;");
        m_mainLayout->addWidget(hLine);

        m_mainLayout->addSpacing(10);

        // Requirements
        m_requirementsTitleLabel = GemInspector::CreateStyledLabel(m_mainLayout, 16, s_headerColor);

        QHBoxLayout* requirementsLayout = new QHBoxLayout();
        requirementsLayout->setMargin(0);
        requirementsLayout->setSpacing(0);

        m_requirementsIcon = new QLabel();
        m_requirementsIcon->setPixmap(QIcon(":/Warning.svg").pixmap(24, 24));
        m_requirementsIcon->setMaximumSize(24, 24);
        requirementsLayout->addWidget(m_requirementsIcon);

        requirementsLayout->addSpacing(10);

        m_requirementsTextLabel = GemInspector::CreateStyledLabel(requirementsLayout, 10, s_textColor);
        m_requirementsTextLabel->setWordWrap(true);
        m_requirementsTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_requirementsTextLabel->setOpenExternalLinks(true);

        m_mainLayout->addLayout(requirementsLayout);

        m_requirementsSpacer = new QSpacerItem(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_mainLayout->addSpacerItem(m_requirementsSpacer);

        // Depending and conflicting gems
        m_dependingGems = new GemsSubWidget();
        m_mainLayout->addWidget(m_dependingGems);
        m_mainLayout->addSpacing(20);

        m_conflictingGems = new GemsSubWidget();
        m_mainLayout->addWidget(m_conflictingGems);
        m_mainLayout->addSpacing(20);

        // Additional information
        QLabel* additionalInfoLabel = CreateStyledLabel(m_mainLayout, 14, s_headerColor);
        additionalInfoLabel->setText("Additional Information");

        m_versionLabel = CreateStyledLabel(m_mainLayout, 12, s_textColor);
        m_lastUpdatedLabel = CreateStyledLabel(m_mainLayout, 12, s_textColor);
        m_binarySizeLabel = CreateStyledLabel(m_mainLayout, 12, s_textColor);
    }

    GemInspector::GemsSubWidget::GemsSubWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_layout = new QVBoxLayout();
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->setMargin(0);
        setLayout(m_layout);

        m_titleLabel = GemInspector::CreateStyledLabel(m_layout, 16, s_headerColor);
        m_textLabel = GemInspector::CreateStyledLabel(m_layout, 10, s_textColor);
        m_textLabel->setWordWrap(true);

        m_tagWidget = new TagContainerWidget();
        m_layout->addWidget(m_tagWidget);
    }

    void GemInspector::GemsSubWidget::Update(const QString& title, const QString& text, const QStringList& gemNames)
    {
        m_titleLabel->setText(title);
        m_textLabel->setText(text);
        m_tagWidget->Update(gemNames);
    }
} // namespace O3DE::ProjectManager
