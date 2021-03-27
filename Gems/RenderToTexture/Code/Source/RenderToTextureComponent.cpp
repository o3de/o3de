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

#include "RenderToTextureComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Transform.h>
#include <MathConversion.h>
#include <IRenderer.h>
#include <I3DEngine.h>
#include <IViewSystem.h>
#include <AzFramework/Components/CameraBus.h>
#include <RTTBus.h>

namespace AzRTT
{
    void RenderContextConfig::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RenderContextConfig>()
                ->Version(1)
                ->Field("Width", &RenderContextConfig::m_width)
                ->Field("Height", &RenderContextConfig::m_height)
                ->Field("Enable Gamma", &RenderContextConfig::m_sRGBWrite)
                ->Field("Alpha Output Mode", &RenderContextConfig::m_alphaMode)
                ->Field("Enable Ocean", &RenderContextConfig::m_oceanEnabled)
                ->Field("Enable Terrain", &RenderContextConfig::m_terrainEnabled)
                ->Field("Enable Vegetation", &RenderContextConfig::m_vegetationEnabled)
                ->Field("Enable Shadows", &RenderContextConfig::m_shadowsEnabled)
                ->Field("GSM LODs", &RenderContextConfig::m_shadowsNumCascades)
                ->Field("GSM Range", &RenderContextConfig::m_shadowsGSMRange)
                ->Field("GSM Range Step", &RenderContextConfig::m_shadowsGSMRangeStep)
                ->Field("Antialiasing Mode", &RenderContextConfig::m_aaMode)
                ->Field("Enable Depth Of Field", &RenderContextConfig::m_depthOfFieldEnabled)
                ->Field("Enable Motion Blur", &RenderContextConfig::m_motionBlurEnabled)
                ;
        }
    }
}

