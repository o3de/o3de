/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SegmentBar.h"

#include <AzQtComponents/Components/Style.h>

#include <QIcon>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QPushButton>
#include <QApplication>

namespace AzQtComponents
{
    bool layoutDirectionIsLeftToRight(QWidget* widget)
    {
        const Qt::LayoutDirection direction = widget->layoutDirection();
        return direction == Qt::LayoutDirectionAuto
                ? (widget->parentWidget()
                   ? layoutDirectionIsLeftToRight(widget->parentWidget())
                   : QApplication::isLeftToRight())
                : direction == Qt::LeftToRight;
    }

    QBoxLayout::Direction boxLayoutDirection(Qt::Orientation orientation, QWidget* widget)
    {
        const bool leftToRight = layoutDirectionIsLeftToRight(widget);
        if (orientation == Qt::Horizontal)
        {
            return leftToRight ? QBoxLayout::LeftToRight : QBoxLayout::RightToLeft;
        }
        return leftToRight ? QBoxLayout::TopToBottom : QBoxLayout::BottomToTop;
    }

    SegmentBar::SegmentBar(Qt::Orientation orientation, QWidget* parent)
        : QFrame(parent)
    {
        initialize(orientation);
    }

    SegmentBar::SegmentBar(QWidget* parent)
        : QFrame(parent)
    {
        initialize(Qt::Horizontal);
    }

    Qt::Orientation SegmentBar::orientation() const
    {
        switch (m_buttonsLayout->direction())
        {
            case QBoxLayout::LeftToRight:
            case QBoxLayout::RightToLeft:
                return Qt::Horizontal;
            case QBoxLayout::TopToBottom:
            case QBoxLayout::BottomToTop:
            default:
                return Qt::Vertical;
        }
    }

    void SegmentBar::setOrientation(Qt::Orientation orientation)
    {
        m_buttonsLayout->setDirection(boxLayoutDirection(orientation, this));
        if (orientation == Qt::Horizontal)
        {
            m_alignmentLayout->itemAt(0)->setAlignment(Qt::AlignCenter);
            setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        }
        else
        {
            m_alignmentLayout->itemAt(0)->setAlignment(Qt::AlignTop);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        }
    }

    int SegmentBar::addTab(const QString& text)
    {
        return insertTab(-1, QIcon(), text);
    }

    int SegmentBar::addTab(const QIcon& icon, const QString& text)
    {
        return insertTab(-1, icon, text);
    }

    int SegmentBar::insertTab(int index, const QString& text)
    {
        return insertTab(index, QIcon(), text);
    }

    int SegmentBar::insertTab(int index, const QIcon& icon, const QString& text)
    {
        const int i = qBound(index < 0 ? count() : 0, index, count());
        QAbstractButton* btn = createButton(icon, text);
        m_buttons->addButton(btn);
        m_buttonsLayout->insertWidget(i, btn);
        updateTabs();
        if (currentIndex() == -1)
        {
            btn->setChecked(true);
        }
        return i;
    }

    void SegmentBar::removeTab(int index)
    {
        if (QAbstractButton* btn = button(index))
        {
            const int curIndex = currentIndex();
            const bool isCurrent = index == curIndex;
            m_buttonsLayout->removeWidget(btn);
            m_buttons->removeButton(btn);
            updateTabs();
            if (isCurrent)
            {
                setCurrentIndex(count() > 0 ? qBound(0, index -1, count() -1) : -1);
            }
            else if (curIndex > index)
            {
                setCurrentIndex(curIndex -1);
            }
        }
    }

    void SegmentBar::moveTab(int from, int to)
    {
        if (from < 0 || from >= count() || to < 0 || to >= count() || from == to)
        {
            return;
        }
        const bool isCurrent = from == currentIndex();
        m_buttonsLayout->insertItem(to, m_buttonsLayout->takeAt(from));
        updateTabs();
        if (isCurrent)
        {
            emit currentChanged(to);
        }
    }

    bool SegmentBar::isTabEnabled(int index) const
    {
        const QAbstractButton* btn = button(index);
        return btn ? btn->isEnabled() : false;
    }

    void SegmentBar::setTabEnabled(int index, bool enabled)
    {
        if (QAbstractButton* btn = button(index))
        {
            btn->setEnabled(enabled);
        }
    }

    QString SegmentBar::tabText(int index) const
    {
        const QAbstractButton* btn = button(index);
        return btn ? btn->text() : QString();
    }

    void SegmentBar::setTabText(int index, const QString& text)
    {
        if (QAbstractButton* btn = button(index))
        {
            btn->setText(text);
            updateTabs();
        }
    }

    QColor SegmentBar::tabTextColor(int index) const
    {
        const QAbstractButton* btn = button(index);
        return btn ? btn->palette().brush(QPalette::Active, QPalette::Text).color() : QColor();
    }

    void SegmentBar::setTabTextColor(int index, const QColor& color)
    {
        if (QAbstractButton* btn = button(index))
        {
            QPalette pal = btn->palette();
            pal.setBrush(QPalette::Active, QPalette::Text, color);
            btn->setPalette(pal);
        }
    }

    QIcon SegmentBar::tabIcon(int index) const
    {
        const QAbstractButton* btn = button(index);
        return btn ? btn->icon() : QIcon();
    }

