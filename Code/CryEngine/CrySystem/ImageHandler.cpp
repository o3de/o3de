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
#include "CrySystem_precompiled.h"
#include <AzCore/IO/SystemFile.h>
#include "ImageHandler.h"
#include <numeric>
#include "ScopeGuard.h"
#include "Algorithm.h"
#include "System.h"

#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(ImageHandler_cpp)
#endif

#if !(defined(ANDROID) || defined(IOS) || defined(LINUX)) && AZ_LEGACY_CRYSYSTEM_TRAIT_IMAGEHANDLER_TIFFIO // Rally US1050 - Compile libtiff for Android and IOS
    #include <tiffio.h>

static_assert(sizeof(thandle_t) >= sizeof(AZ::IO::HandleType), "Platform defines thandle_t to be smaller than required");
#endif

namespace
{
    class Image
        : public IImageHandler::IImage
    {
    public:
        Image(std::vector<unsigned char>&& data, int width, int height)
        {
            CRY_ASSERT(data.size() == width * height * ImageHandler::c_BytesPerPixel);
            m_data = std::move(data);
            m_width = width;
            m_height = height;
        }
    private:
        virtual const std::vector<unsigned char>& GetData() const override { return m_data; }
        virtual int GetWidth() const override { return m_width; }
        virtual int GetHeight() const override { return m_height; }

        unsigned int m_width;
        unsigned int m_height;
        std::vector<unsigned char> m_data;
    };
#if AZ_LEGACY_CRYSYSTEM_TRAIT_IMAGEHANDLER_TIFFIO
    struct TiffIO
    {
        static tsize_t Read(thandle_t handle, tdata_t buffer, tsize_t size)
        {
            AZ::u64 bytesRead = 0;
            AZ::IO::FileIOBase::GetDirectInstance()->Read(static_cast<AZ::IO::HandleType>(reinterpret_cast<AZ::u64>(handle)), buffer, size, false, &bytesRead);
            return static_cast<tsize_t>(bytesRead);
        };

        static tsize_t Write(thandle_t handle, tdata_t buffer, tsize_t size)
        {
            AZ::u64 sizeWritten;
            if (AZ::IO::FileIOBase::GetDirectInstance()->Write(static_cast<AZ::IO::HandleType>(reinterpret_cast<AZ::u64>(handle)), buffer, size, &sizeWritten))
            {
                return static_cast<tsize_t>(sizeWritten);
            }
            else
            {
                return 0;
            }
        };

        static int Close(thandle_t handle)
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Close(static_cast<AZ::IO::HandleType>(reinterpret_cast<AZ::u64>(handle)));
            return 0;
        };

        static toff_t Seek(thandle_t handle, toff_t pos, int mode)
        {
            if (AZ::IO::FileIOBase::GetDirectInstance()->Seek(static_cast<AZ::IO::HandleType>(reinterpret_cast<AZ::u64>(handle)), static_cast<uint64_t>(pos), AZ::IO::GetSeekTypeFromFSeekMode(mode)))
            {
                if (mode == SEEK_SET)
                {
                    return pos;
                }
                else
                {
                    AZ::u64 offsetFromBegin;
                    if (AZ::IO::FileIOBase::GetDirectInstance()->Tell(static_cast<AZ::IO::HandleType>(reinterpret_cast<AZ::u64>(handle)), offsetFromBegin))
                    {
                        return static_cast<tsize_t>(offsetFromBegin);
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
            return -1;
        };

        static toff_t Size(thandle_t handle)
        {
            AZ::u64 fileSize = 0;
            AZ::IO::FileIOBase::GetDirectInstance()->Size(static_cast<AZ::IO::HandleType>(reinterpret_cast<AZ::u64>(handle)), fileSize);
            return static_cast<tsize_t>(fileSize);
        };

        static int Map(thandle_t, tdata_t*, toff_t*)
        {
            return 0;
        };

        static void Unmap(thandle_t, tdata_t, toff_t)
        {
            return;
        };
    };
#endif
}

std::unique_ptr<IImageHandler::IImage> ImageHandler::CreateImage(std::vector<unsigned char>&& data, int width, int height) const
{
    return std::make_unique<Image>(std::move(data), width, height);
}

std::unique_ptr<IImageHandler::IImage> ImageHandler::LoadImage([[maybe_unused]] const char* filename) const
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_IMAGEHANDLER_TIFFIO

    AZ::IO::HandleType fileHandle;
    AZ::IO::FileIOBase::GetDirectInstance()->Open(filename, AZ::IO::GetOpenModeFromStringMode("rb"), fileHandle);

    if (fileHandle == AZ::IO::InvalidHandle)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to open image file %s", filename);
        return nullptr;
    }

    auto tifHandle = std17::unique_resource_checked(TIFFClientOpen(filename, "rb", reinterpret_cast<thandle_t>(static_cast<AZ::u64>(fileHandle)), TiffIO::Read, TiffIO::Write, TiffIO::Seek, TiffIO::Close, &TiffIO::Size, TiffIO::Map, TiffIO::Unmap), (TIFF*)nullptr, TIFFClose);
    if (!tifHandle)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to load image %s", filename);
        return nullptr;
    }

    int width = 0;
    int height = 0;
    TIFFGetField(tifHandle, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tifHandle, TIFFTAG_IMAGELENGTH, &height);
    std::vector<unsigned char> data(4 * width * height);
    if (!TIFFReadRGBAImageOriented(tifHandle, width, height, reinterpret_cast<uint32*>(data.data()), ORIENTATION_TOPLEFT))
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to load image %s", filename);
        return nullptr;
    }

    //strip alpha
    int every4th = 0;
    data.erase(std::remove_if(begin(data), end(data), [&](unsigned char)
            {
                return (every4th++ & 3) == 3;
            }), end(data));

    return std::make_unique<Image>(std::move(data), width, height);
