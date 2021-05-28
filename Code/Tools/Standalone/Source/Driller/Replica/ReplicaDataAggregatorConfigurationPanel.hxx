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
