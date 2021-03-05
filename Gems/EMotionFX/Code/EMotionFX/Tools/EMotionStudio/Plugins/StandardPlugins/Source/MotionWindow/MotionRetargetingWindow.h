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

#ifndef __EMSTUDIO_MOTIONRETARGETINGWINDOW_H
#define __EMSTUDIO_MOTIONRETARGETINGWINDOW_H

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;

    class MotionRetargetingWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionRetargetingWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionRetargetingWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionRetargetingWindow();

        void Init();

    public slots:
        void UpdateInterface();
        void UpdateMotions();

    private:
        MotionWindowPlugin*                 mMotionWindowPlugin;
        QCheckBox*                          mMotionRetargetingButton;
        //QCheckBox*                          mRenderMotionBindPose;
        EMotionFX::ActorInstance*           mSelectedActorInstance;
        EMotionFX::Actor*                   mActor;
        CommandSystem::SelectionList        mSelectionList;
    };
} // namespace EMStudio


#endif
