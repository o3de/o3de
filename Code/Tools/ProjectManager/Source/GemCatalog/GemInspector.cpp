/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemItemDelegate.h>

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

        m_nameLabel->setText(m_model->GetDisplayName(modelIndex));
        m_creatorLabel->setText(m_model->GetCreator(modelIndex));

        m_summaryLabel->setText(m_model->GetSummary(modelIndex));
        m_summaryLabel->adjustSize();

        m_directoryLinkLabel->SetUrl(m_model->GetDirectoryLink(modelIndex));
        m_documentationLinkLabel->SetUrl(m_model->GetDocLink(modelIndex));

        if (m_model->HasRequirement(modelIndex))
        {
            m_reqirementsIconLabel->show();
            m_reqirementsTitleLabel->show();
            m_reqirementsTextLabel->show();

            m_reqirementsTitleLabel->setText("Requirement");
            m_reqirementsTextLabel->setText(m_model->GetRequirement(modelIndex));
        }
        else
        {
            m_reqirementsIconLabel->hide();
            m_reqirementsTitleLabel->hide();
            m_reqirementsTextLabel->hide();
        }

        // Depending gems
        m_dependingGems->Update("Depending Gems", "The following Gems will be automatically enabled with this Gem.", m_model->GetDependingGemNames(modelIndex));

        // Additional information
        m_versionLabel->setText(QString("Gem Version: %1").arg(m_model->GetVersion(modelIndex)));
        m_lastUpdatedLabel->setText(QString("Last Updated: %1").arg(m_model->GetLastUpdated(modelIndex)));
        m_binarySizeLabel->setText(QString("Binary Size:  %1 KB").arg(m_model->GetBinarySizeInKB(modelIndex)));

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

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setStyleSheet("color: #666666;");
        m_mainLayout->addWidget(hLine);

        m_mainLayout->addSpacing(10);

        // Requirements
        m_reqirementsTitleLabel = GemInspector::CreateStyledLabel(m_mainLayout, 16, s_headerColor);

        QHBoxLayout* requrementsLayout = new QHBoxLayout();
        requrementsLayout->setAlignment(Qt::AlignTop);
        requrementsLayout->setMargin(0);
        requrementsLayout->setSpacing(0);

        m_reqirementsIconLabel = new QLabel();
        m_reqirementsIconLabel->setPixmap(QIcon(":/Warning.svg").pixmap(24, 24));
        requrementsLayout->addWidget(m_reqirementsIconLabel);

        m_reqirementsTextLabel = GemInspector::CreateStyledLabel(requrementsLayout, 10, s_textColor);
        m_reqirementsTextLabel->setWordWrap(true);
        m_reqirementsTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_reqirementsTextLabel->setOpenExternalLinks(true);

        QSpacerItem* reqirementsSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding);
        requrementsLayout->addSpacerItem(reqirementsSpacer);

        m_mainLayout->addLayout(requrementsLayout);

        m_mainLayout->addSpacing(20);

        // Depending gems
        m_dependingGems = new GemsSubWidget();
        m_mainLayout->addWidget(m_dependingGems);
        m_mainLayout->addSpacing(20);

        // Additional information
        QLabel* additionalInfoLabel = CreateStyledLabel(m_mainLayout, 14, s_headerColor);
        additionalInfoLabel->setText("Additional Information");

        m_versionLabel = CreateStyledLabel(m_mainLayout, 12, s_textColor);
        m_lastUpdatedLabel = CreateStyledLabel(m_mainLayout, 12, s_textColor);
        m_binarySizeLabel = CreateStyledLabel(m_mainLayout, 12, s_textColor);
    }
} // namespace O3DE::ProjectManager
