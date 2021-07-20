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

namespace Ui {
    class SliderComboPage;
}

class SliderComboPage
    : public QWidget
{
    Q_OBJECT
public:
    explicit SliderComboPage(QWidget* parent = nullptr);
    ~SliderComboPage() override;

    void sliderValueChanged();

private:
    QScopedPointer<Ui::SliderComboPage> ui;
};


