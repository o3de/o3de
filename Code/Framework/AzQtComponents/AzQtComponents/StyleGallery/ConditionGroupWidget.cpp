/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ConditionGroupWidget.h"
#include "ConditionWidget.h"
#include <QGridLayout>
#include <QToolButton>
#include <QWidgetItem>
#include <QLayoutItem>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOptionToolButton>

enum Action {
    ActionAdd,
    ActionRemove
};

class ActionButton : public QToolButton
{
public:
    explicit ActionButton(QWidget *parent = nullptr)
        : QToolButton(parent)
    {
        setAttribute(Qt::WA_Hover);
    }

    void paintEvent(QPaintEvent *)
    {
        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        auto state = (opt.state & QStyle::State_Sunken) ? QIcon::Active
                                                        : ((opt.state & QStyle::State_MouseOver) ? QIcon::Selected
                                                                                                 : QIcon::Normal);

        QPixmap pix = icon().pixmap(QSize(16, 16), state);
        QPainter p(this);
        QRect pixRect = pix.rect();
        pixRect.moveCenter(rect().center());
        pixRect.adjust(-2, 1, -2, 1);
        p.drawPixmap(pixRect , pix);
    }
};

ConditionGroupWidget::ConditionGroupWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QGridLayout(this))
{
    m_addIcon.addPixmap(QPixmap(":/stylesheet/img/condition_add.png"), QIcon::Normal);
    m_addIcon.addPixmap(QPixmap(":/stylesheet/img/condition_add_hover.png"), QIcon::Selected);
    m_addIcon.addPixmap(QPixmap(":/stylesheet/img/condition_add_pressed.png"), QIcon::Active);

    m_delIcon.addPixmap(QPixmap(":/stylesheet/img/condition_remove.png"), QIcon::Normal);
    m_delIcon.addPixmap(QPixmap(":/stylesheet/img/condition_remove_hover.png"), QIcon::Selected);
    m_delIcon.addPixmap(QPixmap(":/stylesheet/img/condition_remove_pressed.png"), QIcon::Active);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    m_layout->setHorizontalSpacing(0);

    // Lets have at least one condition
    appendCondition();
}

void ConditionGroupWidget::appendCondition()
{
    auto cond = new ConditionWidget();
    connect(cond, SIGNAL(geometryChanged()), this, SLOT(update()));
    m_layout->addWidget(cond, count(), 0);
    m_layout->addWidget(new ActionButton(), count() - 1, 1);
    updateButtons();
}

void ConditionGroupWidget::removeCondition(int n)
{
    if (n >= 0 && n < count() && count() > 1) {
        m_layout->removeItem(m_layout->itemAtPosition(n, 0));
        m_layout->removeItem(m_layout->itemAtPosition(n, 1));
        updateButtons();
    }
}

int ConditionGroupWidget::count() const
{
    return m_layout->rowCount();
}

void ConditionGroupWidget::paintEvent(QPaintEvent *)
{
    const int c = count();
    for (int i = 0; i < c - 1; ++i) {
        ConditionWidget *cond1 = conditionAt(i);
        ConditionWidget *cond2 = conditionAt(i + 1);
        if (!cond1 || !cond2)
            continue; // defensive, doesn't happen

        const int separatorHeight = 3;
        const int y = ((cond1->geometry().bottom() + cond2->y()) / 2) - (separatorHeight / 2);
        QPainter p(this);
        p.setPen(QColor(50, 52, 53));
        p.setBrush(QColor(94, 96, 96));
        p.drawRoundedRect(QRect(0, y, width() - 1, separatorHeight), 1, 1);
    }
}

ConditionWidget *ConditionGroupWidget::conditionAt(int row) const
{
    if (row < 0 || row >= count())
        return nullptr;

    if (auto item = m_layout->itemAtPosition(row, 0))
        return qobject_cast<ConditionWidget*>(item->widget());

    return nullptr;
}

void ConditionGroupWidget::updateButtons()
{
    update();
    const int n = count();
    for (int i = 0; i < n; ++i) {
        if (auto item = m_layout->itemAtPosition(i, 1)) {
            if (auto button = qobject_cast<QToolButton*>(item->widget())) {
                bool isLast = i == n - 1;
                button->setCheckable(true);
                button->setChecked(true);
                button->setIcon(isLast ? m_addIcon : m_delIcon);
                button->disconnect();

                if (isLast) {
                    connect(button, &QToolButton::clicked, this, &ConditionGroupWidget::appendCondition);
                } else {
                    connect(button, &QToolButton::clicked, this, [this, i] {
                        removeCondition(i);
                    });
                }
            }
        }
    }
}

#include "StyleGallery/moc_ConditionGroupWidget.cpp"
