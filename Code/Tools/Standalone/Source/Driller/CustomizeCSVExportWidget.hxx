/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
