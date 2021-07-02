/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemItemDelegate.h>
#include <GemCatalog/GemListHeaderWidget.h>
#include <QStandardItemModel>
#include <QLabel>
#include <QVBoxLayout>
#include <QSpacerItem>

namespace O3DE::ProjectManager
{
    GemListHeaderWidget::GemListHeaderWidget(GemSortFilterProxyModel* proxyModel, QWidget* parent)
        : QFrame(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        setLayout(vLayout);

        setStyleSheet("background-color: #333333;");

        vLayout->addSpacing(20);

        // Top section
        QHBoxLayout* topLayout = new QHBoxLayout();
        topLayout->setMargin(0);
        topLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        QLabel* showCountLabel = new QLabel();
        showCountLabel->setObjectName("GemCatalogHeaderShowCountLabel");
        topLayout->addWidget(showCountLabel);
        connect(proxyModel, &GemSortFilterProxyModel::OnInvalidated, this, [=]
            {
                const int numGemsShown = proxyModel->rowCount();
                showCountLabel->setText(QString(tr("showing %1 Gems")).arg(numGemsShown));
            });

        topLayout->addSpacing(GemItemDelegate::s_contentMargins.right() + GemItemDelegate::s_borderWidth);

        vLayout->addLayout(topLayout);

        vLayout->addSpacing(20);

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

        QLabel* gemSelectedLabel = new QLabel(tr("Selected"));
        gemSelectedLabel->setObjectName("GemCatalogHeaderLabel");
        columnHeaderLayout->addWidget(gemSelectedLabel);

        columnHeaderLayout->addSpacing(65);

        vLayout->addLayout(columnHeaderLayout);
    }
} // namespace O3DE::ProjectManager
