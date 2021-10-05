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
#include <QAbstractButton>

namespace O3DE::ProjectManager
{
    TagWidget::TagWidget(const QString& text, QWidget* parent)
        : QLabel(text, parent)
    {
        setObjectName("TagWidget");
    }

    TagWidget::TagWidget(const QString& text, QAbstractButton* button, QWidget* parent)
        : QLabel(text, parent)
        , m_button(button)
    {
        setObjectName("TagWidget");
        layout()->addWidget(button);
    }

    TagContainerWidget::TagContainerWidget(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName("TagWidgetContainer");
        setLayout(new FlowLayout(this));

        // layout margins cannot be set via qss 
        layout()->setMargin(0);

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
