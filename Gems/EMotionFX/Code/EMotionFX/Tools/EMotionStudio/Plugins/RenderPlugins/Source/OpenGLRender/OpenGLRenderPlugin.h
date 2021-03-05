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
#include <EMotionFX/Rendering/OpenGL2/Source/glactor.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GraphicsManager.h>
#include <EMotionFX/Rendering/OpenGL2/Source/GLRenderUtil.h>
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderPlugin.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderOptions.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderWidget.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderLayouts.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderUpdateCallback.h"
#include "GLWidget.h"
#endif

namespace EMStudio
{
    class OpenGLRenderPlugin
        : public EMStudio::RenderPlugin
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(OpenGLRenderPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERPLUGIN);

    public:
        OpenGLRenderPlugin();
        ~OpenGLRenderPlugin();

        // plugin information
        const char* GetCompileDate() const override         { return MCORE_DATE; }
        const char* GetName() const override                { return "OpenGL Render Window"; }
        uint32 GetClassID() const override                  { return static_cast<uint32>(RenderPlugin::CLASS_ID); }
        const char* GetCreatorName() const override         { return "Amazon"; }
        float GetVersion() const override                   { return 1.0f;  }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        bool Init();
        EMStudioPlugin* Clone()                             { return new OpenGLRenderPlugin(); }

        // overloaded functions
        void CreateRenderWidget(RenderViewWidget* renderViewWidget, RenderWidget** outRenderWidget, QWidget** outWidget) override;

        // OpenGL engine helper functions
        bool InitializeGraphicsManager();
        MCORE_INLINE RenderGL::GraphicsManager* GetGraphicsManager()                { return mGraphicsManager; }

    private:
        RenderGL::GraphicsManager*          mGraphicsManager;           // shared OpenGL engine object

        // overloaded emstudio actor create function which creates an OpenGL render actor internally
        bool CreateEMStudioActor(EMotionFX::Actor* actor);

        void RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds) override;
    };
} // namespace EMStudio
