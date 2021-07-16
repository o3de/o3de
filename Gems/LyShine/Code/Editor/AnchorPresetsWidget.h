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
#endif

class PresetButton;

class AnchorPresetsWidget
    : public QWidget
{
    Q_OBJECT

public:

    using PresetChanger = std::function<void(int presetIndex)>;

    AnchorPresetsWidget(int defaultPresetIndex,
        PresetChanger presetChanger,
        QWidget* parent = nullptr);

    void SetPresetSelection(int presetIndex);

    void SetPresetButtonEnabledAt(int presetIndex, bool enabled);

private:

    int m_presetIndex;

    std::vector<PresetButton*> m_buttons;
};
