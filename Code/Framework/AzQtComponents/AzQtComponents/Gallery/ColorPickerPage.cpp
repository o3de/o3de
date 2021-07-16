/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColorPickerPage.h"
#include <AzQtComponents/Gallery/ui_ColorPickerPage.h>

#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QDir>
#include <QFileDialog>
#include <QStringList>
#include <QDebug>

namespace
{
    QString getConfigurationName(AzQtComponents::ColorPicker::Configuration configuration)
    {
        switch (configuration)
        {
            case AzQtComponents::ColorPicker::Configuration::RGBA:
                return QStringLiteral("RGBA");
            case AzQtComponents::ColorPicker::Configuration::RGB:
                return QStringLiteral("RGB");
            case AzQtComponents::ColorPicker::Configuration::HueSaturation:
                return QStringLiteral("Lighting");
        }
        Q_UNREACHABLE();
    }
}

ColorPickerPage::ColorPickerPage(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ColorPickerPage)
    , m_lastColor(0.0f, 1.0f, 0.0f, 0.5f)
{
    ui->setupUi(this);

    QString exampleText = R"(

The ColorPicker is a dialog that allows the user to select a color, with support for a currently selected palette and user managed palettes.<br/>
It can be used as a modal dialog (via ColorPicker::getColor) or as a non-modal dialog (via ColorPicker::show and ColorPicker::selectedColor).<br/>
<br/>
Example:<br/>
<br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/ColorPicker.h&gt;

// To let the user picker a color:
QColor initialColor { 0, 0, 0, 0 };
QColor newColor = AzQtComponents::ColorPicker::getColor(AzQtComponents::ColorPicker::Configuration::RGB, initialColor, "Color Picker Dialog Title");

// Possible Configurations
AzQtComponents::ColorPicker::Configuration::RGBA
AzQtComponents::ColorPicker::Configuration::RGB
AzQtComponents::ColorPicker::Configuration::HueSaturation

</pre>

)";

    ui->exampleText->setHtml(exampleText);

    connect(ui->colorPickRGBAButton, &QPushButton::released, this, [this] { pickColor(AzQtComponents::ColorPicker::Configuration::RGBA); });
    connect(ui->colorPickRGBButton, &QPushButton::released, this, [this] { pickColor(AzQtComponents::ColorPicker::Configuration::RGB); });
    connect(ui->colorPickHueSaturationButton, &QPushButton::released, this, [this] { pickColor(AzQtComponents::ColorPicker::Configuration::HueSaturation); });
    connect(ui->colorPickRGBARulerButton, &QPushButton::released, this, [this] { pickColor(AzQtComponents::ColorPicker::Configuration::RGBA, QStringLiteral("Ruler")); });
    connect(ui->colorPickWithPalettesButton, &QPushButton::clicked, this, &ColorPickerPage::onColorPickWithPalettesButtonClicked);
}

ColorPickerPage::~ColorPickerPage()
{
}

void ColorPickerPage::pickColor(AzQtComponents::ColorPicker::Configuration configuration, const QString& context)
{
    m_lastColor = AzQtComponents::ColorPicker::getColor(configuration, m_lastColor, QStringLiteral("Color Picker (%1)").arg(getConfigurationName(configuration)), context, QStringList(), this);
    qDebug() << AzQtComponents::toQColor(m_lastColor);
}

void ColorPickerPage::onColorPickWithPalettesButtonClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select a Directory with palettes"),
        QDir::currentPath(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    m_lastColor = AzQtComponents::ColorPicker::getColor(AzQtComponents::ColorPicker::Configuration::RGBA, m_lastColor, tr("Color Picker With Palettes"), QString(), QStringList() << dir, this);
    qDebug() << AzQtComponents::toQColor(m_lastColor);
}
