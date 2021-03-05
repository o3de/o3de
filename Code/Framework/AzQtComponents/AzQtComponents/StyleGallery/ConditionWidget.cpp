/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
