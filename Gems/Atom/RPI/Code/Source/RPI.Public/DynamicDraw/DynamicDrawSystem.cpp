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

#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystem.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>

#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace RPI
    {
        DynamicDrawInterface* DynamicDrawInterface::Get()
        {
            return Interface<DynamicDrawInterface>::Get();
        }

        void DynamicDrawSystem::Init(const DynamicDrawSystemDescriptor& descriptor)
        {
            m_bufferAlloc = AZStd::make_unique<DynamicBufferAllocator>();
            if (m_bufferAlloc)
            {
                m_bufferAlloc->Init(descriptor.m_dynamicBufferPoolSize);
                Interface<DynamicDrawInterface>::Register(this);
            }
            ViewportContextManagerNotificationsBus::Handler::BusConnect();
        }

        void  DynamicDrawSystem::Shutdown()
        {
            if (m_bufferAlloc)
            {
                Interface<DynamicDrawInterface>::Unregister(this);
                m_bufferAlloc->Shutdown();
                m_bufferAlloc = nullptr;
            }
            m_dynamicDrawContexts.clear();
            ViewportContextManagerNotificationsBus::Handler::BusDisconnect();
        }

        void DynamicDrawSystem::RegisterPerViewportDynamicDrawContext(AZ::Name name, DrawContextFactory contextInitializer)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
            m_registeredNamedDrawContexts[name] = contextInitializer;
        }

        void DynamicDrawSystem::UnregisterPerViewportDynamicDrawContext(AZ::Name name)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
            m_registeredNamedDrawContexts.erase(name);
        }

        RHI::Ptr<DynamicDrawContext> DynamicDrawSystem::GetDynamicDrawContextForViewport(AZ::Name name, AzFramework::ViewportId viewportId)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);

            auto contextFactoryIt = m_registeredNamedDrawContexts.find(name);
            if (contextFactoryIt == m_registeredNamedDrawContexts.end())
            {
                return nullptr;
            }

            auto viewportContextMananger = ViewportContextRequests::Get();
            ViewportContextPtr viewportContext = viewportContextMananger->GetViewportContextById(viewportId);
            if (viewportContext == nullptr)
            {
                return nullptr;
            }

            NamedDrawContextViewportInfo& contextInfo = m_namedDynamicDrawContextInstances[viewportId];
            if (!contextInfo.m_initialized)
            {
                // Ensure that our cached debug draws use the correct render pipeline info.
                contextInfo.m_pipelineChangeHandler = AZ::Event<RenderPipelinePtr>::Handler([this, viewportId](RenderPipelinePtr pipeline)
                {
                    AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
                    NamedDrawContextViewportInfo& contextInfo = m_namedDynamicDrawContextInstances[viewportId];
                    contextInfo.m_scene = pipeline ? pipeline->GetScene() : nullptr;
                    for (auto& context : contextInfo.m_dynamicDrawContexts)
                    {
                        context.second->SetRenderPipeline(pipeline.get());
                    }
                });

                viewportContext->ConnectCurrentPipelineChangedHandler(contextInfo.m_pipelineChangeHandler);
                contextInfo.m_scene = viewportContext->GetRenderScene().get();

                contextInfo.m_initialized = true;
            }

            RHI::Ptr<DynamicDrawContext>& context = contextInfo.m_dynamicDrawContexts[name];
            if (context == nullptr)
            {
                RenderPipelinePtr pipeline = viewportContext->GetCurrentPipeline();
                if (pipeline == nullptr || pipeline->GetScene() == nullptr)
                {
                    return nullptr;
                }
                context = aznew DynamicDrawContext();
                context->SetRenderPipeline(pipeline.get());
                contextFactoryIt->second(context);
            }

            return context;
        }

        RHI::Ptr<DynamicBuffer> DynamicDrawSystem::GetDynamicBuffer(uint32_t size, uint32_t alignment)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexBufferAlloc);
            return m_bufferAlloc->Allocate(size, alignment);
        }

        RHI::Ptr<DynamicDrawContext> DynamicDrawSystem::CreateDynamicDrawContext(Scene* scene)
        {
            if (!scene)
            {
                AZ_Error("RPI", false, "Failed to create a DynamicDrawContext: the input scene is invalid");
                return nullptr;
            }
            RHI::Ptr<DynamicDrawContext> drawContext = aznew DynamicDrawContext();
            drawContext->m_scene = scene;

            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
            m_dynamicDrawContexts.push_back(drawContext);
            return drawContext;
        }

        RHI::Ptr<DynamicDrawContext> DynamicDrawSystem::CreateDynamicDrawContext(RenderPipeline* pipeline)
        {
            if (!pipeline || !pipeline->GetScene())
            {
                AZ_Error("RPI", false, "Failed to create a DynamicDrawContext: the input RenderPipeline is invalid or wasn't added to a Scene");
                return nullptr;
            }

            auto context = CreateDynamicDrawContext(pipeline->GetScene());
            context->m_drawFilter = pipeline->GetDrawFilterMask();
            return context;
        }

        // [GFX TODO][ATOM-13184] Add support of draw geometry with material for DynamicDrawSystemInterface
        void DynamicDrawSystem::DrawGeometry([[maybe_unused]] Data::Instance<Material> material, [[maybe_unused]] const GeometryData& geometry, [[maybe_unused]] ScenePtr scene)
        {
            AZ_Error("RPI", false, "Unimplemented function");
        }

        void DynamicDrawSystem::AddDrawPacket(Scene* scene, AZStd::unique_ptr<const RHI::DrawPacket> drawPacket)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
            m_drawPackets[scene].emplace_back(AZStd::move(drawPacket));
        }

        void DynamicDrawSystem::SubmitDrawData(Scene* scene, AZStd::vector<ViewPtr> views)
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
                for (RHI::Ptr<DynamicDrawContext> drawContext : m_dynamicDrawContexts)
                {
                    if (drawContext->m_scene == scene)
                    {
                        for (auto& view : views)
                        {
                            drawContext->SubmitDrawData(view);
                        }
                    }
                }

                for (auto& namedContextInfo : m_namedDynamicDrawContextInstances)
                {
                    if (namedContextInfo.second.m_scene == scene)
                    {
                        for (auto& view : views)
                        {
                            for (auto& drawContextData : namedContextInfo.second.m_dynamicDrawContexts)
                            {
                                drawContextData.second->SubmitDrawData(view);
                            }
                        }
                    }
                }
            }

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
                for (auto& dp : m_drawPackets[scene])
                {
                    for (auto& view : views)
                    {
                        view->AddDrawPacket(dp.get());
                    }
                }
            }
        }

        void DynamicDrawSystem::FrameEnd()
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutexBufferAlloc);
                m_bufferAlloc->FrameEnd();
            }

            // Clean up released dynamic draw contexts (which use count is 1)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
                auto unused = AZStd::remove_if(
                    m_dynamicDrawContexts.begin(), m_dynamicDrawContexts.end(), [](const RHI::Ptr<DynamicDrawContext>& drawContext) {
                        return drawContext->use_count() == 1;
                    });
                m_dynamicDrawContexts.erase(unused, m_dynamicDrawContexts.end());

                // Call FrameEnd for each DynamicDrawContext;
                AZStd::for_each(
                    m_dynamicDrawContexts.begin(), m_dynamicDrawContexts.end(), [](const RHI::Ptr<DynamicDrawContext>& drawContext) {
                        drawContext->FrameEnd();
                    });
                for (auto& namedContextInfo : m_namedDynamicDrawContextInstances)
                {
                    for (auto& drawContextData : namedContextInfo.second.m_dynamicDrawContexts)
                    {
                        drawContextData.second->FrameEnd();
                    }
                }
            }

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
                m_drawPackets.clear();
            }
        }

        void DynamicDrawSystem::OnViewportContextRemoved(AzFramework::ViewportId viewportId)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
            m_namedDynamicDrawContextInstances.erase(viewportId);
        }
    }
}
