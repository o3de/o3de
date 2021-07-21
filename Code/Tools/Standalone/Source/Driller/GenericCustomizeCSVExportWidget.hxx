/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef DRILLER_GENERICCUSTOMIZECSVEXPORTPANEL_H
#define DRILLER_GENERICCUSTOMIZECSVEXPORTPANEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#pragma once

#include "Source/Driller/CustomizeCSVExportWidget.hxx"
#include "Source/Driller/CSVExportSettings.h"
#endif

namespace Ui
{
    class GenericCustomizeCSVExportWidget;
}

namespace Driller
{
    class GenericCSVExportSettings;

    class GenericCustomizeCSVExportWidget
        : public CustomizeCSVExportWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GenericCustomizeCSVExportWidget, AZ::SystemAllocator, 0);
        
        GenericCustomizeCSVExportWidget(GenericCSVExportSettings& exportSettings, QWidget* parent = nullptr);
        ~GenericCustomizeCSVExportWidget();

        void FinalizeSettings() override;
        
    public slots:
    
        void OnActiveItemsChanged();

    private:
        
        bool m_exportFieldsDirty;
        
        Ui::GenericCustomizeCSVExportWidget* m_gui;
    };

    class GenericCSVExportSettings
        : public CSVExportSettings
    {
        friend class GenericCustomizeCSVExportWidget;
    public:
        GenericCSVExportSettings()
        {        
        }

        virtual void GetExportItems(QStringList& items) const = 0;
        virtual void GetActiveExportItems(QStringList& items) const = 0;

    protected:
        virtual void UpdateExportOrdering(const QStringList& items) = 0;
    };
}

#endif
