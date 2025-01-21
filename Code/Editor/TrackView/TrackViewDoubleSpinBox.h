/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#endif

class CTrackViewDoubleSpinBox
    : public AzQtComponents::DoubleSpinBox
{
    Q_OBJECT

public:
    CTrackViewDoubleSpinBox(QWidget* parent = nullptr);
    ~CTrackViewDoubleSpinBox() override;

protected:
    void stepBy(int steps) override;

signals:
    void stepByFinished();
};
