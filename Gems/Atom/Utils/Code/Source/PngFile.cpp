/*  
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/PngFile.h>
#include <png.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ
{
    namespace Utils
    {
        namespace
        {
            void PngImage_user_error_fn(png_structp png_ptr, png_const_charp error_msg)
            {
                PngFile::ErrorHandler* errorHandler = reinterpret_cast<PngFile::ErrorHandler*>(png_get_error_ptr(png_ptr));
                (*errorHandler)(error_msg);
            }

            void PngImage_user_warning_fn(png_structp /*png_ptr*/, [[maybe_unused]] png_const_charp warning_msg)
            {
                AZ_Warning("PngFile", false, "%s", warning_msg);
            }
        }

        PngFile PngFile::Create(const RHI::Size& size, RHI::Format format, AZStd::span<const uint8_t> data, ErrorHandler errorHandler)
        {
            return Create(size, format, AZStd::vector<uint8_t>{data.begin(), data.end()}, errorHandler);
        }

        PngFile PngFile::Create(const RHI::Size& size, RHI::Format format, AZStd::vector<uint8_t>&& data, ErrorHandler errorHandler)
        {
            if (!errorHandler)
            {
                errorHandler = [](const char* message) { DefaultErrorHandler(message); };
            }

            PngFile image;

            if (RHI::Format::R8G8B8A8_UNORM == format)
            {
                if (size.m_width * size.m_height * 4 == data.size())
                {
                    image.m_width = size.m_width;
                    image.m_height = size.m_height;
                    image.m_bitDepth = 8;
                    image.m_bufferFormat = PngFile::Format::RGBA;
                    image.m_buffer = AZStd::move(data);
                }
                else
                {
                    errorHandler("Invalid arguments. Buffer size does not match the image dimensions.");
                }
            }
            else
            {
                errorHandler(AZStd::string::format("Cannot create PngFile with unsupported format %s", AZ::RHI::ToString(format)).c_str());
            }

            return image;
        }

        PngFile PngFile::Load(const char* path, LoadSettings loadSettings)
        {
            if (!loadSettings.m_errorHandler)
            {
                loadSettings.m_errorHandler = [path](const char* message)
                {
                    DefaultErrorHandler(AZStd::string::format("Could not load file '%s'. %s", path, message).c_str());
                };
            }

            AZ::IO::SystemFileStream fileLoadStream(path, AZ::IO::OpenMode::ModeRead);
            if (!fileLoadStream.IsOpen())
            {
                loadSettings.m_errorHandler("Cannot open file.");
                return {};
            }

            auto pngFile = LoadInternal(fileLoadStream, loadSettings);
            return pngFile;
        }

        PngFile PngFile::LoadFromBuffer(AZStd::span<const uint8_t> data, LoadSettings loadSettings)
        {
            if (!loadSettings.m_errorHandler)
            {
                loadSettings.m_errorHandler = [](const char* message)
                {
                    DefaultErrorHandler(AZStd::string::format("Could not load Png from buffer. %s", message).c_str());
                };
            }

            if (data.empty())
            {
                loadSettings.m_errorHandler("Buffer is empty.");
                return {};
            }

            AZ::IO::MemoryStream memStream(data.data(), data.size());

            return LoadInternal(memStream, loadSettings);
        }

        PngFile PngFile::LoadInternal(AZ::IO::GenericStream& dataStream, LoadSettings loadSettings)
        {
            // For documentation of this code, see http://www.libpng.org/pub/png/libpng-1.4.0-manual.pdf chapter 3

            // Verify that we've passed in a valid data stream.
            if (!dataStream.IsOpen()  || !dataStream.CanRead())
            {
                loadSettings.m_errorHandler("Data stream isn't valid.");
                return {};
            }

            png_byte header[HeaderSize] = {};
            size_t headerBytesRead = 0;

            // This is the one I/O read that occurs outside of the png library, so either read from the file or the buffer and
            // verify the results.
            headerBytesRead = dataStream.Read(HeaderSize, header);
            if (headerBytesRead != HeaderSize)
            {
                loadSettings.m_errorHandler("Invalid png header.");
                return {};
            }

            bool isPng = !png_sig_cmp(header, 0, HeaderSize);
            if (!isPng)
            {
                loadSettings.m_errorHandler("Invalid png header.");
                return {};
            }

            png_voidp user_error_ptr = &loadSettings.m_errorHandler;
            png_error_ptr user_error_fn = PngImage_user_error_fn;
            png_error_ptr user_warning_fn = PngImage_user_warning_fn;

            png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, user_error_ptr, user_error_fn, user_warning_fn);
            if (!png_ptr)
            {
                loadSettings.m_errorHandler("png_create_read_struct failed.");
                return {};
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr)
            {
                png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
                loadSettings.m_errorHandler("png_create_info_struct failed.");
                return {};
            }

            png_infop end_info = png_create_info_struct(png_ptr);
            if (!end_info)
            {
                png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
                loadSettings.m_errorHandler("png_create_info_struct failed.");
                return {};
            }

