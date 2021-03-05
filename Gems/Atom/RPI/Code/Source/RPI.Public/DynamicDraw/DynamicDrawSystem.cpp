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

#include <AzCore/Interface/Interface.h>

namespace AZ
{
    namespace RPI
    {
        DynamicDrawSystemInterface2* DynamicDrawSystemInterface2::Get()
        {
            return Interface<DynamicDrawSystemInterface2>::Get();
        }

        void DynamicDrawSystem::Init(const DynamicDrawSystemDescriptor& descriptor)
        {
            m_bufferAlloc = AZStd::make_unique<DynamicBufferAllocator>();
            if (m_bufferAlloc)
            {
                m_bufferAlloc->Init(descriptor.m_dynamicBufferPoolSize);
                Interface<DynamicDrawSystemInterface2>::Register(this);
            }
        }

        void  DynamicDrawSystem::Shutdown()
        {
            if (m_bufferAlloc)
            {
                Interface<DynamicDrawSystemInterface2>::Unregister(this);
                m_bufferAlloc->Shutdown();
                m_bufferAlloc = nullptr;
            }
            m_dynamicDrawContexts.clear();
        }

        RHI::Ptr<DynamicBuffer> DynamicDrawSystem::GetDynamicBuffer(uint32_t size, uint32_t alignment)
        {
            return m_bufferAlloc->Allocate(size, alignment);
        }

        RHI::Ptr<DynamicDrawContext> DynamicDrawSystem::CreateDynamicDrawContext(Scene* scene)
        {
            RHI::Ptr<DynamicDrawContext> drawContext = aznew DynamicDrawContext();
            drawContext->m_scene = scene;
            m_dynamicDrawContexts.push_back(drawContext);
            return drawContext;
        }

        // [GFX TODO][ATOM-13185] Add support for creating DynamicDrawContext for Pass
        RHI::Ptr<DynamicDrawContext> DynamicDrawSystem::CreateDynamicDrawContext([[maybe_unused]] Pass* pass)
        {
            AZ_Error("RPI", false, "Unimplemented function");
            return nullptr;
        }

        // [GFX TODO][ATOM-13184] Add support of draw geometry with material for DynamicDrawSystemInterface
        void DynamicDrawSystem::DrawGeometry([[maybe_unused]] Data::Instance<Material> material, [[maybe_unused]] const GeometryData& geometry, [[maybe_unused]] ScenePtr scene)
        {
            AZ_Error("RPI", false, "Unimplemented function");
        }

        void DynamicDrawSystem::SubmitDrawData(Scene* scene, AZStd::vector<ViewPtr> views)
        {
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
        }

        void DynamicDrawSystem::FrameEnd()
        {
            m_bufferAlloc->FrameEnd();

            // Clean up released dynamic draw contexts (which use count is 1)
            auto unused = AZStd::remove_if(m_dynamicDrawContexts.begin(), m_dynamicDrawContexts.end(),
                [](const RHI::Ptr<DynamicDrawContext>& drawContext)
                {
                    return drawContext->use_count() == 1;
                });
            m_dynamicDrawContexts.erase(unused, m_dynamicDrawContexts.end());

            // Call FrameEnd for each DynamicDrawContext;
            AZStd::for_each(m_dynamicDrawContexts.begin(), m_dynamicDrawContexts.end(),
                [](const RHI::Ptr<DynamicDrawContext>& drawContext)
                {
                    drawContext->FrameEnd();
                });
        }
    }
}
