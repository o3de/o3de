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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QComboBox>

namespace O3DE::ProjectManager
{
    GemInspector::GemInspector(GemModel* model, QWidget* parent, bool readOnly)
        : QScrollArea(parent)
        , m_model(model)
        , m_readOnly(readOnly)
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

    void SetLabelElidedText(QLabel* label, const QString& text, int labelWidth = 0)
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

    void GemInspector::Update(const QModelIndex& modelIndex, const QString& version)
    {
        m_curModelIndex = modelIndex;

        if (!modelIndex.isValid())
        {
            m_mainWidget->hide();
        }

        // use the provided version if available
        QString displayVersion = version;
        QString activeVersion = m_model->GetNewVersion(modelIndex);
        if (activeVersion.isEmpty())
        {
            // fallback to the current version
            activeVersion = m_model->GetVersion(modelIndex);
        }

        if (displayVersion.isEmpty())
        {
            displayVersion = activeVersion;
        }

        const GemInfo& gemInfo = m_model->GetGemInfo(modelIndex, displayVersion);

        // The gem display name should stay the same
        SetLabelElidedText(m_nameLabel, m_model->GetDisplayName(modelIndex));
        SetLabelElidedText(m_creatorLabel, gemInfo.m_origin);

        m_summaryLabel->setText(gemInfo.m_summary);
        m_summaryLabel->adjustSize();

        // Manually define remaining space to elide text because spacer would like to take all of the space
        SetLabelElidedText(m_licenseLinkLabel, gemInfo.m_licenseText, width() - m_licenseLabel->width() - 35);
        m_licenseLinkLabel->SetUrl(gemInfo.m_licenseLink);

        m_directoryLinkLabel->SetUrl(gemInfo.m_directoryLink);
        m_documentationLinkLabel->SetUrl(gemInfo.m_documentationLink);

        m_requirementsTextLabel->setVisible(!gemInfo.m_requirement.isEmpty());
        m_requirementsTextLabel->setText(gemInfo.m_requirement);

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
        m_lastUpdatedLabel->setText(tr("Last Updated: %1").arg(gemInfo.m_lastUpdatedDate));
        m_binarySizeLabel->setText(tr("Binary Size:  %1").arg(gemInfo.m_binarySizeInKB ? tr("%1 KB").arg(gemInfo.m_binarySizeInKB) : tr("Unknown")));

        // Versions
        disconnect(m_versionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GemInspector::OnVersionChanged);
        m_versionComboBox->clear();
        auto gemVersions = m_model->GetGemVersions(modelIndex);
        if (gemInfo.m_isEngineGem || gemVersions.count() < 2)
        {
            m_versionComboBox->setVisible(false);
            m_versionLabel->setText(gemInfo.m_version);
            m_versionLabel->setVisible(true);
            m_updateVersionButton->setVisible(false);
        }
        else
        {
            m_versionLabel->setVisible(false);
            m_versionComboBox->setVisible(true);
            m_versionComboBox->addItems(gemVersions);
            if (m_versionComboBox->count() == 0)
            {
                m_versionComboBox->insertItem(0, "Unknown");
            }

            auto foundIndex = m_versionComboBox->findText(displayVersion);
            m_versionComboBox->setCurrentIndex(foundIndex > -1 ? foundIndex : 0);

            bool versionChanged = displayVersion != activeVersion && !m_readOnly && m_model->IsAdded(modelIndex);
            m_updateVersionButton->setVisible(versionChanged);
            if (versionChanged)
            {
                m_updateVersionButton->setText(tr("Use Version %1").arg(m_versionComboBox->currentText()));
            }
            connect(m_versionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &GemInspector::OnVersionChanged);

        }

        // Compatible engines
        m_enginesTitleLabel->setVisible(!gemInfo.m_isEngineGem);
        m_enginesLabel->setVisible(!gemInfo.m_isEngineGem);
        if (!gemInfo.m_isEngineGem)
        {
            if (gemInfo.m_compatibleEngines.isEmpty())
            {
                m_enginesLabel->setText("All");
            }
            else
            {
                m_enginesLabel->setText(gemInfo.m_compatibleEngines.join("\n"));
            }
        }

        const bool isRemote = gemInfo.m_gemOrigin == GemInfo::Remote;
        const bool isDownloaded = gemInfo.m_downloadStatus == GemInfo::Downloaded ||
                                  gemInfo.m_downloadStatus == GemInfo::DownloadSuccessful;

        m_updateGemButton->setVisible(isRemote && isDownloaded);
        m_uninstallGemButton->setVisible(isRemote && isDownloaded);
        m_editGemButton->setVisible(!isRemote || (isRemote && isDownloaded));

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

    void GemInspector::OnVersionChanged([[maybe_unused]] int index)
    {
        Update(m_curModelIndex, m_versionComboBox->currentText());
        if (!GemModel::IsAdded(m_curModelIndex))
        {
            GemModel::UpdateWithVersion(*m_model, m_curModelIndex, m_versionComboBox->currentText());
        }
    }

    void GemInspector::InitMainWidget()
    {
        // Gem name, creator and summary
        m_nameLabel = CreateStyledLabel(m_mainLayout, 18, s_headerColor);
        m_creatorLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_headerColor);

        // Version
        {
            m_versionWidget = new QWidget();
            m_versionWidget->setObjectName("GemCatalogVersion");
            auto versionVLayout = new QVBoxLayout();
            versionVLayout->setMargin(0);
            auto versionHLayout = new QHBoxLayout();
            versionHLayout->setMargin(0);
            versionVLayout->addLayout(versionHLayout);
            m_versionWidget->setLayout(versionVLayout);
            m_mainLayout->addWidget(m_versionWidget);

            auto versionLabelTitle = new QLabel(tr("Version: "));
            versionLabelTitle->setProperty("class", "label");
            versionHLayout->addWidget(versionLabelTitle);
            m_versionLabel = new QLabel();
            m_versionLabel->setProperty("class", "value");
            versionHLayout->addWidget(m_versionLabel);

            m_versionComboBox = new QComboBox();
            versionHLayout->addWidget(m_versionComboBox);

            m_updateVersionButton = new QPushButton(tr("Use Version"));
            m_updateVersionButton->setProperty("class", "Secondary");
            versionVLayout->addWidget(m_updateVersionButton);
            connect(m_updateVersionButton, &QPushButton::clicked, this , [this]{
                GemModel::SetIsAdded(*m_model, m_curModelIndex, true, m_versionComboBox->currentText());
                GemModel::UpdateWithVersion(*m_model, m_curModelIndex, m_versionComboBox->currentText());
                m_updateVersionButton->setVisible(false);
            });

            auto enginesHLayout = new QHBoxLayout();
            enginesHLayout->setMargin(0);
            versionVLayout->addLayout(enginesHLayout);
            m_enginesTitleLabel = new QLabel(tr("Engines: "));
            m_enginesTitleLabel->setProperty("class", "label");
            enginesHLayout->addWidget(m_enginesTitleLabel);
            m_enginesLabel = new QLabel();
            m_enginesLabel->setProperty("class", "value");
            enginesHLayout->addWidget(m_enginesLabel);
        }

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
        m_requirementsTextLabel = new QLabel();
        m_requirementsTextLabel->setObjectName("GemCatalogRequirements");
        m_requirementsTextLabel->setWordWrap(true);
        m_requirementsTextLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        m_requirementsTextLabel->setOpenExternalLinks(true);

        m_mainLayout->addWidget(m_requirementsTextLabel);

        // Additional information
        auto additionalInfoLabel = new QLabel(tr("Additional Information"));
        additionalInfoLabel->setProperty("class", "title");
        m_mainLayout->addWidget(additionalInfoLabel);

        m_lastUpdatedLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_textColor);
        m_binarySizeLabel = CreateStyledLabel(m_mainLayout, s_baseFontSize, s_textColor);

