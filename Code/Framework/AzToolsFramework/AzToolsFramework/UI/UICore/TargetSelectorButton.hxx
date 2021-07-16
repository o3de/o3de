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
#include "AzFramework/TargetManagement/TargetManagementAPI.h"
#include <QtWidgets/QPushButton>
#include <qwidgetaction.h>
#endif

#pragma once

namespace AzToolsFramework
{
    class TargetSelectorButton
        : public QPushButton
        , private AzFramework::TargetManagerClient::Bus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TargetSelectorButton, AZ::SystemAllocator, 0);

        TargetSelectorButton(QWidget* pParent = 0);
        virtual ~TargetSelectorButton();

        // implement AzFramework::TargetManagerClient::Bus::Handler
        void DesiredTargetConnected(bool connected);

    private:
        void UpdateStatus();
        void ConstructDisplayTargetString(QString& outputString, const AzFramework::TargetInfo& info);

    private slots:
        void DoPopup();
    };


    class TargetSelectorButtonAction
        : public QWidgetAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TargetSelectorButtonAction, AZ::SystemAllocator, 0);

        TargetSelectorButtonAction(QObject* pParent);                                     // create default action

    protected:
        virtual QWidget* createWidget(QWidget* pParent);
    };
}

#endif
