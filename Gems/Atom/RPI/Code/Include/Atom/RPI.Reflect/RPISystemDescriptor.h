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

#include <Atom/RPI.Reflect/GpuQuerySystemDescriptor.h>
#include <Atom/RPI.Reflect/Image/ImageSystemDescriptor.h>
#include <Atom/RHI.Reflect/RHISystemDescriptor.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>

namespace AZ
{
    namespace RPI
    {

        //! A descriptor used to initialize dynamic draw system
        struct DynamicDrawSystemDescriptor
        {
            AZ_TYPE_INFO(DynamicDrawSystemDescriptor, "{BC1F1C0A-4A87-4D30-A331-BE8358A23F6D}");

            //! The maxinum size of pool which is used to allocate dynamic buffers for dynamic draw system
            uint32_t m_dynamicBufferPoolSize = 3 * 16 * 1024 * 1024;
        };

        struct RPISystemDescriptor final
        {
            AZ_TYPE_INFO(RPISystemDescriptor, "{96DAC3DA-40D4-4C03-8D6A-3181E843262A}");
            static void Reflect(AZ::ReflectContext* context);

            RHI::RHISystemDescriptor m_rhiSystemDescriptor;

            //! The path of the only one scene srg asset for the RPI system. This is used to create any RPI::Scene.
            AZStd::string m_sceneSrgAssetPath = "shaderlib/scenesrg_scenesrg.azsrg";

            //! The path of the only one view srg asset for the RPI system. This is used to create any RPI::View.
            AZStd::string m_viewSrgAssetPath = "shaderlib/viewsrg_viewsrg.azsrg";

            ImageSystemDescriptor m_imageSystemDescriptor;
            GpuQuerySystemDescriptor m_gpuQuerySystemDescriptor;
            DynamicDrawSystemDescriptor m_dynamicDrawSystemDescriptor;
        };
    } // namespace RPI
} // namespace AZ
