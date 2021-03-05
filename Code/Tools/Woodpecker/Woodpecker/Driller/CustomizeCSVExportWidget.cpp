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

#include "Woodpecker_precompiled.h"

#include "Woodpecker/Driller/CustomizeCSVExportWidget.hxx"
#include <Woodpecker/Driller/moc_CustomizeCSVExportWidget.cpp>

#include "Woodpecker/Driller/CSVExportSettings.h"

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
