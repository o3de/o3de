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

#ifndef __EMSTUDIO_MOTIONPROPERTIESWINDOW_H
#define __EMSTUDIO_MOTIONPROPERTIESWINDOW_H

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <QWidget>
#include <QLabel>
#endif


QT_FORWARD_DECLARE_CLASS(QPushButton)


namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;

    class MotionPropertiesWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionPropertiesWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000005
        };

        MotionPropertiesWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionPropertiesWindow();

        void Init();

        void AddSubProperties(QWidget* widget);
        void FinalizeSubProperties();
    };
} // namespace EMStudio


#endif
