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

#include "GradientSliderPage.h"
#include <AzQtComponents/Utilities/Conversions.h>
#include "Gallery/ui_GradientSliderPage.h"

#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <AzCore/Casting/numeric_cast.h>

GradientSliderPage::GradientSliderPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::GradientSliderPage)
{
    ui->setupUi(this);

    ui->slider1->setColorFunction([](qreal position) {
        return AzQtComponents::toQColor(1.0, 0.0, 0.0, aznumeric_cast<float>(position));
    });

    ui->slider2->setColorFunction([](qreal position) {
        return AzQtComponents::toQColor(aznumeric_cast<float>(position), 1.0, 0.5);
    });

    ui->slider3->setColorFunction([](qreal position) {
        return AzQtComponents::toQColor(0.0, 0.0, aznumeric_cast<float>(position));
    });

    QString exampleText = R"(
A GradientSlider is a QSlider with a color gradient on the background, described by a user-defined function. It's primarily meant to be used by the Color Picker dialog.<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/GradientSlider.h&gt;

GradientSlider *slider;

// Set a color function for the slider background. The argument goes from 0.0 to 1.0.
slider->setColorFunction([](qreal position) { return QColor::fromHslF(position, 1.0, 0.5); });

// To recompute the background colors:
slider->updateGradient();
</pre>
)";

    ui->exampleText->setHtml(exampleText);
}

#include "Gallery/moc_GradientSliderPage.cpp"
