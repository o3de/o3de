/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DispatchItem.h>
#include <AtomCore/Instance/InstanceData.h>

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

            //! Holds and manages an RHI DispatchItem for a specific skinned mesh, and the resources that are
            //!  needed to build and maintain it.
            class HairDispatchItem
                : public Data::InstanceData
            {
            public:
                AZ_CLASS_ALLOCATOR(HairDispatchItem, AZ::SystemAllocator, 0);

                HairDispatchItem() = default;

                //! One dispatch item per hair object per computer pass.
                //! The amount of dispatches depends on the amount of vertices required to be created

                ~HairDispatchItem();

                void InitSkinningDispatch(
                    RPI::Shader* shader,
                    RPI::ShaderResourceGroup* hairGenerationSrg,
                    RPI::ShaderResourceGroup* hairSimSrg,
                    uint32_t elementsAmount
                );

                RHI::DispatchItem* GetDispatchItem() { return &m_dispatchItem;  }

            private:
                RHI::DispatchItem m_dispatchItem;
                RPI::Shader* m_shader;
            };

        } // namespace Hair
    } // namespace Render
} // namespace AZ
