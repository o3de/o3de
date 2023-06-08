/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemsSubWidget.h>
#include <TagWidget.h>

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>

namespace O3DE::ProjectManager
{
    GemsSubWidget::GemsSubWidget(QWidget* parent)
        : QWidget(parent)
    {
        auto layout = new QVBoxLayout();
        layout->setAlignment(Qt::AlignTop);
        layout->setMargin(0);
        setLayout(layout);

        m_titleLabel = new QLabel();
        m_titleLabel->setObjectName("gemSubWidgetTitleLabel");
        layout->addWidget(m_titleLabel);

        m_textLabel = new QLabel();
        m_textLabel->setObjectName("gemSubWidgetTextLabel");
        m_textLabel->setWordWrap(true);
        layout->addWidget(m_textLabel);

        m_tagWidget = new TagContainerWidget();
        connect(m_tagWidget, &TagContainerWidget::TagClicked, this, [=](const Tag& tag){ emit TagClicked(tag); });
        layout->addWidget(m_tagWidget);
    }

    void GemsSubWidget::Update(const QString& title, const QString& text, const QVector<Tag>& tags)
    {
        m_titleLabel->setText(title);

        // hide the text label if it's empty
        m_textLabel->setText(text);
        m_textLabel->setVisible(!text.isEmpty());

        m_tagWidget->Update(tags);
        m_tagWidget->adjustSize();
        adjustSize();
    }
} // namespace O3DE::ProjectManager
