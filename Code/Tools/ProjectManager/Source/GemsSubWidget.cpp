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
        m_layout = new QVBoxLayout();
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->setMargin(0);
        setLayout(m_layout);

        m_titleLabel = new QLabel();
        m_titleLabel->setObjectName("gemSubWidgetTitleLabel");
        m_layout->addWidget(m_titleLabel);

        m_textLabel = new QLabel();
        m_textLabel->setObjectName("gemSubWidgetTextLabel");
        m_textLabel->setWordWrap(true);
        m_layout->addWidget(m_textLabel);

        m_tagWidget = new TagContainerWidget();
        m_layout->addWidget(m_tagWidget);
    }

    void GemsSubWidget::Update(const QString& title, const QString& text, const QStringList& gemNames)
    {
        m_titleLabel->setText(title);
        m_textLabel->setText(text);
        m_tagWidget->Update(gemNames);
    }
} // namespace O3DE::ProjectManager
