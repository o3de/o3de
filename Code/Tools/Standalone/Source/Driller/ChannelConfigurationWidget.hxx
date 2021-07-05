/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#ifndef PROFILER_CHANNELCONFIGURATIONWIDGET_H
#define PROFILER_CHANNELCONFIGURATIONWIDGET_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QWidget>
#endif

namespace Driller
{
    class ChannelConfigurationWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ChannelConfigurationWidget, AZ::SystemAllocator,0);
        
        ChannelConfigurationWidget(QWidget* parent = nullptr);
        ~ChannelConfigurationWidget() = default;
        
    public slots:
    signals:
        void ConfigurationChanged();
    };
}

#endif
