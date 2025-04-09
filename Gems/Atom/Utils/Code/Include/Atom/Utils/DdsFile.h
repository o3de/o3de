/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Size.h>

namespace AZ
{
    // Forward Declarations
    namespace IO
    {
        class GenericStream;
    }

    // Dds file writing capabilty without any platform specific requirements. Details of the flags used in DDS are 
    // encapsulated in the cpp. This class doesn't handle generating or converting any data, so data should be
    // passed to it already in the format for writing out. This class only deals with setting up the dds header 
    // structures for supported formats.
    
    class DdsFile final
    {
    public:

        struct DdsFileData
        {
            RHI::Size                     m_size;
            RHI::Format                   m_format;
            const AZStd::vector<uint8_t>* m_buffer;
            bool                          m_isCubemap = false;
            uint32_t                      m_mipLevels = 1;
        };

        struct DdsFailure
        {
            enum class Code 
            {
                OpenFileFailed,
                WriteFailed,
            };

            Code m_code;
            AZStd::string m_message;
        };

        DdsFile();
        ~DdsFile();

        void SetSize(RHI::Size size);
        RHI::Size GetSize();

        void SetFormat(RHI::Format format);
        RHI::Format GetFormat();

        void SetAsCubemap();

        // Called automatically if size.depth > 1 in SetSize().
        void SetAsVolumeTexture();
        
        void SetMipLevels(uint32_t mipLevels);
        uint32_t GetMipLevels();

        AZ::Outcome<void, DdsFile::DdsFailure> WriteHeaderToStream(AZ::IO::GenericStream& stream);

        static AZ::Outcome<void, DdsFailure> WriteFile(const AZStd::string& filePath, const DdsFileData& ddsFiledata);
        static bool DoesSupportFormat(RHI::Format format);

    private:

        struct DdsPixelFormat
        {
            const uint32_t m_size = 32; // Must be set to 32, the size of the struct.
            uint32_t m_flags = 0;
            uint32_t m_fourCC = 0;
            uint32_t m_rgbBitCount = 0;
            uint32_t m_rBitMask = 0;
            uint32_t m_gBitMask = 0;
            uint32_t m_bBitMask = 0;
            uint32_t m_aBitMask = 0;

            DdsPixelFormat();
        };

        // Standard dds header
        struct DdsHeader
        {
            const uint32_t m_size = 124; // Must be set to 124, the size of the struct.
            uint32_t m_flags = 0;
            uint32_t m_height = 0;
            uint32_t m_width = 0;
            uint32_t m_pitchOrLinearSize = 0;
            uint32_t m_depth = 1;
            uint32_t m_mipMapCount = 1;
            const uint32_t m_reserved1[11] = {0};
            DdsPixelFormat m_pixelFormat;
            uint32_t m_caps = 0;
            uint32_t m_caps_2 = 0;
            const uint32_t m_caps_3 = 0; // unused
            const uint32_t m_caps_4 = 0; // unused
            const uint32_t m_reserved2 = 0;

            DdsHeader();
        };

        // DX10 header extension
        struct DdsHeaderDxt10
        {
            uint32_t m_dxgiFormat = 0;
            uint32_t m_resourceDimension = 0;
            uint32_t m_miscFlag = 0;
            uint32_t m_arraySize = 0;
            const uint32_t m_reserved = 0;
        };

        void ResolvePitch();

        uint32_t m_magic;
        DdsHeader m_header;
        DdsHeaderDxt10 m_headerDx10;

        RHI::Format m_externalFormat = RHI::Format::Unknown;
    };
} // namespace AZ
