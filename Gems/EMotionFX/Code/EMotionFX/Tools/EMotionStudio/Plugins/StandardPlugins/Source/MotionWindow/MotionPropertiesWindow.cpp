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

// include required headers
#include "MotionPropertiesWindow.h"
#include "MotionWindowPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>

namespace EMStudio
{
    // constructor
    MotionPropertiesWindow::MotionPropertiesWindow(QWidget* parent, [[maybe_unused]] MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
    {
    }


    // destructor
    MotionPropertiesWindow::~MotionPropertiesWindow()
    {
    }


    // init after the parent dock window has been created
    void MotionPropertiesWindow::Init()
    {
        // motion properties
        QVBoxLayout* motionPropertiesLayout = new QVBoxLayout(this);
        motionPropertiesLayout->setMargin(0);
        motionPropertiesLayout->setSpacing(0);
    }

    void MotionPropertiesWindow::AddSubProperties(QWidget* widget)
    {
        layout()->addWidget(widget);
    }

    void MotionPropertiesWindow::FinalizeSubProperties()
    {
        layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/moc_MotionPropertiesWindow.cpp>
