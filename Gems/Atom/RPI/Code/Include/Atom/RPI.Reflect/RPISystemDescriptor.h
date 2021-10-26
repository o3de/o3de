/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/GpuQuerySystemDescriptor.h>
#include <Atom/RPI.Reflect/Image/ImageSystemDescriptor.h>

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

            //! The asset cache relative path of the only common shader asset for the RPI system that is used
            //! as means to load the layout for scene srg and view srg. This is used to create any RPI::Scene.
            AZStd::string m_commonSrgsShaderAssetPath = "shaders/sceneandviewsrgs.azshader";

            ImageSystemDescriptor m_imageSystemDescriptor;
            GpuQuerySystemDescriptor m_gpuQuerySystemDescriptor;
            DynamicDrawSystemDescriptor m_dynamicDrawSystemDescriptor;
        };
    } // namespace RPI
} // namespace AZ
