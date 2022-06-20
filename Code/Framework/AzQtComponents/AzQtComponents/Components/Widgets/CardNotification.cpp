/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/Widgets/CardNotification.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace AzQtComponents
{
    CardNotification::CardNotification(QWidget* parent, const QString& title, const QIcon& icon, const QSize iconSize)
        : QFrame(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        QFrame* headerFrame = new QFrame(this);
        headerFrame->setObjectName("HeaderFrame");
        headerFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        // icon widget
        QLabel* iconLabel = new QLabel(headerFrame);
        iconLabel->setObjectName("Icon");
        iconLabel->setPixmap(icon.pixmap(iconSize));

        // title widget
        QLabel* titleLabel = new QLabel(title, headerFrame);
        titleLabel->setObjectName("Title");
        titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        titleLabel->setWordWrap(true);
        titleLabel->setTextFormat(Qt::RichText);
        titleLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        titleLabel->setOpenExternalLinks(true);

        QHBoxLayout* headerLayout = new QHBoxLayout(headerFrame);
        headerLayout->setSizeConstraint(QLayout::SetMinimumSize);
        headerLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        headerLayout->addWidget(iconLabel);
        headerLayout->addWidget(titleLabel);
        headerFrame->setLayout(headerLayout);

        m_featureLayout = new QVBoxLayout(this);
        m_featureLayout->setSizeConstraint(QLayout::SetMinimumSize);
        m_featureLayout->setContentsMargins(QMargins(0, 0, 0, 0));
        m_featureLayout->addWidget(headerFrame);
    }

    void CardNotification::addFeature(QWidget* feature)
    {
        feature->setParent(this);
        m_featureLayout->addWidget(feature);
    }

    QPushButton* CardNotification::addButtonFeature(const QString& buttonText)
    {
        QPushButton* featureButton = new QPushButton(buttonText, this);

        addFeature(featureButton);

        return featureButton;
    }

    #include <Components/Widgets/moc_CardNotification.cpp>
}

