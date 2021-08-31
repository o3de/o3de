/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMStudio/AnimViewportWidget.h>
#include <QWidget>
#endif

namespace EMStudio
{
    class AtomRenderPlugin
        : public DockWidgetPlugin
    {
    public:
        enum
        {
            CLASS_ID = 0x32b0c04d
        };
        AtomRenderPlugin();
        ~AtomRenderPlugin();

        // Plugin information
        const char* GetName() const override;
        uint32 GetClassID() const override;
        const char* GetCreatorName() const override;
        float GetVersion() const override;
        bool GetIsClosable() const override;
        bool GetIsFloatable() const override;
        bool GetIsVertical() const override;
        bool Init() override;
        EMStudioPlugin* Clone();
        EMStudioPlugin::EPluginType GetPluginType() const override;

    private:
        QWidget* m_innerWidget;
        AnimViewportWidget* m_animViewportWidget;
    };
}
