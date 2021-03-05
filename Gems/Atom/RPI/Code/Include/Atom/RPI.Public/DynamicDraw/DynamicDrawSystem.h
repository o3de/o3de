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

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystemInterface2.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBufferAllocator.h>
#include <Atom/RPI.Reflect/RPISystemDescriptor.h>


namespace AZ
{
    namespace RPI
    {
        //! DynamicDrawSystem is the dynamic draw system in RPI system which implements DynamicDrawSystemInterface2
        //! It contains the dynamic buffer allocator which is used to allocate dynamic buffers.
        //! It also responses to submit all the dynamic draw data to the rendering passes.
        class DynamicDrawSystem final
            : public DynamicDrawSystemInterface2
        {
            friend class RPISystem;
        public:
            AZ_TYPE_INFO(DynamicDrawSystem, "{49D23FE9-352F-4AB0-B9BB-A3B75592023B}");

            // Dynamic draw system initialization and shutdown
            void Init(const DynamicDrawSystemDescriptor& descriptor);
            void Shutdown();

            // DynamicDrawSystemInterface2 overrides...
            RHI::Ptr<DynamicDrawContext> CreateDynamicDrawContext(Scene* scene) override;
            RHI::Ptr<DynamicDrawContext> CreateDynamicDrawContext(Pass* pass) override;
            RHI::Ptr<DynamicBuffer> GetDynamicBuffer(uint32_t size, uint32_t alignment = 1) override;
            void DrawGeometry(Data::Instance<Material> material, const GeometryData& geometry, ScenePtr scene) override;

            // Submit draw data for selected scene and pipeline
            void SubmitDrawData(Scene* scene, AZStd::vector<ViewPtr> views);
        protected:

            void FrameEnd();

        private:
            AZStd::unique_ptr<DynamicBufferAllocator> m_bufferAlloc;
            AZStd::list<RHI::Ptr<DynamicDrawContext>> m_dynamicDrawContexts;
        };
    }
}
