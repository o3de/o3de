/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SliderPage.h"
#include <AzQtComponents/Gallery/ui_SliderPage.h>

#include <AzQtComponents/Components/Widgets/Slider.h>

SliderPage::SliderPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::SliderPage)
{
    ui->setupUi(this);

    ui->percentage->setToolTipFormatting("Opacity ", "%");
    ui->percentage->setRange(0, 100);

    AzQtComponents::Slider::applyMidPointStyle(ui->midPointSliderDisabled);
    AzQtComponents::Slider::applyMidPointStyle(ui->midPointSliderEnabled);
    AzQtComponents::Slider::applyMidPointStyle(ui->verticalMidPointSliderEnabled);
    AzQtComponents::Slider::applyMidPointStyle(ui->verticalMidPointSliderDisabled);

    AzQtComponents::Slider::applyMidPointStyle(ui->doubleMidPointSliderDisabled);
    AzQtComponents::Slider::applyMidPointStyle(ui->doubleMidPointSliderEnabled);
    AzQtComponents::Slider::applyMidPointStyle(ui->doubleVerticalMidPointSliderEnabled);
    AzQtComponents::Slider::applyMidPointStyle(ui->doubleVerticalMidPointSliderDisabled);

    {
        ui->sliderEnabled->setValue(20);
        ui->sliderDisabled->setValue(40);

        ui->verticalSliderEnabled->setValue(20);
        ui->verticalSliderDisabled->setValue(0);

        ui->doubleSliderEnabled->setRange(0, 10);
        ui->doubleSliderEnabled->setValue(6.0);

        ui->doubleSliderDisabled->setValue(0);
    }

    {
        int min = -10;
        int max = 10;

        ui->midPointSliderDisabled->setRange(min, max);
        ui->midPointSliderDisabled->setValue(5);

        ui->midPointSliderEnabled->setRange(min, max);
        ui->midPointSliderEnabled->setValue(2);

        ui->verticalMidPointSliderEnabled->setRange(min, max);
        ui->verticalMidPointSliderEnabled->setValue(-2);

        ui->verticalMidPointSliderDisabled->setRange(min, max);
        ui->verticalMidPointSliderDisabled->setValue(-5);
    }

    {
        int min = -20;
        int max = 20;
        ui->doubleMidPointSliderDisabled->setRange(min, max);
        ui->doubleMidPointSliderDisabled->setValue(10.0);

        ui->doubleMidPointSliderEnabled->setRange(min, max);
        ui->doubleMidPointSliderEnabled->setValue(4);

        ui->doubleVerticalMidPointSliderEnabled->setRange(min, max);
        ui->doubleVerticalMidPointSliderEnabled->setValue(-4.0);

        ui->doubleVerticalMidPointSliderDisabled->setRange(min, max);
        ui->doubleVerticalMidPointSliderDisabled->setValue(-10.0);
    }

    {
        double min = 0.0;
        double max = 1.0;
        double value = 0.5;

        const auto curveMidpointSliders =
        {
            ui->curveMidpoint25,
            ui->curveMidpoint5,
            ui->curveMidpoint75,
            ui->verticalcurveMidpoint25,
            ui->verticalcurveMidpoint5,
            ui->verticalcurveMidpoint75
        };

        for (auto slider : curveMidpointSliders)
        {
            slider->setRange(min, max);
            slider->setValue(value);
            AzQtComponents::Slider::applyMidPointStyle(slider);
        }

        ui->curveMidpoint25->setCurveMidpoint(0.25);
        ui->verticalcurveMidpoint25->setCurveMidpoint(0.25);

        ui->curveMidpoint75->setCurveMidpoint(0.75);
        ui->verticalcurveMidpoint75->setCurveMidpoint(0.75);
    }

    QString exampleText = R"(

A Slider is a wrapper around a QSlider.<br/>
There are two variants: SliderInt and SliderDouble, for working with signed integers and doubles, respectively.<br/>
They add tooltip functionality, as well as conversion to the proper data types (int and double).
<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/Slider.h&gt;
#include &lt;QDebug&gt;

// Here's an example that creates a slider and sets the hover/tooltip indicator to display as a percentage
SliderInt* sliderInt = new SliderInt();
sliderInt->setRange(0, 100);
sliderInt->setToolTipFormatting("", "%");

// Assuming you've created a slider already (either in code or via .ui file), give it the mid point style like this:
Slider::applyMidPointStyle(sliderInt);

// Disable it like this:
sliderInt->setEnabled(false);

// Here's an example of creating a SliderDouble and setting it up:
SliderDouble* sliderDouble = new SliderDouble();
double min = -10.0;
double max = 10.0;
int numSteps = 21;
sliderDouble->setRange(min, max, numSteps);
sliderDouble->setValue(0.0);

// Listen for changes; same format for SliderInt as SliderDouble
connect(sliderDouble, &SliderDouble::valueChanged, sliderDouble, [sliderDouble](double newValue){
    qDebug() &lt;&lt; "Slider value changed to " &lt;&lt; newValue;
});

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

SliderPage::~SliderPage()
{
}

#include "Gallery/moc_SliderPage.cpp"
