/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/MemoryFile.h>
#include <EMotionFX/Rendering/Common/RenderUtil.h>
#include <EMotionFX/Rendering/Common/Camera.h>
#include <Integration/Rendering/RenderFlag.h>
#include "EMStudioConfig.h"
#include <QString>
#include <QObject>
#include <QFile>
#endif

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace AZ
{
    class ReflectContext;
}

namespace EMStudio
{
    // forward declaration
    class PluginOptions;
    class PreferencesWindow;
    class RenderPlugin;

    class EMSTUDIO_API EMStudioPlugin
        : public QObject
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(EMStudioPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        enum EPluginType
        {
            PLUGINTYPE_WINDOW = 0,
            PLUGINTYPE_TOOLBAR = 1,
            PLUGINTYPE_RENDERING = 2
        };

        EMStudioPlugin()
            : QObject() {}
        virtual ~EMStudioPlugin() = default;

        virtual const char* GetName() const = 0;
        virtual uint32 GetClassID() const = 0;
        virtual void Reflect(AZ::ReflectContext*) {}
        virtual bool Init() = 0;
        virtual EMStudioPlugin* Clone() const = 0;
        virtual EMStudioPlugin::EPluginType GetPluginType() const = 0;

        virtual void OnAfterLoadLayout() {}
        virtual void OnAfterLoadProject() {}
        virtual void OnAfterLoadActors() {}
        virtual void OnBeforeRemovePlugin(uint32 classID) { MCORE_UNUSED(classID); }
        virtual void OnMainWindowClosed() {}

        struct RenderInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(EMStudioPlugin::RenderInfo, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

            RenderInfo(MCommon::RenderUtil* renderUtil, MCommon::Camera* camera, uint32 screenWidth, uint32 screenHeight)
            {
                m_renderUtil     = renderUtil;
                m_camera         = camera;
                m_screenWidth    = screenWidth;
                m_screenHeight   = screenHeight;
            }

            MCommon::RenderUtil*        m_renderUtil;
            MCommon::Camera*            m_camera;
            uint32                      m_screenWidth;
            uint32                      m_screenHeight;
        };

        //! Deprecated: LegacyRender will call EMotionFX::DebugDraw that tied to OpenGL render.
        //! It will be removed after OpenGLPlugin and GLWidget is gone.
        virtual void LegacyRender(RenderPlugin* renderPlugin, RenderInfo* renderInfo)             { MCORE_UNUSED(renderPlugin); MCORE_UNUSED(renderInfo); }

        //! Render function will call atom auxGeom internally to render. This is the replacement for LegacyRender function.
        virtual void Render(EMotionFX::ActorRenderFlags renderFlags)
        {
            AZ_UNUSED(renderFlags);
        };

        virtual PluginOptions* GetOptions() { return nullptr; }

        virtual void WriteLayoutData(MCore::MemoryFile& outFile)                            { MCORE_UNUSED(outFile); }
        virtual bool ReadLayoutSettings(QFile& file, uint32 dataSize, uint32 dataVersion)   { MCORE_UNUSED(file); MCORE_UNUSED(dataSize); MCORE_UNUSED(dataVersion); return true; }
        virtual uint32 GetLayoutDataVersion() const                                         { return 0; }

        virtual void ProcessFrame(float timePassedInSeconds)                                { MCORE_UNUSED(timePassedInSeconds); }
        virtual uint32 GetProcessFramePriority() const                                      { return 0; }

        bool operator<(const EMStudioPlugin& plugin)                                        { return GetProcessFramePriority() < plugin.GetProcessFramePriority(); }
        bool operator>(const EMStudioPlugin& plugin)                                        { return GetProcessFramePriority() > plugin.GetProcessFramePriority(); }

        virtual bool GetHasWindowWithObjectName(const AZStd::string& objectName) = 0;

        virtual QString GetObjectName() const = 0;
        virtual void SetObjectName(const QString& objectName) = 0;

        virtual void CreateBaseInterface(const char* objectName) = 0;

        virtual bool AllowMultipleInstances() const     { return false; }

        virtual void AddWindowMenuEntries([[maybe_unused]] QMenu* parent) { }
    };
} // namespace EMStudio