// Disables "interaction between '_setjmp' and C++ object destruction is non-portable".
// See https://docs.microsoft.com/en-us/cpp/preprocessor/warning?view=msvc-160
AZ_PUSH_DISABLE_WARNING(4611, "-Wunknown-warning-option") 
            if (setjmp(png_jmpbuf(png_ptr)))
            {
                png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
                // We don't report an error message here because the user_error_fn should have done that already.
                return {};
            }
AZ_POP_DISABLE_WARNING

            auto genericStreamReader = [](png_structp pngPtr, png_bytep data, png_size_t length)
            {
                // Here we get our IO pointer back from the read struct.
                // This should be the GenericStream pointer we passed to the png_set_read_fn() function.
                png_voidp ioPtr = png_get_io_ptr(pngPtr);

                if (ioPtr != nullptr)
                {
                    AZ::IO::GenericStream* genericStream = static_cast<AZ::IO::GenericStream*>(ioPtr);
                    genericStream->Read(length, data);
                }
            };

            png_set_read_fn(png_ptr, &dataStream, genericStreamReader);

            png_set_sig_bytes(png_ptr, HeaderSize);

            png_set_keep_unknown_chunks(png_ptr, PNG_HANDLE_CHUNK_NEVER, NULL, 0);

            // To keep things simple for now we limit all images to RGB and RGBA, 8 bits per channel
            int png_transforms = PNG_TRANSFORM_PACKING | // Expand 1, 2 and 4-bit samples to bytes
                PNG_TRANSFORM_STRIP_16 | // Reduce 16 bit samples to 8 bits
                PNG_TRANSFORM_GRAY_TO_RGB;

            if (loadSettings.m_stripAlpha)
            {
                png_transforms |= PNG_TRANSFORM_STRIP_ALPHA;
            }

            png_read_png(png_ptr, info_ptr, png_transforms, NULL);

            // Note that libpng will allocate row_pointers for us. If we want to manage the memory ourselves, we need to call png_set_rows.
            // In that case we would have to use the low level interface: png_read_info, png_read_image, and png_read_end.
            png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);

            PngFile pngFile;

            int colorType = 0;

            png_get_IHDR(png_ptr, info_ptr, &pngFile.m_width, &pngFile.m_height, &pngFile.m_bitDepth, &colorType, NULL, NULL, NULL);

            uint32_t bytesPerPixel = 0;

            switch (colorType)
            {
            case PNG_COLOR_TYPE_RGB:
                pngFile.m_bufferFormat = PngFile::Format::RGB;
                bytesPerPixel = 3;
                break;
            case PNG_COLOR_TYPE_RGBA:
                pngFile.m_bufferFormat = PngFile::Format::RGBA;
                bytesPerPixel = 4;
                break;
            case PNG_COLOR_TYPE_PALETTE:
                // Handles cases where the image uses 1, 2, or 4 bit samples.
                // Note bytesPerPixel is 3 because we use PNG_TRANSFORM_PACKING
                pngFile.m_bufferFormat = PngFile::Format::RGB;
                bytesPerPixel = 3;
                break;
            default:
                AZ_Assert(false, "The png transforms should have ensured a pixel format of RGB or RGBA, 8 bits per channel");
                png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
                loadSettings.m_errorHandler("Unsupported pixel format.");
                return {};
            }

            // In the future we could use the low-level interface to avoid copying the image (and provide progress callbacks)
            pngFile.m_buffer.set_capacity(pngFile.m_width * pngFile.m_height * bytesPerPixel);
            for (uint32_t rowIndex = 0; rowIndex < pngFile.m_height; ++rowIndex)
            {
                png_bytep row = row_pointers[rowIndex];
                pngFile.m_buffer.insert(pngFile.m_buffer.end(), row, row + (pngFile.m_width * bytesPerPixel));
            }

            png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
            return pngFile;
        }

        bool PngFile::Save(const char* path, SaveSettings saveSettings)
        {
            if (!saveSettings.m_errorHandler)
            {
                saveSettings.m_errorHandler = [path](const char* message) { DefaultErrorHandler(AZStd::string::format("Could not save file '%s'. %s", path, message).c_str()); };
            }

            if (!IsValid())
            {
                saveSettings.m_errorHandler("This PngFile is invalid.");
                return false;
            }

            // For documentation of this code, see http://www.libpng.org/pub/png/libpng-1.4.0-manual.pdf chapter 4

            FILE* fp = NULL;
            azfopen(&fp, path, "wb"); // return type differs across platforms so can't do inside if
            if (!fp)
            {
                saveSettings.m_errorHandler("Cannot open file.");
                return false;
            }

            png_voidp user_error_ptr = &saveSettings.m_errorHandler;
            png_error_ptr user_error_fn = PngImage_user_error_fn;
            png_error_ptr user_warning_fn = PngImage_user_warning_fn;

            png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, user_error_ptr, user_error_fn, user_warning_fn);
            if (!png_ptr)
            {
                fclose(fp);
                saveSettings.m_errorHandler("png_create_write_struct failed.");
                return false;
            }

            png_infop info_ptr = png_create_info_struct(png_ptr);
            if (!info_ptr)
            {
                png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
                fclose(fp);
                saveSettings.m_errorHandler("png_destroy_write_struct failed.");
                return false;
            }

