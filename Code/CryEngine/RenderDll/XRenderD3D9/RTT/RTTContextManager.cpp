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

#include "RenderDll_precompiled.h"

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

#include "RTTContextManager.h"
#include "DriverD3D.h"
#include "AzCore/std/smart_ptr/make_shared.h"
#include "RTTContext.h"

namespace AzRTT 
{
    RenderContextManager::RenderContextManager() 
        : m_currentContextId(AzRTT::RenderContextId::CreateNull())

    {
        // NOTE: do not connect to the RTTRequestBus here because it will 
        // get called during LoadLibrary before EBus globals are set up
    }

    RenderContextManager::~RenderContextManager()
    {
        // RTTRequestBus::Handler::BusDisconnect is automatically called in 
        // the ::Handler destructor

        Release();
    }

    void RenderContextManager::Release()
    {
        SetActiveContext(AzRTT::RenderContextId::CreateNull());
        m_renderContexts.clear();
        AzRTT::RTTRequestBus::Handler::BusDisconnect();
    }

    void RenderContextManager::Init()
    {
        AzRTT::RTTRequestBus::Handler::BusConnect();
    }

    bool RenderContextManager::SetActiveContext(AzRTT::RenderContextId contextId)
    {
        bool requestedContextIsActive = true;
        if (m_currentContextId != contextId)
        {
            if (!m_currentContextId.IsNull())
            {
                // deactivate current context
                AZ_Assert(m_renderContexts.find(m_currentContextId) != m_renderContexts.end(), "Old render to texture context ID is invalid.");
                m_renderContexts[m_currentContextId]->SetActive(false);
            }

            // if a null context ID is passed in we just want to deactivate the current context 
            if (!contextId.IsNull())
            {
                // activate the new context
                AZ_Assert(m_renderContexts.find(contextId) != m_renderContexts.end(), "New render to texture context ID is invalid.");
                requestedContextIsActive = m_renderContexts[contextId]->SetActive(true);
            }

            if (requestedContextIsActive)
            {
                m_currentContextId = contextId;

                gRenDev->m_pRT->EnqueueRenderCommand([]()
                {
                    // refresh all shader sampler engine render target textures on render thread
                    CHWShader_D3D::UpdateSamplerEngineTextures();
                });
            }
        }
        return requestedContextIsActive;
    }

    bool RenderContextManager::ContextIsValid(RenderContextId contextId) const
    {
        const AZStd::shared_ptr<RenderContext> context = GetContext(contextId);
        if (context)
        {
            return context->IsValid();
        }

        return false;
    }

    AzRTT::RenderContextId RenderContextManager::CreateContext(const RenderContextConfig& config)
    {
        AzRTT::RenderContextId id = AzRTT::RenderContextId::Create();
        AZStd::shared_ptr<AzRTT::RenderContext> renderContext = AZStd::make_shared<AzRTT::RenderContext>(id, config);
        m_renderContexts.insert(AZStd::pair<AzRTT::RenderContextId, AZStd::shared_ptr<AzRTT::RenderContext>>(id, renderContext));
        return id;
    }

    void RenderContextManager::DestroyContext(AzRTT::RenderContextId contextId)
    {
        AZ_Assert(m_renderContexts.find(contextId) != m_renderContexts.end(), "Invalid render to texture context ID provided in DestroyContext().");

        // do this on the render thread so we can release render thread resources
        gRenDev->m_pRT->EnqueueRenderCommand([this, contextId]()
        {
            // deactivate this context first to free resources and swap
            if (m_currentContextId == contextId)
            {
                SetActiveContext(AzRTT::RenderContextId::CreateNull());
            }

            // always update sampler textures to not point to the render targets.
            CHWShader_D3D::UpdateSamplerEngineTextures();

            // remove the context from our list
            m_renderContexts.erase(contextId);
        });
    }

    AzRTT::RenderContextConfig RenderContextManager::GetContextConfig(AzRTT::RenderContextId contextId) const
    {
        const AZStd::shared_ptr<RenderContext> context = GetContext(contextId);
        if (context)
        {
            return context->GetConfig();
        }

        return AzRTT::RenderContextConfig();
    }

    void RenderContextManager::SetContextConfig(AzRTT::RenderContextId contextId, const AzRTT::RenderContextConfig& config)
    {
        AZStd::shared_ptr<RenderContext> context = GetContext(contextId);
        if (context)
        {
            context->SetConfig(config);
        }
    }

    AZStd::shared_ptr<RenderContext> RenderContextManager::GetContext(RenderContextId contextId) const
    {
        if (!contextId.IsNull())
        {
            const auto itr = m_renderContexts.find(contextId);
            if (itr != m_renderContexts.end())
            {
                return itr->second;
            }
        }

        return nullptr;
    }

    void RenderContextManager::RenderWorld(int renderTargetTextureHandle, const CCamera& camera, RenderContextId contextId)
    {
        CTexture::RenderToTexture(renderTargetTextureHandle, camera, contextId);
    }
};

#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
