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

#ifndef DRILLER_GENERICCUSTOMIZECSVEXPORTPANEL_H
#define DRILLER_GENERICCUSTOMIZECSVEXPORTPANEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#pragma once

#include "Woodpecker/Driller/CustomizeCSVExportWidget.hxx"
#include "Woodpecker/Driller/CSVExportSettings.h"
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