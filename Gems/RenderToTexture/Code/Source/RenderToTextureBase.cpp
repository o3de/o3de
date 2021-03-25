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

#include "RenderToTexture_precompiled.h"

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include "RenderToTextureBase.h"
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Components/CameraBus.h>
#include <MathConversion.h>
#include <IRenderer.h>
#include <I3DEngine.h>
#include <IViewSystem.h>
#include <RTTBus.h>

namespace RenderToTexture
{
    void RenderToTextureBase::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RenderToTextureBase>()
                ->Version(1);
        }
    }

    void RenderToTextureBase::DisplayDebugImage(const RenderToTextureConfig& config) const
    {
        if (config.m_renderContextId.IsNull())
        {
            return;
        }

        if (config.m_renderContextConfig.m_width < AzRTT::MinRenderTargetWidth
            || config.m_renderContextConfig.m_height < AzRTT::MinRenderTargetHeight)
        {
            return;
        }

        if (config.m_renderContextConfig.m_alphaMode == AzRTT::AlphaMode::ALPHA_DEPTH_BASED)
        {
            // use alpha from rtt image
            gEnv->pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
        }
        else
        {
            // ignore alpha
            gEnv->pRenderer->SetState(GS_BLSRC_ONE | GS_BLDST_ZERO | GS_NODEPTHTEST);
        }

        const float width = (float)config.m_renderContextConfig.m_width;
        const float height = (float)config.m_renderContextConfig.m_height;
        const float viewportScaleX = (800.0f / (float)gEnv->pRenderer->GetWidth());
        const float viewportScaleY = (600.0f / (float)gEnv->pRenderer->GetHeight());

        gEnv->pRenderer->Draw2dImage(
            0.f,
            0.f,
            (float)width * viewportScaleX,
            (float)height * viewportScaleY,
            m_renderTargetHandle,
            0.f, 1.f, 1.f, 0.f, // texture coordinates
            0.f, // angle
            1.f, 1.f, 1.f, 1.f // rgba
        );
    }

    void RenderToTextureBase::Render(int renderTargetHandle, const RenderToTextureConfig& config, const AZ::EntityId& entityId)
    {
        if (config.m_renderContextId.IsNull())
        {
            AZ_Printf("RenderToTextureComponent", "$2Invalid render context");
            return;
        }

        if (config.m_renderContextConfig.m_width < AzRTT::MinRenderTargetWidth
            || config.m_renderContextConfig.m_height < AzRTT::MinRenderTargetHeight)
        {
            AZ_Printf("RenderToTextureComponent", "$2Invalid render target width or height");
            return;
        }

        if (renderTargetHandle <= 0)
        {
            AZ_Printf("RenderToTextureComponent", "$2Invalid render target handle");
            return;
        }

        double maxFPS = config.m_maxFPS;

#ifndef _RELEASE
        // allow overriding the fps limit
        static ICVar* rttMaxFPS = gEnv->pConsole->GetCVar("rtt_maxfps");
        if (rttMaxFPS && rttMaxFPS->GetFVal() >= 0.f)
        {
            maxFPS = rttMaxFPS->GetFVal();
        }
#endif // #ifndef _RELEASE

        // optional fps limit
        if (maxFPS > 0.0)
        {
            AZ::ScriptTimePoint time;
            AZ::TickRequestBus::BroadcastResult(time, &AZ::TickRequestBus::Events::GetTimeAtCurrentTick);
            if (time.GetMilliseconds() < m_nextRefreshTime)
            {
                return;
            }

            const double ms = 1000.0 / maxFPS;
            m_nextRefreshTime = time.GetMilliseconds() + ms;
        }

        CCamera camera;
        float nearPlane = DEFAULT_NEAR;
        float farPlane = gEnv->p3DEngine->GetMaxViewDistance();
        float fov = DEFAULT_FOV;

        // order of preference
        // 1. existing camera component from provided entity
        // 2. new camera based on the transform of the provided entity
        const AZ::EntityId cameraEntityId = config.m_camera.IsValid() ? config.m_camera : entityId;

        AZ::Transform transform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(transform, cameraEntityId, &AZ::TransformBus::Events::GetWorldTM);
        Matrix34 lyTransform = AZTransformToLYTransform(transform);

        IViewSystem* viewSystem = gEnv->pSystem->GetIViewSystem();
        IView* view = viewSystem->GetViewByEntityId(cameraEntityId);
        if (view)
        {
            // use the camera assigned to this entity (from the camera component)
            camera = view->GetCamera();

            // the view camera near/far plane and fov do not always match
            // what the CameraComponent provides
            Camera::CameraRequestBus::EventResult(nearPlane, cameraEntityId, &Camera::CameraRequestBus::Events::GetNearClipDistance);
            Camera::CameraRequestBus::EventResult(farPlane, cameraEntityId, &Camera::CameraRequestBus::Events::GetFarClipDistance);
            Camera::CameraRequestBus::EventResult(fov, cameraEntityId, &Camera::CameraRequestBus::Events::GetFovRadians);
        }
        else
        {
            camera = gEnv->pSystem->GetViewCamera();
        }

        camera.SetMatrixNoUpdate(lyTransform);
        camera.SetEntityPos(lyTransform.GetTranslation());
        camera.SetFrustum(config.m_renderContextConfig.m_width, config.m_renderContextConfig.m_height, fov, nearPlane, farPlane, camera.GetPixelAspectRatio());
        camera.SetEntityId(entityId);

        // notify users we are about to render to texture
        RenderToTextureNotificationBus::Event(entityId, &RenderToTextureNotificationBus::Events::OnPreRenderToTexture);

        AzRTT::RTTRequestBus::Broadcast(&AzRTT::RTTRequestBus::Events::RenderWorld, m_renderTargetHandle, camera, config.m_renderContextId);

        // notify users we are finished rendering to texture (at least on the main thread)
        RenderToTextureNotificationBus::Event(entityId, &RenderToTextureNotificationBus::Events::OnPostRenderToTexture);

        // update the frame ID if not the active camera.
        // The active camera is updated by the view system.
        if (view && view != viewSystem->GetActiveView())
        {
            CCamera& cameraRef = view->GetCamera();
            cameraRef.IncrementFrameUpdateId();
        }
    }

    void RenderToTextureBase::ValidateCVars() const
    {
        // console may not be available when running unit tests
        if (!gEnv->pConsole)
        {
            return;
        }

        // check some cvars that affect RTT
        ICVar* lodForceUpdate = gEnv->pConsole->GetCVar("e_LodForceUpdate");
        if (lodForceUpdate)
        {
            AZ_WarningOnce("EditorRenderToTextureComponent", lodForceUpdate->GetIVal() == 1, "$2e_LodForceUpdate is off which may lead to object flickering from incorrect LOD calculations per camera.");
        }

        // this is probably not needed if RTT doesn't call PreWorldStreamUpdate()
        ICVar* autoPrecacheCameraJumpDist = gEnv->pConsole->GetCVar("e_autoPrecacheCameraJumpDist");
        if (autoPrecacheCameraJumpDist)
        {
            AZ_WarningOnce("EditorRenderToTextureComponent", autoPrecacheCameraJumpDist->GetIVal() == 0, "$2e_autoPrecacheCameraJumpDist > 0 may lead to thrashing of the streaming precache system.  This should be turned off when multiple cameras are active.");
        }

        ICVar* antialiasingMode = gEnv->pConsole->GetCVar("r_antialiasingmode");
        if (antialiasingMode)
        {
            AZ_WarningOnce("EditorRenderToTextureComponent", antialiasingMode->GetIVal() < 3, "$2Render to texture does not currently support TAA in the main camera and may cause jitter issues.  Set r_antialiasingmode to a value other than 3 (TAA).");
        }

        ICVar* shadowsCache = gEnv->pConsole->GetCVar("r_shadowscache");
        if (shadowsCache)
        {
            AZ_WarningOnce("EditorRenderToTextureComponent", antialiasingMode->GetIVal() > 0, "$2Render to texture does not currently support shadows cache. Set r_shadowscache 0 to turn off shadow caching.");
        }
    }

}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
