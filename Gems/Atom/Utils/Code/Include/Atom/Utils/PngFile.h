/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/std/functional.h>

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/Format.h>

namespace AZ
{
    //! This is a light wrapper class for libpng, to load and save .png files.
    //! Functionality is limited, feel free to add more features as needed.
    class PngImage
    {
    public:
        using ErrorHandler = AZStd::function<void(const char*)>;

        struct LoadSettings
        {
            ErrorHandler m_errorHandler = {}; //!< optional callback function describing any errors that are encountered
            bool m_stripAlpha = false;        //!< the alpha channel will be skipped, loading an RGBA image as RGB
        };

        struct SaveSettings
        {
            ErrorHandler m_errorHandler = {}; //!< optional callback function describing any errors that are encountered
            bool m_stripAlpha = false;        //!< the alpha channel will be skipped, saving an RGBA buffer as RGB
            int m_compressionLevel = 6;       //!< this is the zlib compression level. See png_set_compression_level in png.h
        };

        // To keep things simple for now we limit all images to RGB and RGBA, 8 bits per channel.
        enum class Format
        {
            Unknown,
            RGB,
            RGBA
        };

        //! @return the loaded PngImage or an invalid PngImage if there was an error.
        static PngImage Load(const char* path, LoadSettings loadSettings = {});

        //! Create a PngImage from an RHI data buffer.
        //! @param size the dimensions of the image (m_depth is not used, assumed to be 1)
        //! @param format indicates the pixel format represented by @data. Only a limited set of formats are supported, see implementation.
        //! @param data the buffer of image data. The size of the buffer must match the @size and @format parameters.
        //! @return the created PngImage or an invalid PngImage if there was an error.
        static PngImage Create(const RHI::Size& size, RHI::Format format, AZStd::array_view<uint8_t> data);
        static PngImage Create(const RHI::Size& size, RHI::Format format, AZStd::vector<uint8_t>&& data);

        PngImage() = default;
        AZ_DEFAULT_MOVE(PngImage)

        //! @return true if the save operation was successful
        bool Save(const char* path, SaveSettings saveSettings = {});

        bool IsValid() const;
        operator bool() const { return IsValid(); }

        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }

        Format GetBufferFormat() const { return m_bufferFormat; }
        const AZStd::vector<uint8_t>& GetBuffer() const { return m_buffer; }

        //! Returns a r-value reference that can be moved. This will invalidate the PngImage.
        AZStd::vector<uint8_t>&& TakeBuffer();

    private:
        AZ_DEFAULT_COPY(PngImage)
            
        static const int HeaderSize = 8;

        static void DefaultErrorHandler(const char* message);

        // See png_get_IHDR in http://www.libpng.org/pub/png/libpng-1.4.0-manual.pdf...
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        int32_t m_bitDepth = 0;
        int32_t m_colorType = 0;

        Format m_bufferFormat = Format::Unknown;
        AZStd::vector<uint8_t> m_buffer;
    };
} // namespace AZ
