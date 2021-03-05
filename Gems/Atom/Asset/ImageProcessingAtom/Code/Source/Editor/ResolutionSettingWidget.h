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
#include <AzCore/Memory/SystemAllocator.h>
#include <Source/Editor/ResolutionSettingItemWidget.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWidget>
#include <QListWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace Ui
{
    class ResolutionSettingWidget;
}

namespace ImageProcessingAtomEditor
{
    class ResolutionSettingWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ResolutionSettingWidget, AZ::SystemAllocator, 0);
        explicit ResolutionSettingWidget(ResoultionWidgetType type, EditorTextureSetting& texureSetting, QWidget* parent = nullptr);
        ~ResolutionSettingWidget();

    private:
        QScopedPointer<Ui::ResolutionSettingWidget> m_ui;
        ResoultionWidgetType m_type;
        EditorTextureSetting* m_textureSetting;
    };
} //namespace ImageProcessingAtomEditor

