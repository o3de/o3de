/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Inspector/ContentHeaderWidget.h>
#include <QHBoxLayout>
#include <QLabel>

namespace EMStudio
{
    ContentHeaderWidget::ContentHeaderWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_iconLabel = new QLabel();

        m_titleLabel = new QLabel();
        m_titleLabel->setStyleSheet("font-weight: bold;");

        QHBoxLayout* filenameLayout = new QHBoxLayout();
        filenameLayout->setMargin(2);
        filenameLayout->addWidget(m_titleLabel, 0, Qt::AlignTop);

        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->addLayout(filenameLayout);

        QHBoxLayout* mainLayout = new QHBoxLayout(this);
        mainLayout->addWidget(m_iconLabel, 0, Qt::AlignLeft | Qt::AlignTop);
        mainLayout->addLayout(vLayout);
        mainLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    }

    void ContentHeaderWidget::Update(const QString& title, const QString& iconFilename)
    {
        m_titleLabel->setText(title);
        m_iconLabel->setPixmap(FindOrCreateIcon((iconFilename)));
    }

    const QPixmap& ContentHeaderWidget::FindOrCreateIcon(const QString& iconFilename)
    {
        // Check if we have already loaded the icon.
        const auto iterator = m_cachedIcons.find(iconFilename);
        if (iterator != m_cachedIcons.end())
        {
            return iterator.value();
        }

        // Load it in case the icon is not in the cache.
        m_cachedIcons[iconFilename] = QPixmap(iconFilename).scaled(QSize(s_iconSize, s_iconSize), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        return m_cachedIcons[iconFilename];
    }
} // namespace EMStudio
