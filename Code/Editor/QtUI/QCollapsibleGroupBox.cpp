/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDefs.h"

#include "QtUI/QCollapsibleGroupBox.h"


QCollapsibleGroupBox::QCollapsibleGroupBox(QWidget* parent)
    : QGroupBox(parent)
    , m_collapsed(false)
    , m_toggleButton(0)
{
    m_toggleButton = new QToolButton(this);
    m_toggleButton->setFixedSize(16, 16);
    m_toggleButton->setArrowType(Qt::DownArrow);
    connect(m_toggleButton, &QToolButton::clicked, this, [&]()
        {
            setCollapsed(!m_collapsed);
        });
}

QCollapsibleGroupBox::~QCollapsibleGroupBox()
{
}

void QCollapsibleGroupBox::setCollapsed(bool v)
{
    if (v == m_collapsed)
    {
        return;
    }

    m_collapsed = v;
    if (m_collapsed)
    {
        m_visibleState.clear();
    }
    Q_FOREACH(QObject * child, children())
    {
        QWidget* w = qobject_cast<QWidget*>(child);
        if (w && w != m_toggleButton)
        {
            if (m_collapsed)
            {
                m_visibleState[w] = w->isVisible();
                w->setHidden(true);
            }
            else
            {
                if (m_visibleState.contains(w))
                {
                    w->setVisible(m_visibleState[w]);
                }
                else
                {
                    w->setVisible(true);
                }
            }
        }
    }
    m_toggleButton->setArrowType(v ? Qt::LeftArrow : Qt::DownArrow);
    adaptSize(v);
    Q_EMIT collapsed(v);
}

void QCollapsibleGroupBox::resizeEvent(QResizeEvent* event)
{
    if (m_toggleButton)
    {
        m_toggleButton->move(event->size().width() - m_toggleButton->width() - 1, 1);
    }
    QGroupBox::resizeEvent(event);
}

void QCollapsibleGroupBox::adaptSize(bool v)
{
    if (v)
    {
        m_expandedSize = maximumSize();
        setMaximumHeight(fontMetrics().height() + 5); // shrinking to just let the groupbox title
    }
    else
    {
        setMaximumHeight(m_expandedSize.height());
    }
}

#include <QtUI/moc_QCollapsibleGroupBox.cpp>
