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
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setAlignment(Qt::AlignTop);
        m_layout->addWidget(m_headerWidget);
        setLayout(m_layout);
    }

    ContentWidget::~ContentWidget()
    {
        CleanupOldWidget();
    }

    void ContentWidget::CleanupOldWidget()
    {
        // The content widget does not own the widget, thus remove it from the layout to not
        // auto delete it along with destructing the content widget.
        if (m_widget)
        {
            m_widget->hide();
            m_layout->removeWidget(m_widget);
            m_widget->setParent(nullptr);
        }
    }

    void ContentWidget::Update(QWidget* widget)
    {
        UpdateInternal(/*headerTitle=*/{}, /*iconFilename=*/{}, widget, /*showHeader=*/false);
    }

    void ContentWidget::UpdateWithHeader(const QString& headerTitle, const QString& iconFilename, QWidget* widget)
    {
        UpdateInternal(headerTitle, iconFilename, widget, /*showHeader=*/true);
    }

    void ContentWidget::UpdateInternal(const QString& headerTitle, const QString& iconFilename, QWidget* widget, bool showHeader)
    {
        CleanupOldWidget();
        m_widget = widget;

        if (widget)
        {
            m_layout->addWidget(widget);
            m_widget->show();
        }

        if (showHeader)
        {
            m_headerWidget->Update(headerTitle, iconFilename);
            m_headerWidget->show();
        }
        else
        {
            m_headerWidget->hide();
        }
    }

    void ContentWidget::Clear()
    {
        UpdateWithHeader(/*headerTitle=*/{}, /*iconFilename=*/{}, /*widget=*/nullptr);
    }
} // namespace EMStudio
