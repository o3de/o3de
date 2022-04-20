/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Inspector/ContentHeaderWidget.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/Inspector/ContentWidget.h>
#include <QHBoxLayout>

namespace EMStudio
{
    ContentWidget::ContentWidget(QWidget* parent)
        : QWidget(parent)
    {
        m_headerWidget = new ContentHeaderWidget(this);

        m_layout = new QVBoxLayout();
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->addWidget(m_headerWidget);
        setLayout(m_layout);
    }

    void ContentWidget::Update(const QString& headerTitle, const QString& iconFilename, QWidget* widget)
    {
        if (m_widget)
        {
            m_widget->hide();
            m_widget->deleteLater();
        }
        m_widget = widget;

        if (widget)
        {
            m_layout->addWidget(widget);
            m_headerWidget->Update(headerTitle, iconFilename);
        }
    }

    void ContentWidget::Clear()
    {
        Update(/*headerTitle=*/{}, /*iconFilename=*/{}, /*widget=*/nullptr);
    }
} // namespace EMStudio
