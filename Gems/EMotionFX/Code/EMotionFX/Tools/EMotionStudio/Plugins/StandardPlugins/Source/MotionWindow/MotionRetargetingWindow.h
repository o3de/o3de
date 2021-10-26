/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        MotionWindowPlugin*                 m_motionWindowPlugin;
        QCheckBox*                          m_motionRetargetingButton;
        CommandSystem::SelectionList        m_selectionList;
    };
} // namespace EMStudio


#endif
