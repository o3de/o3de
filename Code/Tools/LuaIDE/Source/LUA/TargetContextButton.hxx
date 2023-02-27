/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef INC_TARGETCONTEXTBUTTON_H
#define INC_TARGETCONTEXTBUTTON_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Source/LUA/LUAContextControlMessages.h>
#include <QPushButton>
#include <QWidgetAction>
#endif

#pragma once

namespace LUA
{
    class TargetContextButton 
        : public QPushButton
        , private LUAEditor::Context_ControlManagement::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TargetContextButton, AZ::SystemAllocator);

        TargetContextButton(QWidget *pParent = 0);
        virtual ~TargetContextButton();

        // These come from the CONTEXT
        void OnDebuggerAttached(){}
        void OnDebuggerRefused(){}
        void OnDebuggerDetached(){}
        void OnTargetConnected(){}
        void OnTargetDisconnected(){}
        void OnTargetContextPrepared( AZStd::string &contextName );

    private slots:
        void DoPopup();
    };

    
    class TargetContextButtonAction : public QWidgetAction
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(TargetContextButtonAction,AZ::SystemAllocator,0);
        
        TargetContextButtonAction(QObject *pParent);                                      // create default action

    protected:
        virtual QWidget* createWidget(QWidget* pParent);
    };
}

#endif //INC_TARGETCONTEXTBUTTON_H
