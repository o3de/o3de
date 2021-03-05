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

#ifndef INC_TARGETCONTEXTBUTTON_H
#define INC_TARGETCONTEXTBUTTON_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Woodpecker/LUA/LUAContextControlMessages.h>
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
        AZ_CLASS_ALLOCATOR(TargetContextButton, AZ::SystemAllocator, 0);

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