        m_mainLayout->addSpacing(20);

        // Depending gems
        m_dependingGems = new GemsSubWidget();
        connect(m_dependingGems, &GemsSubWidget::TagClicked, this, [this](const Tag& tag){ emit TagClicked(tag); });
        m_mainLayout->addWidget(m_dependingGems);
        m_dependingGemsSpacer = new QSpacerItem(0, 20, QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_mainLayout->addSpacerItem(m_dependingGemsSpacer);


        // Update and Uninstall buttons
        m_updateGemButton = new QPushButton(tr("Update Gem"));
        m_updateGemButton->setProperty("class", "Secondary");
        m_mainLayout->addWidget(m_updateGemButton);
        connect(m_updateGemButton, &QPushButton::clicked, this , [this]{ emit UpdateGem(m_curModelIndex); });

        m_mainLayout->addSpacing(10);

        m_editGemButton = new QPushButton(tr("Edit Gem"));
        m_editGemButton->setProperty("class", "Secondary");
        m_mainLayout->addWidget(m_editGemButton);
        connect(m_editGemButton, &QPushButton::clicked, this , [this]{ emit EditGem(m_curModelIndex); });

        m_mainLayout->addSpacing(10);

        m_uninstallGemButton = new QPushButton(tr("Uninstall Gem"));
        m_uninstallGemButton->setProperty("class", "Danger");
        m_mainLayout->addWidget(m_uninstallGemButton);
        connect(m_uninstallGemButton, &QPushButton::clicked, this , [this]{ emit UninstallGem(m_curModelIndex); });
    }
} // namespace O3DE::ProjectManager
