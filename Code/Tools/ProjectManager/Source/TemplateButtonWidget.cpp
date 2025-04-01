/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TemplateButtonWidget.h>
#include <ProjectManagerDefs.h>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QAbstractButton>
#include <QProgressBar>
#include <QSpacerItem>
#include <QStyle>
#include <QVariant>

namespace O3DE::ProjectManager
{
    TemplateButton::TemplateButton(const QString& imagePath, const QString& labelText, QWidget* parent)
        : QPushButton(parent)
    {
        setAutoExclusive(true);

        setObjectName("templateButton");

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setSpacing(0);
        vLayout->setContentsMargins(0, 0, 0, 0);
        setLayout(vLayout);

        QLabel* image = new QLabel(this);
        image->setObjectName("templateImage");
        image->setPixmap(QPixmap(imagePath).scaled(
            QSize(ProjectTemplateImageWidth, ProjectTemplateImageWidth), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        vLayout->addWidget(image);

        QLabel* label = new QLabel(labelText, this);
        label->setObjectName("templateLabel");
        label->setWordWrap(true);
        vLayout->addWidget(label);

        // Create an overlay for remote templates
        QGridLayout* overlayLayout = new QGridLayout();
        overlayLayout->setAlignment(Qt::AlignTop);
        overlayLayout->setSpacing(0);
        overlayLayout->setContentsMargins(0, 0, 0, 0);

        // Dark overlay to make text clearer
        m_darkenOverlay = new QLabel(this);
        m_darkenOverlay->setObjectName("labelButtonOverlay");
        m_darkenOverlay->setFixedSize(ProjectTemplateImageWidth, ProjectTemplateImageWidth);
        m_darkenOverlay->setVisible(false);
        overlayLayout->addWidget(m_darkenOverlay,0,0);

        QVBoxLayout* contentsLayout = new QVBoxLayout();
        contentsLayout->setSpacing(0);
        contentsLayout->setContentsMargins(0, 0, 0, 0);
        contentsLayout->setAlignment(Qt::AlignTop);
        overlayLayout->addLayout(contentsLayout, 0, 0);

        // Top status icons
        QHBoxLayout* statusIconsLayout = new QHBoxLayout();
        statusIconsLayout->setSpacing(0);
        statusIconsLayout->setContentsMargins(0, 0, 0, 0);
        statusIconsLayout->addStretch();
        m_cloudIcon = new QLabel(this);
        m_cloudIcon->setObjectName("projectCloudIconOverlay");
        m_cloudIcon->setPixmap(QIcon(":/Download.svg").pixmap(24, 24));
        m_cloudIcon->setVisible(false);
        statusIconsLayout->addWidget(m_cloudIcon);
        statusIconsLayout->addSpacing(5);
        image->setLayout(overlayLayout);

        QWidget* statusIconArea = new QWidget();
        statusIconArea->setFixedSize(ProjectTemplateImageWidth, 24);
        statusIconArea->setLayout(statusIconsLayout);
        contentsLayout->addWidget(statusIconArea);

        QWidget* templateCenter = new QWidget();
        templateCenter->setFixedSize(ProjectTemplateImageWidth, 35);

        // Center area for download information
        QVBoxLayout* centerBlock = new QVBoxLayout();
        templateCenter->setLayout(centerBlock);

        QHBoxLayout* downloadProgressTextBlock = new QHBoxLayout();
        m_progressMessageLabel = new QLabel(tr("0%"), this);
        m_progressMessageLabel->setAlignment(Qt::AlignRight);
        m_progressMessageLabel->setVisible(false);
        downloadProgressTextBlock->addWidget(m_progressMessageLabel);
        centerBlock->addLayout(downloadProgressTextBlock);

        QHBoxLayout* downloadProgressBlock = new QHBoxLayout();
        m_progessBar = new QProgressBar(this);
        m_progessBar->setValue(100);
        m_progessBar->setVisible(false);

        downloadProgressBlock->addSpacing(10);
        downloadProgressBlock->addWidget(m_progessBar);
        downloadProgressBlock->addSpacing(10);
        centerBlock->addLayout(downloadProgressBlock);
        contentsLayout->addWidget(templateCenter);

        QSpacerItem* spacer =
            new QSpacerItem(ProjectTemplateImageWidth, ProjectTemplateImageWidth - 35 - 24, QSizePolicy::Fixed, QSizePolicy::Fixed);
        contentsLayout->addSpacerItem(spacer);

        connect(this, &QAbstractButton::toggled, this, &TemplateButton::onToggled);
    }

    void TemplateButton::SetIsRemote(bool isRemote)
    {
        if (!isRemote)
        {
            ShowDownloadProgress(false);
        }
        m_cloudIcon->setVisible(isRemote);
    }

    void TemplateButton::ShowDownloadProgress(bool showProgress)
    {
        m_progessBar->setVisible(showProgress);
        m_progressMessageLabel->setVisible(showProgress);
        m_darkenOverlay->setVisible(showProgress);
    }

    void TemplateButton::SetProgressPercentage(float percentage)
    {
        m_progessBar->setValue(static_cast<int>(percentage));
        m_progressMessageLabel->setText(QString("%1%").arg(static_cast<int>(percentage)));
    }

    void TemplateButton::onToggled()
    {
        setProperty("Checked", isChecked());

        // we must unpolish/polish every child after changing a property
        // or else they won't use the correct stylesheet selector
        for (auto child : findChildren<QWidget*>())
        {
            child->style()->unpolish(child);
            child->style()->polish(child);
        }

        style()->unpolish(this);
        style()->polish(this);
    }
} // namespace O3DE::ProjectManager
