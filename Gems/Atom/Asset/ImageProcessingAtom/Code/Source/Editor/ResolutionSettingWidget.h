/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(ResolutionSettingWidget, AZ::SystemAllocator);
        explicit ResolutionSettingWidget(ResoultionWidgetType type, EditorTextureSetting& texureSetting, QWidget* parent = nullptr);
        ~ResolutionSettingWidget();

    private:
        QScopedPointer<Ui::ResolutionSettingWidget> m_ui;
        ResoultionWidgetType m_type;
        EditorTextureSetting* m_textureSetting;
    };
} //namespace ImageProcessingAtomEditor

