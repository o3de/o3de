/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef INC_TARGETSELECTORBUTTON_H
#define INC_TARGETSELECTORBUTTON_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <QtWidgets/QPushButton>
#include <qwidgetaction.h>
#endif

#pragma once

namespace AzToolsFramework
{
    class TargetSelectorButton
        : public QPushButton
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TargetSelectorButton, AZ::SystemAllocator);

        TargetSelectorButton(AZ::Crc32 key, QWidget* pParent = 0);
        virtual ~TargetSelectorButton() = default;

        // implement AzFramework::TargetManagerClient::Bus::Handler
        void DesiredTargetConnected(bool connected);

    private:
        void UpdateStatus();
        void ConstructDisplayTargetString(QString& outputString, const AzFramework::RemoteToolsEndpointInfo& info);

        AZ::Crc32 m_remoteToolsKey;
        AzFramework::RemoteToolsEndpointConnectedEvent::Handler m_connectedEventHandler;

    private slots:
        void DoPopup();
    };


    class TargetSelectorButtonAction
        : public QWidgetAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TargetSelectorButtonAction, AZ::SystemAllocator);

        TargetSelectorButtonAction(AZ::Crc32 key, QObject* pParent); // create default action

    protected:
        virtual QWidget* createWidget(QWidget* pParent);

    private:
        AZ::Crc32 m_remoteToolsKey;
    };
}

#endif
