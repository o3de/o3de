/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SegmentControl.h"
#include "SegmentBar.h"

#include <QBoxLayout>
#include <QStackedLayout>
#include <QIcon>

namespace AzQtComponents
{
    bool isVerticalLayout(SegmentControl::TabPosition position)
    {
        return position == SegmentControl::North || position == SegmentControl::South;
    }

    int positionIndex(SegmentControl::TabPosition position)
    {
        return position == SegmentControl::North || position == SegmentControl::West ? 0 : 1;
    }

    SegmentControl::SegmentControl(SegmentControl::TabPosition position, QWidget* parent)
        : QFrame(parent)
    {
        initialize(isVerticalLayout(position) ? Qt::Horizontal : Qt::Vertical, position);
    }

    SegmentControl::SegmentControl(SegmentBar* bar, SegmentControl::TabPosition position, QWidget* parent)
        : QFrame(parent)
    {
        initialize(bar, position);
    }

    SegmentControl::SegmentControl(QWidget* parent)
        : QFrame(parent)
    {
        initialize(Qt::Horizontal, SegmentControl::North);
    }

    Qt::Orientation SegmentControl::tabOrientation() const
    {
        return m_segmentBar->orientation();
    }

    void SegmentControl::setTabOrientation(Qt::Orientation orientation)
    {
        m_segmentBar->setOrientation(orientation);
    }

    SegmentControl::TabPosition SegmentControl::tabPosition() const
    {
        switch (m_layout->direction())
        {
            case QBoxLayout::LeftToRight:
            case QBoxLayout::RightToLeft:
                return m_layout->indexOf(m_segmentBar) == 0 ? SegmentControl::West : SegmentControl::East;
            case QBoxLayout::TopToBottom:
            case QBoxLayout::BottomToTop:
            default:
                return m_layout->indexOf(m_segmentBar) == 0 ? SegmentControl::North : SegmentControl::South;
        }
    }

    void SegmentControl::setTabPosition(SegmentControl::TabPosition position)
    {
        const int from = m_layout->indexOf(m_segmentBar);
        const int to = positionIndex(position);
        m_layout->setDirection(isVerticalLayout(position) ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
        m_layout->insertItem(to, m_layout->takeAt(from));
    }

    int SegmentControl::addTab(QWidget* widget, const QString& text)
    {
        return insertTab(-1, widget, QIcon(), text);
    }

    int SegmentControl::addTab(QWidget* widget, const QIcon& icon, const QString& text)
    {
        return insertTab(-1, widget, icon, text);
    }

    int SegmentControl::insertTab(int index, QWidget* widget, const QString& text)
    {
        return insertTab(index, widget, QIcon(), text);
    }

    int SegmentControl::insertTab(int index, QWidget* widget, const QIcon& icon, const QString& text)
    {
        const int i = m_segmentBar->insertTab(index, icon, text);
        m_widgets->insertWidget(i, widget);
        m_widgets->setCurrentIndex(m_segmentBar->currentIndex());
        return i;
    }

    void SegmentControl::removeTab(int index)
    {
        if (QWidget* w = widget(index))
        {
            m_widgets->removeWidget(w);
            m_segmentBar->removeTab(index);
        }
    }

    void SegmentControl::moveTab(int from, int to)
    {
        if (from < 0 || from >= count() || to < 0 || to >= count() || from == to)
        {
            return;
        }
        QLayoutItem* item = m_widgets->takeAt(from);
        m_widgets->insertWidget(to, item->widget());
        m_segmentBar->moveTab(from, to);
        delete item;
    }

    bool SegmentControl::isTabEnabled(int index) const
    {
        return m_segmentBar->isTabEnabled(index);
    }

    void SegmentControl::setTabEnabled(int index, bool enabled)
    {
        m_segmentBar->setTabEnabled(index, enabled);
    }

    QString SegmentControl::tabText(int index) const
    {
        return m_segmentBar->tabText(index);
    }

    void SegmentControl::setTabText(int index, const QString& text)
    {
        m_segmentBar->setTabText(index, text);
    }

    QIcon SegmentControl::tabIcon(int index) const
    {
        return m_segmentBar->tabIcon(index);
    }

    void SegmentControl::setTabIcon(int index, const QIcon& icon)
    {
        m_segmentBar->setTabIcon(index, icon);
    }

#ifndef QT_NO_TOOLTIP
    void SegmentControl::setTabToolTip(int index, const QString& tip)
    {
        m_segmentBar->setTabToolTip(index, tip);
    }

    QString SegmentControl::tabToolTip(int index) const
    {
        return m_segmentBar->tabToolTip(index);
    }
#endif

#ifndef QT_NO_WHATSTHIS
    void SegmentControl::setTabWhatsThis(int index, const QString& text)
    {
        m_segmentBar->setTabWhatsThis(index, text);
    }

    QString SegmentControl::tabWhatsThis(int index) const
    {
        return m_segmentBar->tabWhatsThis(index);
    }
#endif

    QSize SegmentControl::iconSize() const
    {
        return m_segmentBar->iconSize();
    }

    void SegmentControl::setIconSize(const QSize& size)
    {
        m_segmentBar->setIconSize(size);
    }

    int SegmentControl::currentIndex() const
    {
        return m_segmentBar->currentIndex();
    }

    QWidget* SegmentControl::currentWidget() const
    {
        return m_widgets->currentWidget();
    }

    int SegmentControl::count() const
    {
        return m_segmentBar->count();
    }

    void SegmentControl::clear()
    {
        while (count() > 0)
            removeTab(0);
    }

    SegmentBar* SegmentControl::tabBar() const
    {
        return m_segmentBar;
    }

    void SegmentControl::setCurrentIndex(int index)
    {
        m_segmentBar->setCurrentIndex(index);
    }

    void SegmentControl::setCurrentWidget(QWidget* widget)
    {
        setCurrentIndex(indexOf(widget));
    }

    void SegmentControl::initialize(Qt::Orientation orientation, SegmentControl::TabPosition position)
    {
        initialize(new SegmentBar(orientation, this), position);
    }

    void SegmentControl::initialize(SegmentBar* bar, SegmentControl::TabPosition position)
    {
        m_segmentBar = bar;

        m_widgets = new QStackedLayout;
        m_widgets->setContentsMargins(0, 0, 0, 0);
        m_widgets->setSpacing(0);

        m_layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(0);
        m_layout->addWidget(m_segmentBar);
        m_layout->addLayout(m_widgets);

        setTabPosition(position);

        connect(m_segmentBar, &SegmentBar::currentChanged, this, [this](int index) {
            m_widgets->setCurrentIndex(index);
            emit currentChanged(index);
        });
        connect(m_segmentBar, &SegmentBar::tabBarClicked, this, &SegmentControl::tabBarClicked);
    }

    QWidget* SegmentControl::widget(int index) const
    {
        QLayoutItem* item = m_widgets->itemAt(index);
        return item ? item->widget() : nullptr;
    }

    int SegmentControl::indexOf(QWidget* widget) const
    {
        return m_widgets->indexOf(widget);
    }
}

#include "Components/Widgets/moc_SegmentControl.cpp"
