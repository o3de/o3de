/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
