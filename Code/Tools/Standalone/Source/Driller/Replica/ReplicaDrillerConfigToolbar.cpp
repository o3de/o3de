/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"

#include "ReplicaDrillerConfigToolbar.hxx"
#include <Source/Driller/Replica/moc_ReplicaDrillerConfigToolbar.cpp>
#include <Source/Driller/Replica/ui_ReplicaDrillerConfigToolbar.h>

namespace Driller
{
    ReplicaDrillerConfigToolbar::ReplicaDrillerConfigToolbar(QWidget* parent)
        : QWidget(parent)
        , m_gui(new Ui::ReplicaDrillerConfigToolbar)
    {
        m_gui->setupUi(this);

        m_gui->hideAll->setAutoDefault(false);
        m_gui->hideSelected->setAutoDefault(false);
        m_gui->showAll->setAutoDefault(false);
        m_gui->showSelected->setAutoDefault(false);
        m_gui->collapseAll->setAutoDefault(false);
        m_gui->expandAll->setAutoDefault(false);

        QObject::connect(m_gui->hideAll, SIGNAL(clicked()), this, SIGNAL(hideAll()));
        QObject::connect(m_gui->hideSelected, SIGNAL(clicked()), this, SIGNAL(hideSelected()));
        QObject::connect(m_gui->showAll, SIGNAL(clicked()), this, SIGNAL(showAll()));
        QObject::connect(m_gui->showSelected, SIGNAL(clicked()), this, SIGNAL(showSelected()));
        QObject::connect(m_gui->collapseAll, SIGNAL(clicked()), this, SIGNAL(collapseAll()));
        QObject::connect(m_gui->expandAll, SIGNAL(clicked()), this, SIGNAL(expandAll()));        
    }

    ReplicaDrillerConfigToolbar::~ReplicaDrillerConfigToolbar()
    {
        delete m_gui;
    }    

    void ReplicaDrillerConfigToolbar::enableTreeCommands(bool enabled)
    {
        m_gui->collapseAll->setVisible(enabled);
        m_gui->expandAll->setVisible(enabled);
    }    
}
