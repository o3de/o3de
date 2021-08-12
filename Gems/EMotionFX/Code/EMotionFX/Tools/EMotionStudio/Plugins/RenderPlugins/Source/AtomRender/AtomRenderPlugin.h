/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionStudio/Plugins/RenderPlugins/Source/AtomRender/AnimViewportWidget.h>
#include <QWidget>
#endif

namespace EMStudio
{
    class AtomRenderPlugin
        : public DockWidgetPlugin
    {
        Q_OBJECT // AUTOMOC

    public:
        enum
        {
            CLASS_ID = 0x32b0c04d
        };
        AtomRenderPlugin();
        ~AtomRenderPlugin();

        // Plugin information
        const char* GetCompileDate() const override
        {
            return MCORE_DATE;
        }
        const char* GetName() const override
        {
            return "Atom Render Window";
        }
        uint32 GetClassID() const override
        {
            return static_cast<uint32>(AtomRenderPlugin::CLASS_ID);
        }
        const char* GetCreatorName() const override
        {
            return "O3DE";
        }
        float GetVersion() const override
        {
            return 1.0f;
        }
        bool GetIsClosable() const override
        {
            return true;
        }
        bool GetIsFloatable() const override
        {
            return true;
        }
        bool GetIsVertical() const override
        {
            return false;
        }

        bool Init() override;
        EMStudioPlugin* Clone()
        {
            return new AtomRenderPlugin();
        }
        EMStudioPlugin::EPluginType GetPluginType() const override
        {
            return EMStudioPlugin::PLUGINTYPE_RENDERING;
        }

    private:
        QWidget* m_innerWidget;
        AnimViewportWidget* m_animViewportWidget;
    };
}
