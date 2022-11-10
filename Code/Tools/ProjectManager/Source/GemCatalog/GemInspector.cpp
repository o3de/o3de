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
#include <QPushButton>

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
        if (parent)
        {
            m_mainWidget->setFixedWidth(parent->width());
        }
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

    void SetLabelElidedText(QLabel* label, QString text, int labelWidth = 0)
    {
        QFontMetrics nameFontMetrics(label->font());
        if (!labelWidth)
        {
            labelWidth = label->width();
        }

        // Don't elide if the widgets are sized too small (sometimes occurs when loading gem catalog)
        if (labelWidth > 100)
        {
            label->setText(nameFontMetrics.elidedText(text, Qt::ElideRight, labelWidth));
        }
        else
        {
            label->setText(text);
        }
    }

    void GemInspector::Update(const QModelIndex& modelIndex)
    {
        m_curModelIndex = modelIndex;

        if (!modelIndex.isValid())
        {
            m_mainWidget->hide();
        }

        SetLabelElidedText(m_nameLabel, m_model->GetDisplayName(modelIndex));
        SetLabelElidedText(m_creatorLabel, m_model->GetCreator(modelIndex));

        m_summaryLabel->setText(m_model->GetSummary(modelIndex));
        m_summaryLabel->adjustSize();

        // Manually define remaining space to elide text because spacer would like to take all of the space
        SetLabelElidedText(m_licenseLinkLabel, m_model->GetLicenseText(modelIndex), width() - m_licenseLabel->width() - 35);
        m_licenseLinkLabel->SetUrl(m_model->GetLicenseLink(modelIndex));

        m_directoryLinkLabel->SetUrl(m_model->GetDirectoryLink(modelIndex));
        m_documentationLinkLabel->SetUrl(m_model->GetDocLink(modelIndex));

        if (m_model->HasRequirement(modelIndex))
        {
            m_requirementsIconLabel->show();
            m_requirementsTitleLabel->show();
            m_requirementsTextLabel->show();
            m_requirementsMainSpacer->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);

            m_requirementsTitleLabel->setText(tr("Requirement"));
            m_requirementsTextLabel->setText(m_model->GetRequirement(modelIndex));
        }
        else
        {
            m_requirementsIconLabel->hide();
            m_requirementsTitleLabel->hide();
            m_requirementsTextLabel->hide();
            m_requirementsMainSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        // Depending gems
        const QVector<Tag>& dependingGemTags = m_model->GetDependingGemTags(modelIndex);
        if (!dependingGemTags.isEmpty())
        {
            m_dependingGems->Update(tr("Depending Gems"), tr("The following Gems will be automatically enabled with this Gem."), dependingGemTags);
            m_dependingGems->show();
            m_dependingGemsSpacer->changeSize(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        else
        {
            m_dependingGems->hide();
            m_dependingGemsSpacer->changeSize(0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
        }

        // Additional information
        m_versionLabel->setText(tr("Gem Version: %1").arg(m_model->GetVersion(modelIndex)));
        m_lastUpdatedLabel->setText(tr("Last Updated: %1").arg(m_model->GetLastUpdated(modelIndex)));
        const int binarySize = m_model->GetBinarySizeInKB(modelIndex);
        m_binarySizeLabel->setText(tr("Binary Size:  %1").arg(binarySize ? tr("%1 KB").arg(binarySize) : tr("Unknown")));

        // Update and Uninstall buttons
        if (m_model->GetGemOrigin(modelIndex) == GemInfo::Remote &&
            (m_model->GetDownloadStatus(modelIndex) == GemInfo::Downloaded ||
             m_model->GetDownloadStatus(modelIndex) == GemInfo::DownloadSuccessful))
        {
            m_updateGemButton->show();
            m_uninstallGemButton->show();
        }
        else
        {
            m_updateGemButton->hide();
            m_uninstallGemButton->hide();
        }

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
        m_creatorLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_headerColor);
        m_mainLayout->addSpacing(5);

        // TODO: QLabel seems to have issues determining the right sizeHint() for our font with the given font size.
        // This results into squeezed elements in the layout in case the text is a little longer than a sentence.
        m_summaryLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_headerColor);
        m_mainLayout->addWidget(m_summaryLabel);
        m_summaryLabel->setWordWrap(true);
        m_summaryLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_summaryLabel->setOpenExternalLinks(true);
        m_mainLayout->addSpacing(5);

        // License
        {
            QHBoxLayout* licenseHLayout = new QHBoxLayout();
            licenseHLayout->setMargin(0);
            licenseHLayout->setAlignment(Qt::AlignLeft);
            m_mainLayout->addLayout(licenseHLayout);

            m_licenseLabel = CreateStyledLabel(licenseHLayout, s_baseFontSize, s_headerColor);
            m_licenseLabel->setText(tr("License: "));

            m_licenseLinkLabel = new LinkLabel("", QUrl(), s_baseFontSize);
            licenseHLayout->addWidget(m_licenseLinkLabel);

            licenseHLayout->addStretch();

            m_mainLayout->addSpacing(5);
        }

        // Directory and documentation links
        {
            QHBoxLayout* linksHLayout = new QHBoxLayout();
            linksHLayout->setMargin(0);
            m_mainLayout->addLayout(linksHLayout);

            linksHLayout->addStretch();

            m_directoryLinkLabel = new LinkLabel(tr("View in Directory"));
            linksHLayout->addWidget(m_directoryLinkLabel);
            linksHLayout->addWidget(new QLabel("|"));
            m_documentationLinkLabel  = new LinkLabel(tr("Read Documentation"));
            linksHLayout->addWidget(m_documentationLinkLabel);

            linksHLayout->addStretch();

            m_mainLayout->addSpacing(8);
        }

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setObjectName("horizontalSeparatingLine");
        m_mainLayout->addWidget(hLine);

        m_mainLayout->addSpacing(10);

        // Requirements
        m_requirementsTitleLabel = GemInspector::CreateStyledLabel(m_mainLayout, 16, s_headerColor);

        QHBoxLayout* requirementsLayout = new QHBoxLayout();
        requirementsLayout->setAlignment(Qt::AlignTop);
        requirementsLayout->setMargin(0);
        requirementsLayout->setSpacing(0);

        m_requirementsIconLabel = new QLabel();
        m_requirementsIconLabel->setPixmap(QIcon(":/Warning.svg").pixmap(24, 24));
        requirementsLayout->addWidget(m_requirementsIconLabel);

        m_requirementsTextLabel = GemInspector::CreateStyledLabel(requirementsLayout, 10, s_textColor);
        m_requirementsTextLabel->setWordWrap(true);
        m_requirementsTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_requirementsTextLabel->setOpenExternalLinks(true);

        QSpacerItem* requirementsSpacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding);
        requirementsLayout->addSpacerItem(requirementsSpacer);

        m_mainLayout->addLayout(requirementsLayout);

        m_requirementsMainSpacer = new QSpacerItem(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_mainLayout->addSpacerItem(m_requirementsMainSpacer);

        // Depending gems
        m_dependingGems = new GemsSubWidget();
        connect(m_dependingGems, &GemsSubWidget::TagClicked, this, [this](const Tag& tag){ emit TagClicked(tag); });
        m_mainLayout->addWidget(m_dependingGems);
        m_dependingGemsSpacer = new QSpacerItem(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_mainLayout->addSpacerItem(m_dependingGemsSpacer);

        // Additional information
        QLabel* additionalInfoLabel = CreateStyledLabel(m_mainLayout, 14, s_headerColor);
        additionalInfoLabel->setText(tr("Additional Information"));

        m_versionLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_textColor);
        m_lastUpdatedLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_textColor);
        m_binarySizeLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_textColor);

        m_mainLayout->addSpacing(20);

        // Update and Uninstall buttons
        m_updateGemButton = new QPushButton(tr("Update Gem"));
        m_updateGemButton->setObjectName("gemCatalogUpdateGemButton");
        m_mainLayout->addWidget(m_updateGemButton);
        connect(m_updateGemButton, &QPushButton::clicked, this , [this]{ emit UpdateGem(m_curModelIndex); });

        m_mainLayout->addSpacing(10);

        m_editGemButton = new QPushButton(tr("Edit Gem"));
        m_editGemButton->setObjectName("gemCatalogUpdateGemButton");
        m_mainLayout->addWidget(m_editGemButton);
        connect(m_editGemButton, &QPushButton::clicked, this , [this]{ emit EditGem(m_curModelIndex); });

        m_mainLayout->addSpacing(10);

        m_uninstallGemButton = new QPushButton(tr("Uninstall Gem"));
        m_uninstallGemButton->setObjectName("gemCatalogUninstallGemButton");
        m_mainLayout->addWidget(m_uninstallGemButton);
        connect(m_uninstallGemButton, &QPushButton::clicked, this , [this]{ emit UninstallGem(m_curModelIndex); });
    }
} // namespace O3DE::ProjectManager
