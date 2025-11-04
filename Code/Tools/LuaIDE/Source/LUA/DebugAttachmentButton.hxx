/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef INC_DEBUGATTACHMENTBUTTON_H
#define INC_DEBUGATTACHMENTBUTTON_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include "LUAContextControlMessages.h"
#include <QPushButton>
#include <QWidgetAction>
#endif

#pragma once

namespace LUAEditor
{
    enum DebugAttachmentState
    {
        DAS_UNATTACHED = 0,
        DAS_ATTACHED,
        DAS_REFUSED
    };

    class DebugAttachmentButton
        : public QPushButton
        , private Context_ControlManagement::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(DebugAttachmentButton, AZ::SystemAllocator);

        DebugAttachmentButton(QWidget *pParent = 0);
        virtual ~DebugAttachmentButton();

        // These come from the CONTEXT
        void OnDebuggerAttached();
        void OnDebuggerRefused();
        void OnDebuggerDetached();
        void OnTargetConnected(){}
        void OnTargetDisconnected(){}
        void OnTargetContextPrepared( AZStd::string &contextName ){(void)contextName;}
        virtual void paintEvent(QPaintEvent * /* event */);

    private:
        DebugAttachmentState m_State;

        void UpdateStatus( DebugAttachmentState newState );

        public slots:
            void OnClicked();
    };

    
    class DebugAttachmentButtonAction : public QWidgetAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(DebugAttachmentButtonAction,AZ::SystemAllocator,0);
        
        DebugAttachmentButtonAction(QObject *pParent);                                      // create default action

    protected:
        virtual QWidget* createWidget(QWidget* pParent);
    };
}

#endif //INC_DEBUGATTACHMENTBUTTON_H
