/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <QWidget>
#endif

class PresetButton;

class PivotPresetsWidget : public QWidget
{
    Q_OBJECT

public:
    using PresetChanger = AZStd::function<void(int presetIndex)>;

    PivotPresetsWidget(int defaultPresetIndex, PresetChanger presetChanger, QWidget* parent = nullptr);

    void SetPresetSelection(int presetIndex);

private:
    int m_presetIndex;
    AZStd::vector<PresetButton*> m_buttons;
};
