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

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicBuffer.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawContext.h>
#include <Atom/RPI.Public/Material/Material.h>


namespace AZ
{
    namespace RPI
    {
        class Scene;
        class RenderPipeline;
        class Pass;

        //! Geometry data which uses triangle lists 
        struct GeometryData
        {
            const void* m_vertexData = nullptr;
            uint32_t m_vertexDataSize = 0;
            uint32_t m_vertexCount = 0;
            const void* m_indexData = nullptr;
            uint32_t m_indexDataSize = 0;
            uint32_t m_indexCount = 0;
        };

        //! Interface of dynamic draw system which provide access to system dynamic buffer
        //! and some draw functions
        // DynamicDrawSystemInterface2 is newer version of dynamic draw system which is to replace current DynamicDrawSystemInterface
        // [GFX TODO][ATOM-13183] Remove the old DynamicDrawSystemInterface from both RPI and CommonFeature Gem
        class DynamicDrawSystemInterface2
        {
        public:
            AZ_RTTI(DynamicDrawSystemInterface2, "{0883B8A7-9D25-418A-8F98-B33C52FF21CC}");

            DynamicDrawSystemInterface2() = default;
            virtual ~DynamicDrawSystemInterface2() = default;

            static DynamicDrawSystemInterface2* Get();

            AZ_DISABLE_COPY_MOVE(DynamicDrawSystemInterface2);

            //! Create a DynamicDrawContext for specified scene (and render pipeline).
            //! Draw calls which are made to this DynamicDrawContext will only be submitted for this scene.
            //! The created DynamicDrawContext is managed by dynamic draw system.
            virtual RHI::Ptr<DynamicDrawContext> CreateDynamicDrawContext(Scene* scene) = 0;

            //! Create a DynamicDrawContext for specified pass
            virtual RHI::Ptr<DynamicDrawContext> CreateDynamicDrawContext(Pass* pass = nullptr) = 0;

            //! Get a DynamicBuffer from DynamicDrawSystem.
            //! The returned buffer will be invalidated every time the RPISystem's RenderTick is called
            virtual RHI::Ptr<DynamicBuffer> GetDynamicBuffer(uint32_t size, uint32_t alignment = 1) = 0;

            //! Draw a geometry to a scene with a given material
            virtual void DrawGeometry(Data::Instance<Material> material, const GeometryData& geometry, ScenePtr scene) = 0;

        };
    }
}