    void SegmentBar::setTabIcon(int index, const QIcon& icon)
    {
        if (QAbstractButton* btn = button(index))
        {
            btn->setIcon(icon);
            updateTabs();
        }
    }

#ifndef QT_NO_TOOLTIP
    void SegmentBar::setTabToolTip(int index, const QString& tip)
    {
        if (QAbstractButton* btn = button(index))
        {
            btn->setToolTip(tip);
        }
    }

    QString SegmentBar::tabToolTip(int index) const
    {
        const QAbstractButton* btn = button(index);
        return btn ? btn->toolTip() : QString();
    }
#endif

#ifndef QT_NO_WHATSTHIS
    void SegmentBar::setTabWhatsThis(int index, const QString& text)
    {
        if (QAbstractButton* btn = button(index))
        {
            btn->setWhatsThis(text);
        }
    }

    QString SegmentBar::tabWhatsThis(int index) const
    {
        const QAbstractButton* btn = button(index);
        return btn ? btn->whatsThis() : QString();
    }
#endif

    QSize SegmentBar::iconSize() const
    {
        return m_iconSize;
    }

    void SegmentBar::setIconSize(const QSize& size)
    {
        if (m_iconSize == size)
        {
            return;
        }
        m_iconSize = size;
        updateTabs();
    }

    int SegmentBar::currentIndex() const
    {
        return m_buttonsLayout->indexOf(m_buttons->checkedButton());
    }

    int SegmentBar::count() const
    {
        return m_buttonsLayout->count();
    }

    void SegmentBar::setCurrentIndex(int index)
    {
        if ((index == -1 && count() > 0) || index >= count())
        {
            return;
        }
        if (QAbstractButton* btn = button(index))
        {
            btn->setChecked(true);
        }
        else
        {
            Q_ASSERT(count() == 0);
            emit currentChanged(-1);
        }
    }

    void SegmentBar::initialize(Qt::Orientation orientation)
    {
        m_minimumTabSize = -1;

        QWidget* buttonsWidget = new QWidget(this);

        m_buttons = new QButtonGroup(buttonsWidget);
        m_buttons->setExclusive(true);

        m_buttonsLayout = new QBoxLayout(QBoxLayout::LeftToRight, buttonsWidget);
        m_buttonsLayout->setContentsMargins(0, 0, 0, 0);
        m_buttonsLayout->setSpacing(0);

        m_alignmentLayout = new QBoxLayout(QBoxLayout::LeftToRight, this);
        m_alignmentLayout->setContentsMargins(0, 0, 0, 0);
        m_alignmentLayout->setSpacing(0);
        m_alignmentLayout->addWidget(buttonsWidget);

        setOrientation(orientation);

        connect(m_buttons, static_cast<void(QButtonGroup::*)(QAbstractButton*)>(&QButtonGroup::buttonClicked),
                this, [this](QAbstractButton* btn)
                {
                    emit tabBarClicked(m_buttonsLayout->indexOf(btn));
                });
        connect(m_buttons, static_cast<void(QButtonGroup::*)(QAbstractButton*, bool)>(&QButtonGroup::buttonToggled),
                this, [this](QAbstractButton* btn, bool checked)
                {
                    if (checked)
                    {
                        emit currentChanged(m_buttonsLayout->indexOf(btn));
                    }
                });
    }

    void SegmentBar::updateTabs()
    {
        m_minimumTabSize = -1;

        const QAbstractButton* first = button(0);
        const QAbstractButton* last = button(count() - 1);
        const auto buttons = m_buttons->buttons();
        const bool enabled = updatesEnabled();
        setUpdatesEnabled(false);
        m_alignmentLayout->setEnabled(false);
        m_buttonsLayout->setEnabled(false);

        for (QAbstractButton* btn : buttons)
        {
            Style::removeClass(btn, QStringLiteral("TabOne"));
            Style::removeClass(btn, QStringLiteral("TabFirst"));
            Style::removeClass(btn, QStringLiteral("TabLast"));
            Style::removeClass(btn, QStringLiteral("TabMiddle"));

            if (first == last && first == btn)
            {
                Style::addClass(btn, QStringLiteral("TabOne"));
            }
            else if (first == btn)
            {
                Style::addClass(btn, QStringLiteral("TabFirst"));
            }
            else if (last == btn)
            {
                Style::addClass(btn, QStringLiteral("TabLast"));
            }
            else
            {
                Style::addClass(btn, QStringLiteral("TabMiddle"));
            }

            btn->setIconSize(m_iconSize);
            m_minimumTabSize = qMax(m_minimumTabSize, btn->sizeHint().width());
        }

        for (QAbstractButton* btn : buttons)
        {
            btn->setMinimumWidth(m_minimumTabSize);
        }

        m_buttonsLayout->setEnabled(true);
        m_buttonsLayout->activate();
        m_alignmentLayout->setEnabled(true);
        m_alignmentLayout->activate();
        setUpdatesEnabled(enabled);
    }

    QAbstractButton* SegmentBar::createButton(const QIcon& icon, const QString& text)
    {
        QPushButton* btn = new QPushButton(this);
        btn->setCheckable(true);
        btn->setIcon(icon);
        btn->setText(text);
        btn->setFocusPolicy(Qt::NoFocus);
        Style::flagToIgnore(btn);
        return btn;
    }

    QAbstractButton* SegmentBar::button(int index) const
    {
        QLayoutItem* item = m_buttonsLayout->itemAt(index);
        return item ? qobject_cast<QAbstractButton*>(item->widget()) : nullptr;
    }
}

#include "Components/Widgets/moc_SegmentBar.cpp"
