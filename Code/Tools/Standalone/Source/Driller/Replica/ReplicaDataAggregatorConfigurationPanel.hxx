/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#ifndef DRILLER_REPLICA_REPLICADATAAGGREGATORCONFIGURATIONPANEL_H
#define DRILLER_REPLICA_REPLICADATAAGGREGATORCONFIGURATIONPANEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QWidget>
#include <QTimer>

// Generated File
#include <Source/Driller/Replica/ui_ReplicaDataAggregatorConfigurationPanel.h>

#include <Source/Driller/ChannelConfigurationWidget.hxx>
#include <Source/Driller/Replica/ReplicaDataAggregator.hxx>
#endif

namespace Driller
{
    class ReplicaDataAggregatorConfigurationPanel
        : public ChannelConfigurationWidget
        , private Ui::ReplicaDataAggregatorConfigurationPanel
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ReplicaDataAggregatorConfigurationPanel, AZ::SystemAllocator, 0);

        ReplicaDataAggregatorConfigurationPanel(ReplicaDataConfigurationSettings& configurationSettings);
        ~ReplicaDataAggregatorConfigurationPanel();        

    public slots:
        void OnBudgetChanged(int budget);        
        void OnTypeChanged(int type);
        void OnFPSChanged(int fps);

        void OnTimeout();

    private:

        void InitUI();        

        void DisplayTypeDescriptor();
        void UpdateBudgetDisplay();

        ReplicaDataConfigurationSettings& m_configurationSettings;
        QTimer m_changeTimer;
    };
}

#endif
