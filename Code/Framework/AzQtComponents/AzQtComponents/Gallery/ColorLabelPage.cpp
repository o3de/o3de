/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColorLabelPage.h"
#include <AzQtComponents/Gallery/ui_ColorLabelPage.h>

#include <AzQtComponents/Utilities/Conversions.h>

#include <QDebug>

ColorLabelPage::ColorLabelPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ColorLabelPage)
{
    ui->setupUi(this);
    ui->colorLabelNoInput->setTextInputVisible(false);

    connect(ui->colorLabel, &AzQtComponents::ColorLabel::colorChanged, this, &ColorLabelPage::onColorChanged);
    connect(ui->colorLabelNoInput, &AzQtComponents::ColorLabel::colorChanged, this, &ColorLabelPage::onColorChanged);

    QString exampleText = R"(

The ColorLabel is a widget that allows the user to select and preview a color, with support to add a line edit to input color hex codes.<br/>
<br/>
Example:<br/>
<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/ColorLabel.h&gt;

ColorLabel* label = new ColorLabel();

// hide the hex code line edit (by default it is shown)
label->setTextInputVisible(false);

// Color Labels show a Color Picker with configuration AzQtComponents::ColorPicker::Configuration::RGB on click

</pre>

)";

    ui->exampleText->setHtml(exampleText);
}

ColorLabelPage::~ColorLabelPage()
{
}

void ColorLabelPage::onColorChanged(const AZ::Color& color) const
{
    qDebug() << AzQtComponents::toQColor(color);
}

#include "Gallery/moc_ColorLabelPage.cpp"
