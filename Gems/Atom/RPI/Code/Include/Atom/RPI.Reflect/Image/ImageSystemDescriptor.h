/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        struct ATOM_RPI_REFLECT_API ImageSystemDescriptor final
        {
            AZ_TYPE_INFO(RPI::ImageSystemDescriptor, "{319D14F6-F7F2-487A-AA6B-5800E328C79B}");
            static void Reflect(AZ::ReflectContext* context);

            //! The maximum size of the image pool used for streaming images.
            //! Check ImageSystemInterface::GetSystemStreamingPool() for detail of this image pool
            uint64_t m_systemStreamingImagePoolSize = 0;
            
            //! The mipmap bias applied to streamable images created from the system streaming image pool
            int16_t m_systemStreamingImagePoolMipBias = 0;

            //! The maximum size of the image pool used for system attachments images.
            //! Check ImageSystemInterface::GetSystemAttachmentPool() for detail of this image pool
            uint64_t m_systemAttachmentImagePoolSize = 512 * 1024 * 1024;

        };
    } // namespace RPI
} // namespace AZ
