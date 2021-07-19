/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzQtComponents/Components/ToolButtonWithWidget.h>

#include <QHBoxLayout>
#include <QSizePolicy>
#include <QToolButton>
#include <QToolBar>
#include <QEvent>

namespace AzQtComponents
{
    enum
    {
        ButtonHeightMin = 20,
        ButtonIconMargin = 4,
        WidthProportionButton = 3
    };

    ToolButtonWithWidget::ToolButtonWithWidget(QWidget* widget, QWidget* parent)
        : QWidget(parent)
        , m_button(new QToolButton(this))
        , m_widget(widget)
    {
        auto layout = new QHBoxLayout(this);
        layout->setSpacing(0);
        layout->setMargin(0);
        layout->addWidget(m_button);
        layout->addWidget(m_widget);
        m_button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        setFixedWidth(m_iconSize.height()* WidthProportionButton);
        layout->addSpacing(4);
        connect(m_button, &QAbstractButton::clicked, this, &ToolButtonWithWidget::clicked);

        connectToParentToolBar();
    }


    QSize ToolButtonWithWidget::sizeHint() const
    {
        return QSize(120, m_iconSize.height() + ButtonIconMargin);
    }

    void ToolButtonWithWidget::setIcon(const QIcon& icon)
    {
        m_button->setIcon(icon);
        setIconSize(m_iconSize);
    }

    void ToolButtonWithWidget::setIconSize(const QSize &iconSize)
    {
        if (!iconSize.isValid())
        {
            return;
        }

        m_iconSize = iconSize.height() + ButtonIconMargin < ButtonHeightMin
            ? QSize(ButtonHeightMin, ButtonHeightMin)
            : iconSize;

        const int newHeight = m_iconSize.height() + ButtonIconMargin;
        setFixedHeight(newHeight);
        m_widget->setMinimumHeight(newHeight);
        m_button->setFixedSize(QSize(newHeight, newHeight));
        m_button->setIconSize(m_iconSize);

        setFixedWidth(newHeight * WidthProportionButton);
    }

    QToolButton* ToolButtonWithWidget::button() const
    {
        return m_button;
    }

    QWidget* ToolButtonWithWidget::widget()
    {
        return m_widget;
    }

    bool ToolButtonWithWidget::event(QEvent *event)
    {
        if (event->type() == QEvent::ParentChange)
        {
            connectToParentToolBar();
        }

        return QWidget::event(event);
    }

    void ToolButtonWithWidget::connectToParentToolBar()
    {
        if (auto toolbar = qobject_cast<QToolBar*>(parent()))
        {
            connect(toolbar, &QToolBar::iconSizeChanged, this, &ToolButtonWithWidget::setIconSize, Qt::UniqueConnection);
            setIconSize(toolbar->iconSize());
        }
    }

} // namespace AzQtComponents

#include "Components/moc_ToolButtonWithWidget.cpp"
