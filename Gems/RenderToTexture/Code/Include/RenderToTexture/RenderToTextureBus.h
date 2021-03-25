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

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include <AzCore/Component/ComponentBus.h>
#include <RenderContextConfig.h>

namespace RenderToTexture
{
    /*!
    * Events emitted by a RenderToTextureComponent when enabled and 
    * EditorRenderToTextureComponent when update in editor is enabled.
    */
    class RenderToTextureNotifications
        : public AZ::ComponentBus
    {
    public:
        /**
        * OnPreRenderToTexture is called just before a RenderToTextureComponent 
        * renders a scene to texture on the main thread.
        */
        virtual void OnPreRenderToTexture() = 0;

        /**
        * OnPostRenderToTexture is called just after a RenderToTextureComponent 
        * renders a scene to texture on the main thread.
        */
        virtual void OnPostRenderToTexture() = 0;
    };
    using RenderToTextureNotificationBus = AZ::EBus<RenderToTextureNotifications>;

    /*!
    * Messages serviced by the RenderToTextureComponent and 
    * EditorRenderToTextureComponent.
    */
    class RenderToTextureRequests
        : public AZ::ComponentBus
    {
    public:

        /**
        * Retrieve the texture resource ID for the render target.
        * Internally the engine uses a value of -1 to denote an invalid resource Id
        * @returns the texture resource ID for the render target 
        */
        virtual int GetTextureResourceId() const = 0;

        /**
        * Set the alpha writing mode to use when rendering to this render target.
        * @param mode The alpha mode. (ALPHA_DISABLED, ALPHA_OPAQUE, ALPHA_DEPTH_BASED)
        */
        virtual void SetAlphaMode(AzRTT::AlphaMode mode) = 0;

        /**
        * Set the camera to use for rendering to texture.
        * @param id Entity id of the camera. Use invalid entity id to unset.
        */
        virtual void SetCamera(const AZ::EntityId& id) = 0;

        /**
        * Enable or disable render to texture functionality.  This is useful if 
        * you want to keep all the renderer resources available for this render 
        * target but disable rendering.  Deactivating this component will 
        * disable rendering and free up all resources used by this render target.
        * @param enabled Whether to enable or disable render to texture functionality
        */
        virtual void SetEnabled(bool enabled) = 0;

        /**
        * Set the maximum FPS limit for rendering, 0 for unlimited.
        * @param fps FPS limit or 0 for unlimited.
        */
        virtual void SetMaxFPS(double fps) = 0;

        /**
        * Enable or disable writing sRGB output to the render target (gamma application).
        * @param enabled Whether to enable or disable gamma application.
        */
        virtual void SetWriteSRGBEnabled(bool enabled) = 0;

    };
    using RenderToTextureRequestBus = AZ::EBus<RenderToTextureRequests>;

    class RenderToTextureConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(RenderToTextureConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(RenderToTextureConfig, "{CE284616-E99B-46C0-84FA-77A22D85E6F4}", AZ::ComponentConfig);

        static void Reflect(AZ::ReflectContext* context);

        //! Camera entity (optional)
        AZ::EntityId m_camera;

        //! Render context identifier
        AzRTT::RenderContextId m_renderContextId = AzRTT::RenderContextId::CreateNull();

        //! Render context config settings
        AzRTT::RenderContextConfig m_renderContextConfig;

        //! Maximum refresh rate, 0.f for unlimited.
        double m_maxFPS = 30.f;

        //! Render target name.
        AZStd::string m_textureName = "";

        //! enables drawing a debug image of the render target 
        bool m_displayDebugImage = false;

        //! Whether to update the render target every tick.
        bool m_enabled = true;
    };

} // namespace RenderToTexture

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
