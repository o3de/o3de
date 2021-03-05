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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Matrix4x4.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/EBus/Event.h>

#include <Atom/RHI/DrawPacketBuilder.h>

namespace AZ
{
    namespace RHI
    {
        class BufferPool;
        class ImagePool;
    }

    namespace RPI
    {
        class Scene;

        using DynamicDrawPreRenderNotificationHandler = Event<int>::Handler;

        //! This provides an interface for submitting generic draw packets to the renderer. 
        //! It interfaces to DynamicDrawFeatureProcessor.
        //!
        //! Usage (see DynamicDrawExampleComponent.h/.cpp for a working example): 
        //!  Initialize the various input buffers, stream buffer views, 
        //!  shaders, shader variants, output attachment layout, draw list tags, and pipeline states
        //!  that your system will use to build draw packets.
        //!  
        //!  Once a frame (before DynamicDrawFeatureProcessor::Render() is called):
        //!      Generate DrawPackets using AZ::RHI::DrawPacketBuilder.
        //!      call DynamicDrawSystemInterface->AddDrawPacket() to submit a DrawPacket to be rendered.
        class DynamicDrawInterface
        {
        public:
            DynamicDrawInterface() = default;
            virtual ~DynamicDrawInterface() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(DynamicDrawInterface);

            //! Register an Event handler to receive the notification that draws are about to be processed.
            //! Use this event to control marshalling of your geometry buffers.
            //! Note: geometry buffers can't be orphaned until after command buffers have been generated from
            //! the draw packets. This necessitates double buffering of your geometry buffers.
            virtual void RegisterGeometryPreRenderNotificationHandler(DynamicDrawPreRenderNotificationHandler& handler) = 0;

            //! Unregister an Event handler to receive the notification that draws are about to be processed.
            virtual void UnregisterGeometryPreRenderNotificationHandler(DynamicDrawPreRenderNotificationHandler& handler) = 0;
            
            //! Submits a DrawPacket to the renderer.
            //! Note that ownership of the DrawPacket pointer is passed to the DynamicDrawFeatureProcessor.
            //! (it will be cleaned up correctly since the DrawPacket keeps track of the allocator that was used when it was built)
            virtual void AddDrawPacket(AZStd::unique_ptr<const RHI::DrawPacket> drawPacket) = 0;

            //! For convenience, this provides a BufferPool with bindFlags=InputAssembly, heapMemoryLevel=Host, hostMemoryAccess=Write.
            //! This type of buffer pool resides on the CPU and is a good choice for for dynamic index and vertex buffers that are fully updated every frame.
            virtual RHI::Ptr<RHI::BufferPool>& GetInputAssemblyBufferHostPool() = 0;

            //! For convenience, this provides a BufferPool with bindFlags=Constant, heapMemoryLevel=Device, hostMemoryAccess=Write
            //! This buffer pool can be used for constant buffers.
            virtual RHI::Ptr<RHI::BufferPool>& GetConstantBufferDevicePool() = 0;

            //! For convenience, this provides an ImagePool with bindFlags=ShaderRead.
            //! This ImagePool can be used for images that are bound as input to a shader.
            virtual RHI::Ptr<RHI::ImagePool>& GetImagePool() = 0;
        };

        //! Singleton system interface that is used to query the DynamicDraw interface for a scene (this interfaces to DynamicDrawSystemComponent)
        class DynamicDrawSystemInterface
        {
        public:
            AZ_RTTI(DynamicDrawSystemInterface, "{10AF7B4D-8975-4BE3-8C7E-8609B899C728}");

            DynamicDrawSystemInterface() = default;
            virtual ~DynamicDrawSystemInterface() = default;
            
            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(DynamicDrawSystemInterface);
            
            virtual DynamicDrawInterface* GetDynamicDrawInterface(Scene* scene) = 0;
            virtual void RegisterDynamicDrawForScene(RPI::DynamicDrawInterface* dd, RPI::Scene* scene) = 0;
            virtual void UnregisterDynamicDrawForScene(RPI::Scene* scene) = 0;
        };

        //! Global function to query the DynamicDrawInterface.
        //! For convenience, you can DynamicDrawSystemInterface class and just call this to query the DynamicDrawInterface
        inline DynamicDrawInterface* GetDynamicDraw(Scene* scene = nullptr)
        {
            DynamicDrawSystemInterface* sys = Interface<DynamicDrawSystemInterface>::Get();
            if (sys)
            {
                return sys->GetDynamicDrawInterface(scene);
            }
            else
            {
                AZ_Assert(sys, "No DynamicDrawInterface was registered for this scene! Perhaps the DynamicDrawFeatureProcessor was not registered with the scene?");
                return nullptr;
            }
        }

    } // namespace RPI
} // namespace AZ
