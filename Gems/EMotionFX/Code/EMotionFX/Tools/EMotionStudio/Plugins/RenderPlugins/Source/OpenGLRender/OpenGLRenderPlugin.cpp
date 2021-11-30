/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "OpenGLRenderPlugin.h"
#include "GLWidget.h"
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/RenderPlugin/RenderViewWidget.h"

#include <chrono>

#include <EMotionFX/Source/BlendTreeSmoothingNode.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>

#include <AzCore/IO/Path/Path.h>

namespace EMStudio
{
    OpenGLRenderPlugin::OpenGLRenderPlugin()
        : EMStudio::RenderPlugin()
    {
        m_graphicsManager = nullptr;
    }

    OpenGLRenderPlugin::~OpenGLRenderPlugin()
    {
        // get rid of the OpenGL graphics manager
        delete m_graphicsManager;
    }

    // init after the parent dock window has been created
    bool OpenGLRenderPlugin::Init()
    {
        MCore::LogInfo("Initializing OpenGL rendering");

        RenderPlugin::Init();

        MCore::LogInfo("Render plugin initialized successfully");
        return true;
    }

    // initialize the OpenGL engine
    bool OpenGLRenderPlugin::InitializeGraphicsManager()
    {
        if (m_graphicsManager)
        {
            // initialize all already existing actors and actor instances
            ReInit();
            return true;
        }

        // get the absolute directory path where all the shaders will be located
        const auto shaderPath = AZ::IO::Path(MysticQt::GetDataDir()) / "Shaders";

        // create graphics manager and initialize it
        m_graphicsManager = new RenderGL::GraphicsManager();
        if (m_graphicsManager->Init(shaderPath) == false)
        {
            MCore::LogError("Could not initialize OpenGL graphics manager.");
            return false;
        }

        // set the render util in the base render plugin
        m_renderUtil = m_graphicsManager->GetRenderUtil();

        // initialize all already existing actors and actor instances
        ReInit();

        return true;
    }

    // overloaded version to create a OpenGL render actor
    bool OpenGLRenderPlugin::CreateEMStudioActor(EMotionFX::Actor* actor)
    {
        // check if we have already imported this actor
        if (FindEMStudioActor(actor))
        {
            MCore::LogError("The actor has already been imported.");
            return false;
        }

        // create a new OpenGL actor and try to initialize it
        RenderGL::GLActor* glActor = RenderGL::GLActor::Create();
        if (glActor->Init(actor, "", true, false) == false)
        {
            MCore::LogError("Initializing the OpenGL actor for '%s' failed.", actor->GetFileName());
            glActor->Destroy();
            return false;
        }

        // create the EMStudio OpenGL helper class, set the attributes and register it to the plugin
        OpenGLRenderPlugin::EMStudioRenderActor* emstudioActor = new EMStudioRenderActor(actor, glActor);

        // add the actor to the list and return success
        AddEMStudioActor(emstudioActor);
        return true;
    }

    // overloaded version to render a visible actor instance using OpenGL
    void OpenGLRenderPlugin::RenderActorInstance(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        // extract the RenderGL and EMStudio actors
        RenderGL::GLActor* renderActor = static_cast<RenderGL::GLActor*>(actorInstance->GetCustomData());
        if (renderActor == nullptr)
        {
            return;
        }

        // get the active widget & it's rendering options
        RenderViewWidget*   widget          = GetActiveViewWidget();
        RenderOptions*      renderOptions   = GetRenderOptions();

        // call the base class on render which renders everything else
        RenderPlugin::UpdateActorInstance(actorInstance, timePassedInSeconds);

        // Update the mesh deformers (perform cpu skinning and morphing) when needed.
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_AABB) ||
            widget->GetRenderFlag(RenderViewWidget::RENDER_COLLISIONMESHES) ||
            widget->GetRenderFlag(RenderViewWidget::RENDER_FACENORMALS) ||
            widget->GetRenderFlag(RenderViewWidget::RENDER_TANGENTS) ||
            widget->GetRenderFlag(RenderViewWidget::RENDER_VERTEXNORMALS) ||
            widget->GetRenderFlag(RenderViewWidget::RENDER_WIREFRAME) )
        {              
            actorInstance->UpdateMeshDeformers(timePassedInSeconds, true);  // Update ALL meshes, even with disabled deformers as we need that data.
        }
        else
        {
            actorInstance->UpdateMeshDeformers(timePassedInSeconds, false); // Only update cpu deformed meshes for morphing etc, the rest can be skipped (gpu skinned parts).
        }

        // solid mesh rendering
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_SOLID))
        {
            uint32 renderFlags = RenderGL::GLActor::RENDER_SKINNING;
            if (widget->GetRenderFlag(RenderViewWidget::RENDER_LIGHTING))
            {
                renderFlags |= RenderGL::GLActor::RENDER_LIGHTING;
                renderActor->SetGroundColor(renderOptions->GetLightGroundColor());
                renderActor->SetSkyColor(renderOptions->GetLightSkyColor());
            }
            if (widget->GetRenderFlag(RenderViewWidget::RENDER_SHADOWS))
            {
                renderFlags |= RenderGL::GLActor::RENDER_SHADOWS;
            }
            if (widget->GetRenderFlag(RenderViewWidget::RENDER_TEXTURING))
            {
                renderFlags |= RenderGL::GLActor::RENDER_TEXTURING;
            }

            renderActor->Render(actorInstance, renderFlags);
        }

        RenderPlugin::RenderActorInstance(actorInstance, timePassedInSeconds);
    }

    // overloaded version which created a OpenGL render widget
    void OpenGLRenderPlugin::CreateRenderWidget(RenderViewWidget* renderViewWidget, RenderWidget** outRenderWidget, QWidget** outWidget)
    {
        GLWidget* glWidget  = new GLWidget(renderViewWidget, this);
        *outRenderWidget    = glWidget;
        *outWidget          = glWidget;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/RenderPlugins/Source/OpenGLRender/moc_OpenGLRenderPlugin.cpp>
