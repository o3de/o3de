/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawInterface.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>
#include <Atom/RPI.Reflect/RPISystemDescriptor.h>

namespace AZ
{
    namespace RPI
    {
        //! DynamicDrawSystem is the dynamic draw system in RPI system which implements DynamicDrawInterface
        //! It contains the dynamic buffer allocator which is used to allocate dynamic buffers.
        //! It also responses to submit all the dynamic draw data to the rendering passes.
        class ATOM_RPI_PUBLIC_API DynamicDrawSystem final : public DynamicDrawInterface
        {
            friend class RPISystem;
        public:
            AZ_TYPE_INFO(DynamicDrawSystem, "{49D23FE9-352F-4AB0-B9BB-A3B75592023B}");

            // Dynamic draw system initialization and shutdown
            void Init(const DynamicDrawSystemDescriptor& descriptor);
            void Shutdown();

            // DynamicDrawInterface overrides...
            RHI::Ptr<DynamicDrawContext> CreateDynamicDrawContext() override;
            RHI::Ptr<DynamicBuffer> GetDynamicBuffer(uint32_t size, uint32_t alignment) override;
            void DrawGeometry(Data::Instance<Material> material, const GeometryData& geometry, ScenePtr scene) override;
            void AddDrawPacket(Scene* scene, AZStd::unique_ptr<const RHI::DrawPacket> drawPacket) override;
            void AddDrawPacket(Scene* scene, ConstPtr<RHI::DrawPacket> drawPacket) override;
            AZStd::vector<RHI::DrawListView> GetDrawListsForPass(const RasterPass* pass) override;

            // Submit draw data for selected scene and pipeline
            void SubmitDrawData(Scene* scene, AZStd::vector<ViewPtr> views);

        protected:

            void FrameEnd();

        private:
            AZStd::mutex m_mutexBufferAlloc;
            AZStd::unique_ptr<DynamicBufferAllocator> m_bufferAlloc;

            AZStd::mutex m_mutexDrawContext;
            AZStd::list<RHI::Ptr<DynamicDrawContext>> m_dynamicDrawContexts;

            AZStd::mutex m_mutexDrawPackets;
            AZStd::map<Scene*, AZStd::vector<ConstPtr<const RHI::DrawPacket>>> m_drawPackets;
        };
    }
}