namespace RenderToTexture
{
    /// BahaviorContext forwarder for RenderToTextureNotificationBus
    class BehaviorRenderToTextureNotificationBusHandler : public RenderToTexture::RenderToTextureNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorRenderToTextureNotificationBusHandler, "{8E5D1D55-317B-42FA-A8A4-AC410B671C87}", AZ::SystemAllocator,
            OnPreRenderToTexture, OnPostRenderToTexture);

        void OnPreRenderToTexture() override
        {
            Call(FN_OnPreRenderToTexture);
        }

        void OnPostRenderToTexture() override
        {
            Call(FN_OnPostRenderToTexture);
        }
    };

    void RenderToTextureConfig::Reflect(AZ::ReflectContext* reflection)
    {
        AzRTT::RenderContextConfig::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RenderToTextureConfig>()
                ->Version(1)
                ->Field("Enabled", &RenderToTextureConfig::m_enabled)
                ->Field("Camera", &RenderToTextureConfig::m_camera)
                ->Field("Texture Name", &RenderToTextureConfig::m_textureName)
                ->Field("Max FPS", &RenderToTextureConfig::m_maxFPS)
                ->Field("Render Context Config", &RenderToTextureConfig::m_renderContextConfig)
                ->Field("Display Debug Image", &RenderToTextureConfig::m_displayDebugImage);
        }
    }

    void RenderToTextureComponent::Reflect(AZ::ReflectContext* reflection)
    {
        RenderToTextureBase::Reflect(reflection);

        RenderToTextureConfig::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<RenderToTextureComponent, AZ::Component, RenderToTextureBase>()
                ->Version(1)
                ->Field("Config", &RenderToTextureComponent::m_config);
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->EBus<RenderToTextureRequestBus>("RenderToTextureRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Event("GetTextureResourceId", &RenderToTextureRequestBus::Events::GetTextureResourceId)
                ->Event("SetCamera", &RenderToTextureRequestBus::Events::SetCamera, { { { "Camera" , "The entity ID of the camera to use for rendering" } } })
                ->Event("SetEnabled", &RenderToTextureRequestBus::Events::SetEnabled, { { { "Enabled" , "Enable or disable rendering to texture" } } })
                ->Event("SetMaxFPS", &RenderToTextureRequestBus::Events::SetMaxFPS, { { { "FPS" , "Limit how often the scene is re-rendered and the render target is updated" } } })
                ->Event("SetWriteGamma", &RenderToTextureRequestBus::Events::SetWriteSRGBEnabled, { { { "Enabled" , "Enable or disable gamma application in the render target output." } } })
                ->Event("SetAlphaMode", &RenderToTextureRequestBus::Events::SetAlphaMode, { { { "Mode" , "Set the alpha mode (0 = disabled, 1 = opaque, 2 = depth based)" } } });

            behaviorContext->EBus<RenderToTextureNotificationBus>("RenderToTextureNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "Rendering")
                ->Handler<BehaviorRenderToTextureNotificationBusHandler>();
        }
    }

    void RenderToTextureComponent::Activate()
    {
        if (gEnv->IsDedicated())
        {
            AZ_WarningOnce("RenderToTextureComponent", gEnv->IsDedicated(), "$2RenderToTexture is not supported in dedicated server mode.");
            return;
        }

        if (!m_config.m_camera.IsValid())
        {
            m_config.m_camera = GetEntityId();
        }

        AzRTT::RTTRequestBus::BroadcastResult(m_config.m_renderContextId, &AzRTT::RTTRequestBus::Events::CreateContext, m_config.m_renderContextConfig);
        if (m_config.m_renderContextId.IsNull())
        {
            AZ_Printf("RenderToTextureComponent", "$2Failed to create render context.");
        }
        else if (gEnv->pRenderer)
        {
            m_renderTargetHandle = gEnv->pRenderer->CreateRenderTarget(m_config.m_textureName.c_str(), m_config.m_renderContextConfig.m_width, m_config.m_renderContextConfig.m_height, Clr_Unknown, eTF_R8G8B8A8);
            if (m_renderTargetHandle <= 0)
            {
                AZ_Printf("RenderToTextureComponent", "$2Failed to create render target.");
            }
            else if (m_config.m_enabled)
            {
                AZ::TickBus::Handler::BusConnect();

#ifndef _RELEASE
                // validate cvar settings
                ValidateCVars();
#endif // ifndef _RELEASE
            }
        }

        RenderToTextureRequestBus::Handler::BusConnect(GetEntityId());
    }

    void RenderToTextureComponent::Deactivate()
    {
        if (!m_config.m_renderContextId.IsNull())
        {
            AzRTT::RTTRequestBus::Broadcast(&AzRTT::RTTRequestBus::Events::DestroyContext, m_config.m_renderContextId);
        }

        if (m_renderTargetHandle >= 0)
        {
            if (gEnv->pRenderer)
            {
                gEnv->pRenderer->DestroyRenderTarget(m_renderTargetHandle);
            }
            m_renderTargetHandle = -1;
        }

        RenderToTextureRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    int RenderToTextureComponent::GetTickOrder()
    {
        return AZ::TICK_LAST;
    }

    void RenderToTextureComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        // push the changes from the component's RenderContextConfig to our RenderContext
        // do this here before we render so we avoid threading issues.
        if (m_configDirty && !m_config.m_renderContextId.IsNull())
        {
            AzRTT::RTTRequestBus::Broadcast(&AzRTT::RTTRequestBus::Events::SetContextConfig, m_config.m_renderContextId, m_config.m_renderContextConfig);
            m_configDirty = false;
        }

        Render(m_renderTargetHandle, m_config, GetEntityId());
    }

    bool RenderToTextureComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const RenderToTextureConfig*>(baseConfig))
        {
            m_config = *config;
            return true;
        }
        return false;
    }

    bool RenderToTextureComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<RenderToTextureConfig*>(outBaseConfig))
        {
            *outConfig = m_config;
            return true;
        }
        return false;
    }

    int RenderToTextureComponent::GetTextureResourceId() const
    {
        return m_renderTargetHandle;
    }

    void RenderToTextureComponent::SetEnabled(bool enabled)
    {
        if (enabled != m_config.m_enabled)
        {
            m_config.m_enabled = enabled;
        }
    }

    void RenderToTextureComponent::SetWriteSRGBEnabled(bool enabled)
    {
        m_config.m_renderContextConfig.m_sRGBWrite = enabled;
        m_configDirty = true;
    }

    void RenderToTextureComponent::SetAlphaMode(AzRTT::AlphaMode mode)
    {
        m_config.m_renderContextConfig.m_alphaMode = mode;
        m_configDirty = true;
    }

    void RenderToTextureComponent::SetCamera(const AZ::EntityId& id)
    {
        m_config.m_camera = id;
        m_configDirty = true;
    }

    void RenderToTextureComponent::SetMaxFPS(double fps)
    {
        m_config.m_maxFPS = fps;
        m_configDirty = true;
    }
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
