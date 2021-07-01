/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/Origin.h>

#include <Atom/RPI.Reflect/Pass/PassData.h>

namespace AZ
{
    namespace RPI
    {
        //! Custom data for the CopyPass. Should be specified in the PassRequest.
        struct CopyPassData
            : public PassData
        {
            AZ_RTTI(CopyPassData, "{A0E4DBBF-2FE7-4027-B6B1-4F15AEB03B04}", PassData);
            AZ_CLASS_ALLOCATOR(CopyPassData, SystemAllocator, 0);

            CopyPassData() = default;
            virtual ~CopyPassData() = default;

            static void Reflect(ReflectContext* context)
            {
                if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
                {
                    serializeContext->Class<CopyPassData, PassData>()
                        ->Version(0)
                        ->Field("BufferSize", &CopyPassData::m_bufferSize)
                        ->Field("BufferSourceOffset", &CopyPassData::m_bufferSourceOffset)
                        ->Field("BufferSourceBytesPerRow", &CopyPassData::m_bufferSourceBytesPerRow)
                        ->Field("BufferSourceBytesPerImage", &CopyPassData::m_bufferSourceBytesPerImage)
                        ->Field("BufferDestinationOffset", &CopyPassData::m_bufferDestinationOffset)
                        ->Field("BufferDestinationBytesPerRow", &CopyPassData::m_bufferDestinationBytesPerRow)
                        ->Field("BufferDestinationBytesPerImage", &CopyPassData::m_bufferDestinationBytesPerImage)
                        ->Field("ImageSourceSize", &CopyPassData::m_sourceSize)
                        ->Field("ImageSourceSubresource", &CopyPassData::m_imageSourceSubresource)
                        ->Field("ImageSourceOrigin", &CopyPassData::m_imageSourceOrigin)
                        ->Field("ImageDestinationSubresource", &CopyPassData::m_imageDestinationSubresource)
                        ->Field("ImageDestinationOrigin", &CopyPassData::m_imageDestinationOrigin)
                        ->Field("CloneInput", &CopyPassData::m_cloneInput)
                        ;
                }
            }

            // Size
            uint32_t m_bufferSize = 0;
            RHI::Size m_sourceSize;

            // Buffer Source
            uint32_t m_bufferSourceOffset = 0;
            uint32_t m_bufferSourceBytesPerRow = 0;
            uint32_t m_bufferSourceBytesPerImage = 0;

            // Buffer Destination
            uint32_t m_bufferDestinationOffset = 0;
            uint32_t m_bufferDestinationBytesPerRow = 0;
            uint32_t m_bufferDestinationBytesPerImage = 0;

            // Image Source
            RHI::ImageSubresource m_imageSourceSubresource;
            RHI::Origin m_imageSourceOrigin;

            // Image Destination
            RHI::ImageSubresource m_imageDestinationSubresource;
            RHI::Origin m_imageDestinationOrigin;

            // If set to true, pass will automatically create a transient output attachment based on input
            // If false, the output target of the copy will need to be specified
            bool m_cloneInput = true;
        };
    } // namespace RPI
} // namespace AZ

