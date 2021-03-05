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

// AZ_RENDER_TO_TEXTURE_GEM_ENABLED is defined by the RenderToTexture Gem
#if !defined(AZ_RENDER_TO_TEXTURE_GEM_ENABLED)
#define AZ_RENDER_TO_TEXTURE_GEM_ENABLED      (0)
#endif

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include <RenderContextConfig.h>
#include <AzCore/EBus/EBus.h>

class CCamera;

namespace AzRTT
{
    class RTTBus 
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~RTTBus() = default;

        /**
        * Creates an instance of an RenderContext.
        * @return Shared pointer to the created IRenderContext.
        * @param config The RenderContextConfig settings to apply.
        */
        virtual RenderContextId CreateContext(const RenderContextConfig& config) = 0;

        /**
        * Returns true if the IRenderContext exists and is valid.
        * @return True if context exists and is valid.
        * @param contextId the RenderContextId to check for validity 
        */
        virtual bool ContextIsValid(RenderContextId contextId) const = 0;

        /**
        * Destroys an RenderContext instance.
        * @param contextId The RenderContextId for an existing context to destroy 
        */
        virtual void DestroyContext(RenderContextId contextId) = 0;

        /**
        * Gets the configuration for a context instance.
        * @param contextId the RenderContextId for an existing context.
        */
        virtual RenderContextConfig GetContextConfig(RenderContextId contextId) const = 0;

        /**
        * Sets the active IRenderContext. This will deactivate any existing context.
        * @return True if context was activated, false if not activated.
        * @param contextId the RenderContextId for an existing context to activate
        */
        virtual bool SetActiveContext(RenderContextId contextId) = 0;

        /**
        * Sets the configuration for a context instance.
        * @param contextId the RenderContextId for an existing context.
        * @param config the RenderContextConfig settings to apply.
        */
        virtual void SetContextConfig(RenderContextId contextId, const RenderContextConfig& config) = 0;

        /**
        * Render the world from the provided camera to a render target using the provided render context.
        * @param renderTargetTextureHandle Texture handle of target to render to.
        * @param camera The camera to use for rendering.
        * @param contextId The RenderContextId to use.
        */
        virtual void RenderWorld(int renderTargetTextureHandle, const CCamera& camera, RenderContextId contextId) = 0;
    };

    using RTTRequestBus = AZ::EBus<RTTBus>;
}

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