AZ_PUSH_DISABLE_WARNING(4611, "-Wunknown-warning-option") // Disables "interaction between '_setjmp' and C++ object destruction is non-portable". See https://docs.microsoft.com/en-us/cpp/preprocessor/warning?view=msvc-160
            if (setjmp(png_jmpbuf(png_ptr)))
            {
                png_destroy_write_struct(&png_ptr, &info_ptr);
                fclose(fp);
                // We don't report an error message here because the user_error_fn should have done that already.
                return false;
            }
AZ_POP_DISABLE_WARNING

            png_init_io(png_ptr, fp);

            int colorType = 0;
            if (saveSettings.m_stripAlpha || m_bufferFormat == PngFile::Format::RGB)
            {
                colorType = PNG_COLOR_TYPE_RGB;
            }
            else
            {
                colorType = PNG_COLOR_TYPE_RGBA;
            }
            
            int transforms = PNG_TRANSFORM_IDENTITY;
            if (saveSettings.m_stripAlpha && m_bufferFormat == PngFile::Format::RGBA)
            {
                transforms |= PNG_TRANSFORM_STRIP_FILLER_AFTER;
            }

            png_set_IHDR(png_ptr, info_ptr, m_width, m_height, m_bitDepth, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

            png_set_compression_level(png_ptr, saveSettings.m_compressionLevel);

            const uint32_t bytesPerPixel = (m_bufferFormat == PngFile::Format::RGB) ? 3 : 4;

            AZStd::vector<uint8_t*> rows;
            rows.reserve(m_height);
            for (uint32_t i = 0; i < m_height; ++i)
            {
                rows.push_back(m_buffer.begin() + m_width * bytesPerPixel * i);
            }

            png_set_rows(png_ptr, info_ptr, rows.begin());
            
            png_write_png(png_ptr, info_ptr, transforms, NULL);

            png_destroy_write_struct(&png_ptr, &info_ptr);

            fclose(fp);

            return true;
        }

        void PngFile::DefaultErrorHandler([[maybe_unused]] const char* message)
        {
            AZ_Error("PngFile", false, "%s", message);
        }

        bool PngFile::IsValid() const
        {
            return
                !m_buffer.empty() &&
                m_width > 0 &&
                m_height > 0 &&
                m_bitDepth > 0;
        }

        AZStd::vector<uint8_t>&& PngFile::TakeBuffer()
        {
            return AZStd::move(m_buffer);
        }

    } // namespace Utils
}// namespace AZ

