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
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <QScopedPointer>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>
#endif

namespace Ui {
    class ColorPickerPage;
}

class ColorPickerPage : public QWidget
{
    Q_OBJECT //AUTOMOC

public:
    explicit ColorPickerPage(QWidget* parent = nullptr);
    ~ColorPickerPage() override;

private Q_SLOTS:
    void onColorPickWithPalettesButtonClicked();

private:
    void pickColor(AzQtComponents::ColorPicker::Configuration configuration, const QString& context = QString());

    QScopedPointer<Ui::ColorPickerPage> ui;
    AZ::Color m_lastColor;
};
