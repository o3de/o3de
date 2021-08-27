/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliderComboPage.h"
#include <AzQtComponents/Gallery/ui_SliderComboPage.h>

#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>

#include <QDebug>

using namespace AzQtComponents;

SliderComboPage::SliderComboPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SliderComboPage)
{
    ui->setupUi(this);

    ui->verticalSliderComboWithoutValue->setRange(0, 100);

    ui->verticalSliderCombo->setRange(0, 100);
    ui->verticalSliderCombo->setValue(50);

    ui->verticalSliderComboWithSoftRange->setRange(-500, 1000);
    ui->verticalSliderComboWithSoftRange->setSoftRange(0, 500);
    ui->verticalSliderComboWithSoftRange->setValue(250);

    ui->verticalSliderDoubleComboWithoutValue->setRange(0.5, 250.5);

    ui->verticalSliderDoubleCombo->setRange(0.5, 250.5);
    ui->verticalSliderDoubleCombo->setValue(100.0);

    ui->verticalSliderDoubleComboWithSoftRange->setRange(-500.0, 1000.0);
    ui->verticalSliderDoubleComboWithSoftRange->setSoftRange(0.0, 500.0);
    ui->verticalSliderDoubleComboWithSoftRange->setValue(250.0);

    ui->curveSliderDoubleCombo->setRange(0.0, 1.0);
    ui->curveSliderDoubleCombo->setCurveMidpoint(0.25);
    ui->curveSliderDoubleCombo->setValue(0.25);

    connect(ui->verticalSliderCombo, &SliderCombo::valueChanged, this, &SliderComboPage::sliderValueChanged);
    connect(ui->verticalSliderComboWithoutValue, &SliderCombo::valueChanged, this, &SliderComboPage::sliderValueChanged);
    connect(ui->verticalSliderComboWithSoftRange, &SliderCombo::valueChanged, this, &SliderComboPage::sliderValueChanged);

    connect(ui->verticalSliderDoubleCombo, &SliderDoubleCombo::valueChanged, this, &SliderComboPage::sliderValueChanged);
    connect(ui->verticalSliderDoubleComboWithoutValue, &SliderDoubleCombo::valueChanged, this, &SliderComboPage::sliderValueChanged);
    connect(ui->verticalSliderDoubleComboWithSoftRange, &SliderDoubleCombo::valueChanged, this, &SliderComboPage::sliderValueChanged);

    QString exampleText = R"(

A Slider Combo is a widget which combines a Slider and a Spin Box.<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/SliderCombo.h&gt;

// Here's an example that creates a sliderCombo
SliderCombo* sliderCombo = new SliderCombo();
sliderCombo->setRange(0, 100);

// Set the starting value
sliderCombo->setValue(50);
</pre>

)";


    ui->exampleText->setHtml(exampleText);
}

SliderComboPage::~SliderComboPage()
{
}

void SliderComboPage::sliderValueChanged()
{
    if (auto doubleCombo = qobject_cast<SliderDoubleCombo*>(sender()))
        qDebug() << "Double combo slider valueChanged:" << doubleCombo->value();
    else if (auto combo = qobject_cast<SliderCombo*>(sender()))
        qDebug() << "Combo slider valueChanged:" << combo->value();
    else
        Q_UNREACHABLE();
}

#include <Gallery/moc_SliderComboPage.cpp>
