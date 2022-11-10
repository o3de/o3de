/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>
#include <AzQtComponents/Components/Widgets/ComboBox.h>

#include <QComboBox>
#include <QLabel>
#include <QWidget>

// Widget to select the Prefab Edit mode visualization.
class PrefabEditVisualModeWidget
    : public QWidget
{
public:
    PrefabEditVisualModeWidget();
    ~PrefabEditVisualModeWidget();

private:
    void OnComboBoxValueChanged(int index);

    void UpdatePrefabEditMode();

    //! The different prefab edit mode effects available in the Edit mode menu.
    //! These correspond to the indices in the ComboBox to streamline code.
    enum class PrefabEditModeUXSetting
    {
        Normal,         //!< No effect.
        Monochromatic   //!< Monochromatic effect.
    };

    //! The currently active edit mode effect.
    PrefabEditModeUXSetting m_prefabEditMode = PrefabEditModeUXSetting::Monochromatic;

    QLabel* m_label = nullptr;
    QComboBox* m_comboBox = nullptr;
};
