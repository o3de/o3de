/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "Source/Driller/CustomizeCSVExportWidget.hxx"
#include <Source/Driller/moc_CustomizeCSVExportWidget.cpp>

#include "Source/Driller/CSVExportSettings.h"

namespace Driller
{
    /////////////////////////////
    // CustomizeCSVExportWidget
    /////////////////////////////

    CustomizeCSVExportWidget::CustomizeCSVExportWidget(CSVExportSettings& exportSettings, QWidget* parent)
        : QWidget(parent)
        , m_exportSettings(exportSettings)
    {
    }

    CustomizeCSVExportWidget::~CustomizeCSVExportWidget()
    {
    }

    CSVExportSettings* CustomizeCSVExportWidget::GetExportSettings() const
    {
        return (&m_exportSettings);
    }

    void CustomizeCSVExportWidget::OnShouldExportStateDescriptorChecked(int state)
    {
        m_exportSettings.SetShouldExportColumnDescriptors(state == Qt::Checked);
    }
}
