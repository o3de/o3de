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
#include <AzCore/Serialization/SerializeContext.h>
#include "RenderToTexture/RenderToTextureBus.h"
#include <RTTBus.h>

namespace RenderToTexture
{
    const int INVALID_RENDER_TARGET = -1;

    class RenderToTextureBase
    {
    public:
        AZ_CLASS_ALLOCATOR(RenderToTextureBase, AZ::SystemAllocator, 0);
        AZ_RTTI(RenderToTextureBase, "{95C6079D-0A1B-4C43-BBAC-68EDF2AA3457}");

        RenderToTextureBase() = default;
        virtual ~RenderToTextureBase() = default;

        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        /**
        * Draw the current render target at the correct aspect ratio
        * @param config the RenderToTextureConfig to use 
        */
        void DisplayDebugImage(const RenderToTextureConfig& config) const;

        /**
        * Render the world to a texture 
        * @param renderTargetHandle the texture handle for the render target 
        * @param config the RenderToTextureConfig that contains the settings including the camera EntityId
        * @param entityId the EntityId to use for notifications and as a fallback camera transform
        */
        void Render(int renderTargetHandle, const RenderToTextureConfig& config, const AZ::EntityId& entityId);

        /**
        * Issues warnings to the log if any problematic cvar settings are found. 
        */
        void ValidateCVars() const;

        //! The resource handle for our render target
        int m_renderTargetHandle = INVALID_RENDER_TARGET;

        //! Next time we will render, used for FPS limiting.  If 0 then there is no FPS limiting. 
        double m_nextRefreshTime = 0.0;
    };
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED