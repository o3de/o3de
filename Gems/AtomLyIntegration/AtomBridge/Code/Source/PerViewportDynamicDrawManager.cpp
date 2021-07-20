/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PerViewportDynamicDrawManager.h"

#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>

namespace AZ::AtomBridge
{
    PerViewportDynamicDrawManager::PerViewportDynamicDrawManager()
    {
        PerViewportDynamicDraw::Register(this);
    }

    PerViewportDynamicDrawManager::~PerViewportDynamicDrawManager()
    {
        PerViewportDynamicDraw::Unregister(this);
    }

    void PerViewportDynamicDrawManager::RegisterDynamicDrawContext(AZ::Name name, DrawContextFactory contextInitializer)
    {
        AZStd::lock_guard lock(m_mutexDrawContexts);

        const bool alreadyRegistered = m_registeredDrawContexts.find(name) != m_registeredDrawContexts.end();
        AZ_Error("AtomBridge", !alreadyRegistered, "Attempted to call RegisterDynamicDrawContext for already registered name: \"%s\"", name.GetCStr());
        if (alreadyRegistered)
        {
            return;
        }
        m_registeredDrawContexts[name] = contextInitializer;
    }

    void PerViewportDynamicDrawManager::UnregisterDynamicDrawContext(AZ::Name name)
    {
        AZStd::lock_guard lock(m_mutexDrawContexts);

        auto drawContextFactoryIt = m_registeredDrawContexts.find(name);
        const bool registered = drawContextFactoryIt != m_registeredDrawContexts.end();
        AZ_Error("AtomBridge", registered, "Attempted to call UnregisterDynamicDrawContext for unregistered name: \"%s\"", name.GetCStr());
        if (!registered)
        {
            return;
        }
        m_registeredDrawContexts.erase(drawContextFactoryIt);

        for (auto& viewportData : m_viewportData)
        {
            viewportData.second.m_dynamicDrawContexts.erase(name);
        }
    }

    RHI::Ptr<RPI::DynamicDrawContext> PerViewportDynamicDrawManager::GetDynamicDrawContextForViewport(
        AZ::Name name, AzFramework::ViewportId viewportId)
    {
        AZStd::lock_guard lock(m_mutexDrawContexts);

        auto contextFactoryIt = m_registeredDrawContexts.find(name);
        if (contextFactoryIt == m_registeredDrawContexts.end())
        {
            return nullptr;
        }

        auto viewportContextManager = RPI::ViewportContextRequests::Get();
        RPI::ViewportContextPtr viewportContext = viewportContextManager->GetViewportContextById(viewportId);
        if (viewportContext == nullptr)
        {
            return nullptr;
        }

        // Get or create a ViewportData if one doesn't already exist
        ViewportData& viewportData = m_viewportData[viewportId];
        if (!viewportData.m_initialized)
        {
            viewportData.m_pipelineChangedHandler = AZ::Event<RPI::RenderPipelinePtr>::Handler([this, viewportId](RPI::RenderPipelinePtr pipeline)
            {
                AZStd::lock_guard lock(m_mutexDrawContexts);
                ViewportData& viewportData = m_viewportData[viewportId];
                for (auto& context : viewportData.m_dynamicDrawContexts)
                {
                    context.second->SetRenderPipeline(pipeline.get());
                }
            });
            viewportData.m_viewportDestroyedHandler = AZ::Event<AzFramework::ViewportId>::Handler([this, viewportId](AzFramework::ViewportId id)
            {
                AZStd::lock_guard lock(m_mutexDrawContexts);
                m_viewportData.erase(id);
            });

            viewportContext->ConnectCurrentPipelineChangedHandler(viewportData.m_pipelineChangedHandler);
            viewportContext->ConnectAboutToBeDestroyedHandler(viewportData.m_viewportDestroyedHandler);

            viewportData.m_initialized = true;
        }

        RHI::Ptr<RPI::DynamicDrawContext>& context = viewportData.m_dynamicDrawContexts[name];
        if (context == nullptr)
        {
            auto pipeline = viewportContext->GetCurrentPipeline().get();
            if (pipeline == nullptr)
            {
                return nullptr;
            }
            context = RPI::DynamicDrawInterface::Get()->CreateDynamicDrawContext(pipeline);
            contextFactoryIt->second(context);
        }

        return context;
    }
} //namespace AZ::AtomBridge
