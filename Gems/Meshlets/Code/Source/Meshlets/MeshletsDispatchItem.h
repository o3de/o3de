/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RHI/DispatchItem.h>

namespace AZ
{
    namespace RHI
    {
        class PipelineState;
    }

    namespace RPI
    {
        class Shader;
        class ShaderResourceGroup;
    }

    namespace Meshlets
    {
        class MeshletsDispatchItem
            : public Data::InstanceData
        {
        public:
            AZ_CLASS_ALLOCATOR(MeshletsDispatchItem, AZ::SystemAllocator);

            MeshletsDispatchItem() = default;

            ~MeshletsDispatchItem();

            void InitDispatch(
                RPI::Shader* shader,
                Data::Instance<RPI::ShaderResourceGroup> meshletsDataSrg,
                uint32_t meshletsAmount // Amount of meshlets
            );

            void SetPipelineState(RPI::Shader* shader);

            RHI::DispatchItem* GetDispatchItem() { return m_shader ? &m_dispatchItem : nullptr; }
            Data::Instance<RPI::ShaderResourceGroup> GetMeshletDataSrg() { return m_meshletsDataSrg;  }

        private:
            RHI::DispatchItem m_dispatchItem{RHI::MultiDevice::AllDevices};
            Data::Instance<RPI::ShaderResourceGroup> m_meshletsDataSrg;
            RPI::Shader* m_shader;
        };
    } // namespace Meshlets
} // namespace AZ
