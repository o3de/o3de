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

#ifndef DRILLER_CUSTOMIZECSVEXPORTWIDGET_H
#define DRILLER_CUSTOMIZECSVEXPORTWIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

#include <QtWidgets/QWidget>
#endif

namespace Driller
{
    class CSVExportSettings;

    class CustomizeCSVExportWidget : public QWidget
    {
        Q_OBJECT
    public:
        CustomizeCSVExportWidget(CSVExportSettings& customSettings, QWidget* parent);
        virtual ~CustomizeCSVExportWidget();

        virtual void FinalizeSettings() = 0;
        CSVExportSettings* GetExportSettings() const;

    public slots:
        void OnShouldExportStateDescriptorChecked(int);

    protected:
        CSVExportSettings& m_exportSettings;
    };
}

#endif