/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
