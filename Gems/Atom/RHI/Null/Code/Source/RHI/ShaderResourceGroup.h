/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceShaderResourceGroup.h>

namespace AZ
{
    namespace Null
    {
        class ShaderResourceGroup final
            : public RHI::DeviceShaderResourceGroup
        {
            using Base = RHI::DeviceShaderResourceGroup;
        public:
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator);

            static RHI::Ptr<ShaderResourceGroup> Create();

        private:
            ShaderResourceGroup() = default;
        };
    }
}
