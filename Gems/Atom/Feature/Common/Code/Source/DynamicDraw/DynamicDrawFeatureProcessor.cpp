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

#include <Atom/Feature/DynamicDraw/DynamicDrawFeatureProcessor.h>

#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/parallel/lock.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/View.h>

#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>

#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/Device.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/ImagePool.h>

namespace AZ
{
    namespace Render
    {

        void DynamicDrawFeatureProcessor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DynamicDrawFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        DynamicDrawFeatureProcessor::DynamicDrawFeatureProcessor()
        {
        }

        DynamicDrawFeatureProcessor::~DynamicDrawFeatureProcessor()
        {
        }

        void DynamicDrawFeatureProcessor::Activate()
        {
            Interface<RPI::DynamicDrawSystemInterface>::Get()->RegisterDynamicDrawForScene(this, GetParentScene());

            RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get(); 
            AZ_Assert(rhiSystem, "RHISystemInterface not initialized yet");
            RHI::Ptr<RHI::Device> device = rhiSystem->GetDevice();

            AZ::RHI::Factory& factory = RHI::Factory::Get();

            RHI::BufferPoolDescriptor bufferPoolDesc;
            RHI::ResultCode result;

            m_inputAssemblyBufferHostPool = factory.CreateBufferPool();
            bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::InputAssembly;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Host;
            bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
            result = m_inputAssemblyBufferHostPool->Init(*device, bufferPoolDesc);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to init m_inputAssemblyBufferHostPool");

            m_constantBufferDevicePool = factory.CreateBufferPool();
            bufferPoolDesc.m_bindFlags = RHI::BufferBindFlags::Constant;
            bufferPoolDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
            bufferPoolDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
            result = m_constantBufferDevicePool->Init(*device, bufferPoolDesc);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to init m_constantBufferDevicePool");

            m_imagePool = factory.CreateImagePool();
            RHI::ImagePoolDescriptor imagePoolDesc;
            imagePoolDesc.m_bindFlags = RHI::ImageBindFlags::ShaderRead;
            result = m_imagePool->Init(*device, imagePoolDesc);
            AZ_Assert(result == RHI::ResultCode::Success, "Failed to init m_imagePool");
        }

        void DynamicDrawFeatureProcessor::Deactivate()
        {
            Interface<RPI::DynamicDrawSystemInterface>::Get()->UnregisterDynamicDrawForScene(GetParentScene());

            m_inputAssemblyBufferHostPool = nullptr;
            m_constantBufferDevicePool = nullptr;
            for (auto& dpList : m_drawPackets)
            {
                dpList.clear();
            }
        }

        void DynamicDrawFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& renderPacket)
        {
            AZ_ATOM_PROFILE_FUNCTION("FeatureProcessor", "DynamicDrawFeatureProcessor: Render");

            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
            AZStd::swap(m_writeBufferIdx, m_submitBufferIdx);

            // Delete the DrawPackets that were in-flight last frame, make room for new incoming DrawPackets
            m_drawPackets[m_writeBufferIdx].clear();                    

            m_preRenderNotificationEvent.Signal(0);

            // Submit draw packets...
            for (auto& dp : m_drawPackets[m_submitBufferIdx])
            {
                for (const RPI::ViewPtr& view : renderPacket.m_views)
                {
                    view->AddDrawPacket(dp.get());
                }
            }
        }

        void DynamicDrawFeatureProcessor::AddDrawPacket(AZStd::unique_ptr<const RHI::DrawPacket> drawPacket)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_mutexDrawPackets);
            m_drawPackets[m_writeBufferIdx].emplace_back(AZStd::move(drawPacket));
        }

        RHI::Ptr<RHI::BufferPool>& DynamicDrawFeatureProcessor::GetInputAssemblyBufferHostPool()
        {
            AZ_Assert(m_inputAssemblyBufferHostPool->IsInitialized(), "m_inputAssemblyBufferHostPool not initialized!");
            return m_inputAssemblyBufferHostPool;
        }

        RHI::Ptr<RHI::BufferPool>& DynamicDrawFeatureProcessor::GetConstantBufferDevicePool()
        {
            AZ_Assert(m_constantBufferDevicePool->IsInitialized(), "m_constantBufferDevicePool not initialized!");
            return m_constantBufferDevicePool;
        }

        RHI::Ptr<RHI::ImagePool>& DynamicDrawFeatureProcessor::GetImagePool()
        {
            AZ_Assert(m_imagePool->IsInitialized(), "m_imagePool not initialized!");
            return m_imagePool;
        }

        void DynamicDrawFeatureProcessor::RegisterGeometryPreRenderNotificationHandler(RPI::DynamicDrawPreRenderNotificationHandler& handler)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_eventMutex);

            handler.Connect(m_preRenderNotificationEvent);
        }

        void DynamicDrawFeatureProcessor::UnregisterGeometryPreRenderNotificationHandler(RPI::DynamicDrawPreRenderNotificationHandler& handler)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_eventMutex);

            handler.Disconnect();
        }


    } // namespace Render
} // namespace RPI
