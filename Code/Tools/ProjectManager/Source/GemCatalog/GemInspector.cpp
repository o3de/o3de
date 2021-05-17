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

#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemItemDelegate.h>
#include <QFrame>
#include <QLabel>
#include <QSpacerItem>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    GemInspector::GemInspector(GemModel* model, QWidget* parent)
        : QScrollArea(parent)
        , m_model(model)
    {
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

        // Depending and conflicting gems
        m_dependingGems->Update("Depending Gems", "The following Gems will be automatically enabled with this Gem.", m_model->GetDependingGems(modelIndex));
        m_conflictingGems->Update("Conflicting Gems", "The following Gems will be automatically disabled with this Gem.", m_model->GetConflictingGems(modelIndex));

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
        result->setStyleSheet(QString("font-size: %1pt; color: %2;").arg(QString::number(fontSize), colorCodeString));
        layout->addWidget(result);
        return result;
    }

    void GemInspector::InitMainWidget()
    {
        // Gem name, creator and summary
        m_nameLabel = CreateStyledLabel(m_mainLayout, 17, s_headerColor);
        m_creatorLabel = CreateStyledLabel(m_mainLayout, 12, s_creatorColor);
        m_mainLayout->addSpacing(5);

        // TODO: QLabel seems to have issues determining the right sizeHint() for our font with the given font size.
        // This results into squeezed elements in the layout in case the text is a little longer than a sentence.
        m_summaryLabel = new QLabel();//CreateLabel(m_mainLayout, 12, s_textColor);
        m_mainLayout->addWidget(m_summaryLabel);
        m_summaryLabel->setWordWrap(true);
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

        m_versionLabel = CreateStyledLabel(m_mainLayout, 11, s_textColor);
        m_lastUpdatedLabel = CreateStyledLabel(m_mainLayout, 11, s_textColor);
        m_binarySizeLabel = CreateStyledLabel(m_mainLayout, 11, s_textColor);
    }

    GemInspector::GemsSubWidget::GemsSubWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_layout = new QVBoxLayout();
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->setMargin(0);
        setLayout(m_layout);

        m_titleLabel = GemInspector::CreateStyledLabel(m_layout, 15, s_headerColor);
        m_textLabel = GemInspector::CreateStyledLabel(m_layout, 9, s_textColor);
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
