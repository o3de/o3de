/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/Command.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/RenderPlugin/RenderOptions.h>

#include <EMStudio/AnimViewportWidget.h>
#include <QWidget>
#endif

namespace AZ
{
    class Entity;
}

namespace EMStudio
{
    class AtomRenderPlugin
        : public DockWidgetPlugin
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

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
        QWidget* GetInnerWidget();

        void ReinitRenderer();

        void LoadRenderOptions();
        const RenderOptions* GetRenderOptions() const;

    private:

        QWidget* m_innerWidget = nullptr;
        AnimViewportWidget* m_animViewportWidget = nullptr;
        RenderOptions m_renderOptions;

        MCORE_DEFINECOMMANDCALLBACK(ImportActorCallback);
        MCORE_DEFINECOMMANDCALLBACK(RemoveActorCallback);
        ImportActorCallback* m_importActorCallback = nullptr;
        RemoveActorCallback* m_removeActorCallback = nullptr;
    };
}// namespace EMStudio
