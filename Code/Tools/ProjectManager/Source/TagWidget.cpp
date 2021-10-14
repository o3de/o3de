/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TagWidget.h>
#include <QVBoxLayout>
#include <AzQtComponents/Components/FlowLayout.h>

namespace O3DE::ProjectManager
{
    TagWidget::TagWidget(const QString& text, QWidget* parent)
        : QLabel(text, parent)
    {
        setObjectName("TagWidget");
    }

    TagContainerWidget::TagContainerWidget(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName("TagWidgetContainer");
        setLayout(new FlowLayout(this));

        // layout margins cannot be set via qss 
        constexpr int verticalMargin = 10;
        constexpr int horizontalMargin = 0;
        layout()->setContentsMargins(horizontalMargin, verticalMargin, horizontalMargin, verticalMargin);

        setAttribute(Qt::WA_StyledBackground, true);
    }

    void TagContainerWidget::Update(const QStringList& tags)
    {
        FlowLayout* flowLayout = static_cast<FlowLayout*>(layout());

        // remove old tags
        QLayoutItem* layoutItem = nullptr; 
        while ((layoutItem = layout()->takeAt(0)) != nullptr)
        {
            layoutItem->widget()->deleteLater();
        }

        foreach (const QString& tag, tags)
        {
            flowLayout->addWidget(new TagWidget(tag));
        }
    }
} // namespace O3DE::ProjectManager
