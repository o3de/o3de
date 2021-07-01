/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ConditionWidget.h"
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>

ConditionWidget::ConditionWidget(QWidget *parent)
    : QWidget(parent)
{
    auto layout = new QHBoxLayout(this);
    auto checkbox = new QCheckBox();
    auto combo1 = new QComboBox();
    combo1->addItem(QStringLiteral("Asset Type"));
    combo1->addItem(QStringLiteral("Asset Name"));
    combo1->setProperty("class", "condition");
    auto combo2 = new QComboBox();
    combo2->setFixedWidth(88);
    combo2->setProperty("class", "condition");
    combo2->addItem(QStringLiteral("is"));
    combo2->addItem(QStringLiteral("is not"));
    auto lineedit = new QLineEdit();

    layout->addWidget(checkbox);
    layout->addWidget(combo1);
    layout->addWidget(combo2);
    layout->addWidget(lineedit);
    layout->setSpacing(5);

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
}

void ConditionWidget::resizeEvent(QResizeEvent *event)
{
   QWidget::resizeEvent(event);
   emit geometryChanged();
}

void ConditionWidget::moveEvent(QMoveEvent *event)
{
    QWidget::moveEvent(event);
    emit geometryChanged();
}

#include "StyleGallery/moc_ConditionWidget.cpp"
