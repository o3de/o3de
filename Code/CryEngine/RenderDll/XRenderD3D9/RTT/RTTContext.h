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

/**
* @file
* Header file for render context(RenderContext).  The render to texture system
* stores graphics resources and configuration data in a RenderContext for use
* when rendering the scene to a texture.
*/

#pragma once

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/bind/bind.h>
#include <AzCore/std/functional.h>
#include "RTT/RTTSwappableRenderTarget.h"
#include "RTT/RTTSwappableCVar.h"
#include "RenderContextConfig.h"

namespace AzRTT 
{
    //! The maximum number of swappable render targets to use
    const AZStd::size_t MaxSwappableRenderTargets = 128;

    class RenderContext
    {
    public:
        RenderContext(RenderContextId id);
        RenderContext(RenderContextId id, const RenderContextConfig& config);

        virtual ~RenderContext();

        enum class ErrorState
        {
            OK,
            ResourceCreationFailed,
        };

        /**
        * This context's unique identifier 
        * @return this context's unique identifier 
        */
        RenderContextId GetContextId() const { return m_renderContextId; }

        /**
        * Return true if all context resources have been created 
        * @return true if all resources have been created 
        */
        bool ResourcesCreated() const;

        /**
        * Set this context's active state. This may fail if the context is
        * not valid due to there not being enough memory to activate. 
        * @return true if the active state was set, false if not 
        * @param active the active state 
        */
        bool SetActive(bool active);

        /**
        * Get this RenderContext's configuration settings 
        * @return this RenderContext's RenderContextConfig 
        */
        const RenderContextConfig& GetConfig() const { return m_config;  }

        /**
        * Set this RenderContext's configuration settings 
        * @param config the new RenderContextConfig to use
        */
        void SetConfig(const RenderContextConfig& config);

        /**
        * Return true if this context is valid.  A context may be invalid
        * if there are not enough resources available to activate it.
        * @return true if this context is valid 
        */
        bool IsValid() const { return m_errorState == ErrorState::OK; }

    private:

        //! Only the RenderContextManager should create RenderContext objects
        RenderContext() = delete;

        bool Initialize();
        void Release();

        bool CreateRenderTargets(AZ::u32 width, AZ::u32 height);
        bool CreateDepthTargets(AZ::u32 width, AZ::u32 height);
        bool ResizeRenderTargets(AZ::u32 width, AZ::u32 height);

        bool RenderTargetsAreValid() const;
        bool DepthTargetsAreValid() const;

        void SetActiveMainThread(bool active);
        void SetActiveRenderThread(bool active);

        //! unique identifier
        RenderContextId m_renderContextId = 0;

        //! configuration settings
        RenderContextConfig m_config;

        //! fixed vector to avoid problematic copy/delete that occurs with resizable vectors 
        AZStd::fixed_vector<SwappableRenderTarget, AzRTT::MaxSwappableRenderTargets> m_swappableRenderTargets;

        //! active state
        bool m_active;

        //! the error state this context is in if any
        ErrorState m_errorState = ErrorState::OK;

        //! viewports
        SViewport m_viewport;

        //! depth textures
        SDepthTexture *m_depthTarget;
        SDepthTexture *m_depthTargetMSAA;

        //! resource views
        D3DShaderResourceView* m_zBufferDepthReadOnlySRV;
        D3DShaderResourceView* m_zBufferStencilReadOnlySRV;

        struct RendererSettings
        {
            SViewport m_viewport;
            SDepthTexture m_depthOrig;
            SDepthTexture m_depthMSAA;
        };

        //! renderer settings to backup/restore
        RendererSettings m_previousSettings;

        AZStd::unordered_map<AZStd::string, SwappableCVar<int>> m_iCVars;
        AZStd::unordered_map<AZStd::string, SwappableCVar<float>> m_fCVars;
    };
}

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED