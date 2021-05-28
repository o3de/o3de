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
#ifndef PROFILER_CHANNELCONFIGURATIONDIALOG_H
#define PROFILER_CHANNELCONFIGURATIONDIALOG_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QDialog>
#endif

namespace Driller
{
    class ChannelConfigurationDialog
        : public QDialog
    {
        Q_OBJECT;

    public:        
        AZ_CLASS_ALLOCATOR(ChannelConfigurationDialog,AZ::SystemAllocator,0);

        ChannelConfigurationDialog(QWidget* parent = nullptr);
        ~ChannelConfigurationDialog();        
        
    signals:
    
        void DialogClosed(QDialog* dialog);
    };
}

#endif
