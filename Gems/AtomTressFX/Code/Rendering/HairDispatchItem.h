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

#include <Atom/RHI/DispatchItem.h>
#include <AtomCore/Instance/InstanceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
        class PipelineState;
    }

    namespace RPI
    {
        class Buffer;
        class ModelLod;
        class Shader;
        class ShaderResourceGroup;
    }

    namespace Render
    {
        namespace Hair
        {
            class HairSkinningComputePass;
            class HairRenderObject;

            enum class DispatchLevel
            {
                DISPATCHLEVEL_VERTEX,
                DISPATCHLEVEL_STRAND
            };

            //! Holds and manages an RHI DispatchItem for a specific skinned mesh, and the resources that are needed to build and maintain it.
            //! Ref taken from SkinnedMeshDispatchItem
            class HairDispatchItem
                : public Data::InstanceData
// [To Do] For now keep it simple, but in the future connect to shader notification to re-initialize
//                : private RPI::ShaderReloadNotificationBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR(HairDispatchItem, AZ::SystemAllocator, 0);

                HairDispatchItem() = default;
                //! Create one or more dispatch items per hair component of actor instance
                //! Amount of dispatches depends on the amount of vertices required to be created

                ~HairDispatchItem();

                void InitSkinningDispatch(
                    RPI::Shader* shader,
                    RPI::ShaderResourceGroup* hairGenerationSrg,
                    RPI::ShaderResourceGroup* hairSimSrg,
                    uint32_t elementsAmount
                );

                RHI::DispatchItem* GetDispatchItem() { return &m_dispatchItem;  }


//                void OnShaderReinitialized() override;

                RHI::DispatchItem m_dispatchItem;

                RPI::Shader* m_shader;
            };

        } // namespace Hair
    } // namespace Render
} // namespace AZ
