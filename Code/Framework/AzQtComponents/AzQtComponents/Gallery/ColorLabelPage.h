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
#endif

#include <AzCore/Math/Color.h>
#include <AzQtComponents/Components/Widgets/ColorPicker.h>

namespace Ui
{
    class ColorLabelPage;
}

class ColorLabelPage : public QWidget
{
    Q_OBJECT

public:
    explicit ColorLabelPage(QWidget* parent = nullptr);
    ~ColorLabelPage() override;

private Q_SLOTS:
    void onColorChanged(const AZ::Color& color) const;

private:
    QScopedPointer<Ui::ColorLabelPage> ui;

};
