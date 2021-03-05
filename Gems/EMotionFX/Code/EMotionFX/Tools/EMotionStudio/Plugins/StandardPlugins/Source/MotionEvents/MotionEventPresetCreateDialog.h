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

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <QDialog>
#include <EMotionStudio/EMStudioSDK/Source/MotionEventPresetManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionEvents/EventDataEditor.h>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)


namespace EMotionFX
{
    class ObjectEditor;
}

namespace EMStudio
{
    class MotionEventPresetCreateDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        AZ_TYPE_INFO(EMStudio::MotionEventPresetCreateDialog, "{644087A8-D442-4A48-AF04-8DD34D9DF4D7}");

        MotionEventPresetCreateDialog(const MotionEventPreset& preset = MotionEventPreset(), QWidget* parent = nullptr);

        MotionEventPreset& GetPreset();

    private slots:
        void OnCreateButton();

    private:
        void Init();

        MotionEventPreset m_preset;
        EMotionFX::ObjectEditor* m_editor;
        EMStudio::EventDataEditor m_eventDataEditor;
    };
} // namespace EMStudio
