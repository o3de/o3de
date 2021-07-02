/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"
#include "PresetButton.h"
#include "PivotPresets.h"
#include "PivotPresetsWidget.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>

#include <QGridLayout>

#define UICANVASEDITOR_PIVOT_ICON_NAME_DEFAULT  ":/Icons/PivotIconDefault.tif"
#define UICANVASEDITOR_PIVOT_ICON_NAME_HOVER    ":/Icons/PivotIconHover.tif"
#define UICANVASEDITOR_PIVOT_ICON_NAME_SELECTED ":/Icons/PivotIconSelected.tif"

#define UICANVASEDITOR_PIVOT_WIDGET_FIXED_SIZE              (52)
#define UICANVASEDITOR_PIVOT_BUTTON_AND_ICON_FIXED_SIZE     (12)

PivotPresetsWidget::PivotPresetsWidget(int defaultPresetIndex,
    PresetChanger presetChanger,
    QWidget* parent)
    : QWidget(parent)
    , m_presetIndex(defaultPresetIndex)
    , m_buttons(PivotPresets::PresetIndexCount, nullptr)
{
    setFixedSize(UICANVASEDITOR_PIVOT_WIDGET_FIXED_SIZE, UICANVASEDITOR_PIVOT_WIDGET_FIXED_SIZE);

    // The layout.
    QGridLayout* grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);

    // Preset buttons.
    {
        for (int presetIndex = 0; presetIndex < PivotPresets::PresetIndexCount; ++presetIndex)
        {
            PresetButton* button = new PresetButton(UICANVASEDITOR_PIVOT_ICON_NAME_DEFAULT,
                    UICANVASEDITOR_PIVOT_ICON_NAME_HOVER,
                    UICANVASEDITOR_PIVOT_ICON_NAME_SELECTED,
                    QSize(UICANVASEDITOR_PIVOT_BUTTON_AND_ICON_FIXED_SIZE, UICANVASEDITOR_PIVOT_BUTTON_AND_ICON_FIXED_SIZE),
                    "",
                    [ this, presetChanger, presetIndex ]([[maybe_unused]] bool checked)
                    {
                        SetPresetSelection(presetIndex);

                        presetChanger(presetIndex);
                    },
                    this);

            grid->addWidget(button, (presetIndex / 3), (presetIndex % 3));

            m_buttons[ presetIndex ] = button;
        }
    }
}

void PivotPresetsWidget::SetPresetSelection(int presetIndex)
{
    if (m_presetIndex != -1)
    {
        // Clear the old selection.
        m_buttons[ m_presetIndex ]->setChecked(false);
    }

    if (presetIndex != -1)
    {
        // Set the new selection.
        m_buttons[ presetIndex ]->setChecked(true);
    }

    m_presetIndex = presetIndex;
}

#include <moc_PivotPresetsWidget.cpp>
