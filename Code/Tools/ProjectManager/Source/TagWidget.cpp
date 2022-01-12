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
    TagWidget::TagWidget(const Tag& tag, QWidget* parent)
        : QLabel(tag.text, parent)
        , m_tag(tag)
    {
        setObjectName("TagWidget");
    }

    void TagWidget::mousePressEvent([[maybe_unused]] QMouseEvent* event)
    {
        emit TagClicked(m_tag);
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
        Clear();

        foreach (const QString& tag, tags)
        {
            TagWidget* tagWidget = new TagWidget({tag, tag});
            connect(tagWidget, &TagWidget::TagClicked, this, [=](const Tag& clickedTag){ emit TagClicked(clickedTag); });
            layout()->addWidget(tagWidget);
        }
    }

    void TagContainerWidget::Update(const QVector<Tag>& tags)
    {
        Clear();

        foreach (const Tag& tag, tags)
        {
            TagWidget* tagWidget = new TagWidget(tag);
            connect(tagWidget, &TagWidget::TagClicked, this, [=](const Tag& clickedTag){ emit TagClicked(clickedTag); });
            layout()->addWidget(tagWidget);
        }
    }

    void TagContainerWidget::Clear()
    {
        QLayoutItem* layoutItem = nullptr; 
        while ((layoutItem = layout()->takeAt(0)) != nullptr)
        {
            layoutItem->widget()->deleteLater();
        }
    }
} // namespace O3DE::ProjectManager
