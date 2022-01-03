/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemItemDelegate.h>
#include <GemCatalog/GemListHeaderWidget.h>
#include <AzQtComponents/Components/TagSelector.h>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QLabel>
#include <QSpacerItem>
#include <QStandardItemModel>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    GemListHeaderWidget::GemListHeaderWidget(GemSortFilterProxyModel* proxyModel, QWidget* parent)
        : QFrame(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        setLayout(vLayout);

        setStyleSheet("background-color: #333333;");

        vLayout->addSpacing(13);

        // Top section
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->addSpacing(16);
        topLayout->setMargin(0);

        auto* tagWidget = new FilterTagWidgetContainer();

        // Adjust the proxy model and disable the given feature used for filtering.
        connect(tagWidget, &FilterTagWidgetContainer::TagRemoved, this, [=](QString tagName)
            {
                QSet<QString> filteredFeatureTags = proxyModel->GetFeatures();
                filteredFeatureTags.remove(tagName);
                proxyModel->SetFeatures(filteredFeatureTags);
            });

        // Reinitialize the tag widget in case the filter in the proxy model got invalided.
        connect(proxyModel, &GemSortFilterProxyModel::OnInvalidated, this, [=]
            {
                const QSet<QString>& tagSet = proxyModel->GetFeatures();
                QVector<QString> sortedTags(tagSet.begin(), tagSet.end());
                std::sort(sortedTags.begin(), sortedTags.end());
                tagWidget->Reinit(sortedTags);
            });

        topLayout->addWidget(tagWidget);

        topLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        QLabel* showCountLabel = new QLabel();
        showCountLabel->setObjectName("GemCatalogHeaderShowCountLabel");
        topLayout->addWidget(showCountLabel);

        auto refreshGemCountUI = [=]() {
                const int numGemsShown = proxyModel->rowCount();
                showCountLabel->setText(QString(tr("showing %1 Gems")).arg(numGemsShown));
            };

        connect(proxyModel, &GemSortFilterProxyModel::OnInvalidated, this, refreshGemCountUI);
        connect(proxyModel->GetSourceModel(), &GemModel::dataChanged, this, refreshGemCountUI);

        topLayout->addSpacing(GemItemDelegate::s_contentMargins.right() + GemItemDelegate::s_borderWidth);

        vLayout->addLayout(topLayout);

        vLayout->addSpacing(13);

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setStyleSheet("color: #666666;");
        vLayout->addWidget(hLine);

        vLayout->addSpacing(GemItemDelegate::s_contentMargins.top());

        // Bottom section
        QHBoxLayout* columnHeaderLayout = new QHBoxLayout();
        columnHeaderLayout->setAlignment(Qt::AlignLeft);

        const int gemNameStartX = GemItemDelegate::s_itemMargins.left() + GemItemDelegate::s_contentMargins.left() - 1;
        columnHeaderLayout->addSpacing(gemNameStartX);

        QLabel* gemNameLabel = new QLabel(tr("Gem Name"));
        gemNameLabel->setObjectName("GemCatalogHeaderLabel");
        columnHeaderLayout->addWidget(gemNameLabel);

        columnHeaderLayout->addSpacing(89);

        QLabel* gemSummaryLabel = new QLabel(tr("Gem Summary"));
        gemSummaryLabel->setObjectName("GemCatalogHeaderLabel");
        columnHeaderLayout->addWidget(gemSummaryLabel);

        QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        columnHeaderLayout->addSpacerItem(horizontalSpacer);

        QLabel* gemSelectedLabel = new QLabel(tr("Status"));
        gemSelectedLabel->setObjectName("GemCatalogHeaderLabel");
        columnHeaderLayout->addWidget(gemSelectedLabel);

        columnHeaderLayout->addSpacing(72);

        vLayout->addLayout(columnHeaderLayout);
    }
} // namespace O3DE::ProjectManager
