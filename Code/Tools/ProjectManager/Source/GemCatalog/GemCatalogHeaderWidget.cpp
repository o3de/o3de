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

#include <AzQtComponents/Components/SearchLineEdit.h>
#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <QHBoxLayout>
#include <QLabel>

namespace O3DE::ProjectManager
{
    GemCatalogHeaderWidget::GemCatalogHeaderWidget(GemSortFilterProxyModel* filterProxyModel, QWidget* parent)
        : QFrame(parent)
    {
        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setAlignment(Qt::AlignLeft);
        hLayout->setMargin(0);
        setLayout(hLayout);

        setStyleSheet("background-color: #1E252F;");

        QLabel* titleLabel = new QLabel(tr("Gem Catalog"));
        titleLabel->setStyleSheet("font-size: 21px;");
        hLayout->addWidget(titleLabel);

        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

        AzQtComponents::SearchLineEdit* filterLineEdit = new AzQtComponents::SearchLineEdit();
        filterLineEdit->setStyleSheet("background-color: #DDDDDD;");
        connect(filterLineEdit, &QLineEdit::textChanged, this, [=](const QString& text)
            {
                filterProxyModel->SetSearchString(text);
            });
        hLayout->addWidget(filterLineEdit);

        hLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
        hLayout->addSpacerItem(new QSpacerItem(220, 0, QSizePolicy::Fixed));
        
        setFixedHeight(60);
    }
} // namespace O3DE::ProjectManager