#else
    CRY_ASSERT(0); // UNIMPLEMENTED
    return nullptr;
#endif
}

bool ImageHandler::SaveImage([[maybe_unused]] IImageHandler::IImage* image, [[maybe_unused]] const char* filename) const
{
#if AZ_LEGACY_CRYSYSTEM_TRAIT_IMAGEHANDLER_TIFFIO

    AZ::IO::HandleType fileHandle;
    AZ::IO::FileIOBase::GetDirectInstance()->Open(filename, AZ::IO::GetOpenModeFromStringMode("wb"), fileHandle);

    if (fileHandle == AZ::IO::InvalidHandle)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to open image file for write %s", filename);
        return false;
    }

    auto tifHandle = std17::unique_resource_checked(TIFFClientOpen(filename, "wb", reinterpret_cast<thandle_t>(static_cast<AZ::u64>(fileHandle)), TiffIO::Read, TiffIO::Write, TiffIO::Seek, TiffIO::Close, &TiffIO::Size, TiffIO::Map, TiffIO::Unmap), (TIFF*)nullptr, TIFFClose);
    if (!tifHandle)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "Failed to save image %s", filename);
        return false;
    }

    TIFFSetField(tifHandle, TIFFTAG_IMAGEWIDTH, image->GetWidth());
    TIFFSetField(tifHandle, TIFFTAG_IMAGELENGTH, image->GetHeight());
    TIFFSetField(tifHandle, TIFFTAG_SAMPLESPERPIXEL, c_BytesPerPixel);
    TIFFSetField(tifHandle, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tifHandle, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tifHandle, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tifHandle, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tifHandle, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

    tsize_t bytesPerLine = c_BytesPerPixel * image->GetWidth();
    std::vector<unsigned char> lineBuffer;
    if (TIFFScanlineSize(tifHandle) != bytesPerLine)
    {
        lineBuffer.resize(bytesPerLine);
    }
    else
    {
        lineBuffer.resize(TIFFScanlineSize(tifHandle));
    }
    TIFFSetField(tifHandle, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tifHandle, image->GetWidth() * c_BytesPerPixel));
    auto srcData = image->GetData().data();

    for (uint32 row = 0; row < image->GetHeight(); row++)
    {
        memcpy(lineBuffer.data(), &srcData[row * bytesPerLine], bytesPerLine);
        if (TIFFWriteScanline(tifHandle, lineBuffer.data(), row, 0) < 0)
        {
            CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Failed to write part of image %s", filename);
            return false;
        }
    }

    return true;
#else
    CRY_ASSERT(0); // UNIMPLEMENTED
    return false;
#endif
}

std::unique_ptr<IImageHandler::IImage> ImageHandler::CreateDiffImage(IImageHandler::IImage* image1, IImageHandler::IImage* image2) const
{
    if (!image1 || !image2)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Could not create diff image, null arguments");
        return nullptr;
    }
    if (image1->GetWidth() != image2->GetWidth() || image1->GetHeight() != image2->GetHeight())
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Could not create diff image, 2 images were not the same size");
        return nullptr;
    }
    CRY_ASSERT(image1->GetData().size() == image1->GetWidth() * image1->GetHeight() * ImageHandler::c_BytesPerPixel);
    CRY_ASSERT(image2->GetData().size() == image2->GetWidth() * image2->GetHeight() * ImageHandler::c_BytesPerPixel);

    std::vector<unsigned char> resultRGBData;

    auto iter1 = image1->GetData().data();
    auto iter2 = image2->GetData().data();
    for (int i = 0; i < image1->GetWidth() * image1->GetHeight() * c_BytesPerPixel; ++i)
    {
        resultRGBData.push_back(static_cast<unsigned char>(abs(static_cast<int>(iter1[i]) - static_cast<int>(iter2[i]))));
    }

    return std::make_unique<Image>(std::move(resultRGBData), image1->GetWidth(), image1->GetHeight());
}

float ImageHandler::CalculatePSNR(IImageHandler::IImage* diffIimage) const
{
    if (!diffIimage)
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Could not create diff image, null arguments");
        return 0;
    }
    CRY_ASSERT(diffIimage->GetData().size() == diffIimage->GetWidth() * diffIimage->GetHeight() * ImageHandler::c_BytesPerPixel);

    auto mse = std17::accumulate(diffIimage->GetData(), 0.0, [](double result, unsigned char value) -> double { return result += (double)value * (double)value; });
    mse /= (c_BytesPerPixel * diffIimage->GetWidth() * diffIimage->GetHeight());

    if (mse <= 0)
    {
        return std::numeric_limits<float>::max();
    }

    // see http://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio for a derivation of this formula and source for magic numbers
    return static_cast<float>(20 * log10(255) - 10 * log10(mse));
}
