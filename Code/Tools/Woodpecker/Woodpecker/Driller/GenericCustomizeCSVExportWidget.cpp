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

#include "Woodpecker/Driller/GenericCustomizeCSVExportWidget.hxx"
#include <Woodpecker/Driller/ui_GenericCustomizeCSVExportWidget.h>
#include <Woodpecker/Driller/moc_GenericCustomizeCSVExportWidget.cpp>

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
