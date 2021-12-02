/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemFilterTagWidget.h>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>

namespace O3DE::ProjectManager
{
    FilterTagWidget::FilterTagWidget(const QString& text, QWidget* parent)
        : QFrame(parent)
    {
        setFrameShape(QFrame::NoFrame);

        auto* layout = new QHBoxLayout();
        layout->setContentsMargins(6, 5, 4, 4);
        layout->setSpacing(2);
        setLayout(layout);

        setStyleSheet("background-color: #555555;");

        m_textLabel = new QLabel();
        m_textLabel->setObjectName("FilterTagWidgetTextLabel");
        m_textLabel->setText(text);
        layout->addWidget(m_textLabel);

        m_closeButton = new QPushButton();
        m_closeButton->setFlat(true);
        m_closeButton->setIcon(QIcon(":/X.svg"));
        m_closeButton->setIconSize(QSize(12, 12));
        m_closeButton->setStyleSheet("QPushButton { background-color: transparent; border: 0px }");
        layout->addWidget(m_closeButton);
        connect(m_closeButton, &QPushButton::clicked, this, [=]{ emit RemoveClicked(); });
    }

    QString FilterTagWidget::text() const
    {
        return m_textLabel->text();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    FilterTagWidgetContainer::FilterTagWidgetContainer(QWidget* parent)
        : QWidget(parent)
    {
        m_layout = new QHBoxLayout();
        m_layout->setMargin(0);
        m_layout->setSpacing(0);
        setLayout(m_layout);
    }

    void FilterTagWidgetContainer::Reinit(const QVector<QString>& tags)
    {
        if (m_widget)
        {
            // Hide the old widget and request deletion.
            m_widget->hide();
            m_widget->deleteLater();
        }

        m_widget = new QWidget(this);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setAlignment(Qt::AlignLeft);
        hLayout->setMargin(0);
        hLayout->setSpacing(8);

        for(const QString& tag : tags)
        {
            FilterTagWidget* tagWidget = new FilterTagWidget(tag);

            // Add the tag widget to the current row.
            hLayout->addWidget(tagWidget);

            // Connect the clicked event of the close button of the tag widget to the remove tag function in the container.
            connect(tagWidget, &FilterTagWidget::RemoveClicked, this, [this, tagWidget]{ emit TagRemoved(tagWidget->text()); });
        }

        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        hLayout->addWidget(spacerWidget);

        m_widget->setLayout(hLayout);
        m_layout->addWidget(m_widget);

        setFixedHeight(30);
    }
} // namespace O3DE::ProjectManager
