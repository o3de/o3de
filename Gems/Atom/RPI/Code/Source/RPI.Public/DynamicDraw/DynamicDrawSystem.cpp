/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystem.h>

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

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
        }

        RHI::Ptr<DynamicBuffer> DynamicDrawSystem::GetDynamicBuffer(uint32_t size, uint32_t alignment)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexBufferAlloc);
            return m_bufferAlloc->Allocate(size, alignment);
        }

        RHI::Ptr<DynamicDrawContext> DynamicDrawSystem::CreateDynamicDrawContext()
        {
            RHI::Ptr<DynamicDrawContext> drawContext = aznew DynamicDrawContext();
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
            m_dynamicDrawContexts.push_back(drawContext);
            return drawContext;
        }

        // [GFX TODO][ATOM-13184] Add support of draw geometry with material for DynamicDrawSystemInterface
        void DynamicDrawSystem::DrawGeometry([[maybe_unused]] Data::Instance<Material> material, [[maybe_unused]] const GeometryData& geometry, [[maybe_unused]] ScenePtr scene)
        {
            AZ_Error("RPI", false, "Unimplemented function");
        }

        void DynamicDrawSystem::AddDrawPacket(Scene* scene, AZStd::unique_ptr<const RHI::DrawPacket> drawPacket)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
            m_drawPackets[scene].emplace_back(ConstPtr<RHI::DrawPacket>(drawPacket.release()));
        }

        void DynamicDrawSystem::AddDrawPacket(Scene* scene, ConstPtr<RHI::DrawPacket> drawPacket)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
            m_drawPackets[scene].emplace_back(drawPacket);
        }

        void DynamicDrawSystem::SubmitDrawData(Scene* scene, AZStd::vector<ViewPtr> views)
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
                for (RHI::Ptr<DynamicDrawContext> drawContext : m_dynamicDrawContexts)
                {
                    if (drawContext->m_scene == scene)
                    {
                        drawContext->FinalizeDrawList();
                        for (auto& view : views)
                        {
                            drawContext->SubmitDrawList(view);
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

        AZStd::vector<RHI::DrawListView> DynamicDrawSystem::GetDrawListsForPass(const RasterPass* pass)
        {
            AZStd::vector<RHI::DrawListView> result;
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawContext);
            for (RHI::Ptr<DynamicDrawContext> drawContext : m_dynamicDrawContexts)
            {
                if (drawContext->m_pass == pass)
                {
                    drawContext->FinalizeDrawList();
                    auto drawListView = drawContext->GetDrawList();
                    if (drawListView.size() > 0)
                    {
                        result.push_back(drawListView);
                    }
                }
            }
            return result;
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
            }

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
                m_drawPackets.clear();
            }
        }
    }
}
