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

#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI.Reflect/ResourcePoolDescriptor.h>

namespace AZ
{
    namespace RHI
    {
        enum class ShaderResourceGroupUsage : uint32_t
        {
            /**
             * The resource set is persistent across frames and will be updated rarely.
             */
            Persistent = 0,

            /**
             * The resource group is optimized for once-per-frame updates. Its contents become invalid
             * after the current frame has completed.
             */
            Transient
        };

        class ShaderResourceGroupPoolDescriptor
            : public ResourcePoolDescriptor
        {
        public:
            virtual ~ShaderResourceGroupPoolDescriptor() = default;
            AZ_RTTI(ShaderResourceGroupPoolDescriptor, "{7074E7D1-B98D-48C7-8622-D6635CAEFBB4}");
            static void Reflect(AZ::ReflectContext* context);

            ShaderResourceGroupPoolDescriptor() = default;

            /// [Not Serialized] The resource group layout shared by all resource groups in the pool.
            RHI::ConstPtr<ShaderResourceGroupLayout> m_layout;

            /// The usage type used to update resource groups.
            ShaderResourceGroupUsage m_usage = ShaderResourceGroupUsage::Persistent;
        };
    }
}