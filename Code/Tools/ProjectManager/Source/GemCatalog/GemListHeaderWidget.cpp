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
        showCountLabel->setStyleSheet("font-size: 12px; font: italic;");
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

        const int gemNameStartX = GemItemDelegate::s_itemMargins.left() + GemItemDelegate::s_contentMargins.left() - 3;
        columnHeaderLayout->addSpacing(gemNameStartX);

        QLabel* gemNameLabel = new QLabel(tr("Gem Name"));
        gemNameLabel->setStyleSheet("font-size: 12px;");
        columnHeaderLayout->addWidget(gemNameLabel);

        columnHeaderLayout->addSpacing(77);

        QLabel* gemSummaryLabel = new QLabel(tr("Gem Summary"));
        gemSummaryLabel->setStyleSheet("font-size: 12px;");
        columnHeaderLayout->addWidget(gemSummaryLabel);

        QSpacerItem* horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        columnHeaderLayout->addSpacerItem(horizontalSpacer);

        QLabel* gemSelectedLabel = new QLabel(tr("Selected"));
        gemSelectedLabel->setStyleSheet("font-size: 12px;");
        columnHeaderLayout->addWidget(gemSelectedLabel);

        columnHeaderLayout->addSpacing(60);

        vLayout->addLayout(columnHeaderLayout);
    }
} // namespace O3DE::ProjectManager
