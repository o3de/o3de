/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "Source/Driller/GenericCustomizeCSVExportWidget.hxx"
#include <Source/Driller/ui_GenericCustomizeCSVExportWidget.h>
#include <Source/Driller/moc_GenericCustomizeCSVExportWidget.cpp>

namespace Driller
{
    ////////////////////////////////////
    // GenericCustomizeCSVExportWidget
    ////////////////////////////////////

    GenericCustomizeCSVExportWidget::GenericCustomizeCSVExportWidget(GenericCSVExportSettings& genericSettings, QWidget* parent)
        : CustomizeCSVExportWidget(genericSettings, parent)
        , m_exportFieldsDirty(false)
        , m_gui(nullptr)
    {
        m_gui = azcreate(Ui::GenericCustomizeCSVExportWidget, ());
        m_gui->setupUi(this);

        QStringList items;
        genericSettings.GetExportItems(items);
        m_gui->exportFieldSelector->setItemList(items);

        items.clear();

        genericSettings.GetActiveExportItems(items);
        m_gui->exportFieldSelector->setActiveItems(items);
        items.clear();

        m_gui->exportFieldSelector->setActiveTitle("Exported Fields");
        m_gui->exportFieldSelector->setInactiveTitle("Unused Fields");

        QObject::connect(m_gui->exportFieldSelector, SIGNAL(ActiveItemsChanged()), this, SLOT(OnActiveItemsChanged()));
        QObject::connect(m_gui->addDescriptor, SIGNAL(stateChanged(int)), this, SLOT(OnShouldExportStateDescriptorChecked(int)));
    }

    GenericCustomizeCSVExportWidget::~GenericCustomizeCSVExportWidget()
    {
        azdestroy(m_gui);
    }

    void GenericCustomizeCSVExportWidget::FinalizeSettings()
    {
        if (m_exportFieldsDirty)
        {
            m_exportFieldsDirty = false;

            GenericCSVExportSettings& exportSettings = static_cast<GenericCSVExportSettings&>(m_exportSettings);
            const QStringList& activeItems = m_gui->exportFieldSelector->getActiveItems();

            exportSettings.UpdateExportOrdering(activeItems);
        }
    }

    void GenericCustomizeCSVExportWidget::OnActiveItemsChanged()
    {
        m_exportFieldsDirty = true;
    }
}
