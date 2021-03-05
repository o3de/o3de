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

#include "AzCore/std/smart_ptr/shared_ptr.h"
#include "AzCore/std/containers/vector.h"
#include <RTTBus.h>

namespace AzRTT 
{
    class RenderContext;

    class RenderContextManager
        : protected AzRTT::RTTRequestBus::Handler
    {
    public:
        RenderContextManager();
        virtual ~RenderContextManager();

        //////////////////////////////////////////////////////////////////////////
        // AzRTT::RTTRequestBus::Handler 
        bool ContextIsValid(RenderContextId contextId) const override;

        RenderContextId CreateContext(const RenderContextConfig& config) override;

        void DestroyContext(RenderContextId contextId) override;

        bool SetActiveContext(RenderContextId contextId) override;

        AzRTT::RenderContextConfig GetContextConfig(RenderContextId contextId) const override;

        void SetContextConfig(RenderContextId contextId, const RenderContextConfig& config) override;

        void RenderWorld(int renderTargetTextureHandle, const CCamera& camera, RenderContextId contextId) override;
        //////////////////////////////////////////////////////////////////////////

        //! connect to event busses
        void Init();

        //! release resources
        void Release();

    private:
        AZStd::shared_ptr<RenderContext> GetContext(RenderContextId contextId) const;

        AzRTT::RenderContextId m_currentContextId;
        AZStd::unordered_map<AzRTT::RenderContextId, AZStd::shared_ptr<RenderContext>> m_renderContexts;
    };
}

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
