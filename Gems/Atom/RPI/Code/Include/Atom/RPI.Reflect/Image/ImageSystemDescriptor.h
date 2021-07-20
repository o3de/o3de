/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        struct ImageSystemDescriptor final
        {
            AZ_TYPE_INFO(RPI::ImageSystemDescriptor, "{319D14F6-F7F2-487A-AA6B-5800E328C79B}");
            static void Reflect(AZ::ReflectContext* context);

            uint64_t m_systemStreamingImagePoolSize = 128 * 1024 * 1024;
            uint64_t m_systemAttachmentImagePoolSize = 512 * 1024 * 1024;
            uint64_t m_assetStreamingImagePoolSize = 2u * 1024u * 1024u * 1024u;
        };
    } // namespace RPI
} // namespace AZ
