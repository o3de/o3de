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
