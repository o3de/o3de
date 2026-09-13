/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ImageLoader/ImageLoaders.h>
#include <Atom/ImageProcessing/ImageObject.h>

#include <QString>

// From OpenEXR third party library
#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfRgba.h>
#include <OpenEXR/ImfStandardAttributes.h>
#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/ImfTiledInputFile.h>

// define imf namespace with current library version
#define Imf OPENEXR_IMF_INTERNAL_NAMESPACE
#define ImfMath IMATH_INTERNAL_NAMESPACE

namespace ImageProcessingAtom
{
    namespace ExrLoader
    {
        bool IsExtensionSupported(const char* extension)
        {
            QString ext = QString(extension).toLower();
            // This is the list of file extensions supported by this loader
            return ext == "exr";
        }

        IImageObject* LoadImageFromScanlineFile(const AZStd::string& filename)
        {
            try
            {
                Imf::InputFile exrFile(filename.c_str());

                if (!exrFile.isComplete())
                {
                    AZ_Error("Image Processing", false, "ExrLoader: uncompleted exr file [%s]", filename.c_str());
                    return nullptr;
                }

                const Imf::Header& header = exrFile.header();

                // Count the total number of channels actually present in the file, regardless of name.
                // This is what lets us tell a dual-channel (or otherwise odd) layout apart from a true
                // single-channel image or a full RGB/RGBA image.
                int32_t totalChannelCount = 0;
                for (Imf::ChannelList::ConstIterator countIt = header.channels().begin(); countIt != header.channels().end(); ++countIt)
                {
                    totalChannelCount++;
                }

                // Get Channel information for RGBA
                const Imf::Channel* channels[4];
                channels[0] = header.channels().findChannel("R");
                channels[1] = header.channels().findChannel("G");
                channels[2] = header.channels().findChannel("B");
                channels[3] = header.channels().findChannel("A");

                // Initialize pixel format to invalid one
                Imf::PixelType pixelType = Imf::NUM_PIXELTYPES;
                bool hasChannels = false;
                int32_t rgbaChannelCount = 0;

                for (int32_t idx = 0; idx < 4; idx++)
                {
                    if (channels[idx])
                    {
                        rgbaChannelCount++;
                        if (hasChannels)
                        {
                            if (pixelType != channels[idx]->type)
                            {
                                AZ_Error("Image Processing", false, "load exr file error: image "
                                    "channels have different data types [%s]", filename.c_str());
                                return nullptr;
                            }
                        }
                        else
                        {
                            pixelType = channels[idx]->type;
                            hasChannels = true;
                        }
                    }
                }

                if (totalChannelCount == 0)
                {
                    AZ_Error("Image Processing", false, "load exr file error: exr image doesn't contain "
                        "any valid channels [%s]", filename.c_str());
                    return nullptr;
                }

                // Only two channel layouts are supported today:
                //  - Full RGB (3 channels) or RGBA (4 channels), made up exclusively of R/G/B/A channels
                //  - A single channel image, regardless of how that one channel is named (Y, Height, Roughness, a lone R, etc.)
                // Anything else - dual channel, a partial RGBA combo like R+G only, or any other multi-channel
                // layout - isn't supported yet, so bail out here instead of silently mis-loading it.
                bool isFullRgba = hasChannels && (rgbaChannelCount == totalChannelCount) && (rgbaChannelCount >= 3);
                bool isSingleChannel = (totalChannelCount == 1);

                if (!isFullRgba && !isSingleChannel)
                {
                    AZ_Error("Image Processing", false, "ExrLoader: file [%s] has %d channels - only full "
                        "RGB/RGBA or single channel exr images are supported, this channel layout isn't "
                        "supported in o3de yet", filename.c_str(), totalChannelCount);
                    return nullptr;
                }

                // Identify single-channel utility maps (Luminance "Y", Height, Roughness, AO, a lone R/G/B/A, etc.)
                AZStd::string singleChannelName;
                if (isSingleChannel)
                {
                    if (hasChannels)
                    {
                        // The one channel present happens to be named R, G, B or A
                        if (channels[0]) singleChannelName = "R";
                        else if (channels[1]) singleChannelName = "G";
                        else if (channels[2]) singleChannelName = "B";
                        else singleChannelName = "A";
                    }
                    else
                    {
                        // The one channel present has some other name (Y, Height, Roughness, etc.)
                        Imf::ChannelList::ConstIterator it = header.channels().begin();
                        singleChannelName = it.name();
                        pixelType = it.channel().type;
                        hasChannels = true;
                    }
                }

                // Map to native O3DE single-channel or multi-channel pixel formats
                EPixelFormat format = ePixelFormat_Unknown;
                int32_t pixelSize = 0;

                if (isSingleChannel)
                {
                    if (pixelType == Imf::FLOAT)
                    {
                        format = EPixelFormat::ePixelFormat_R32F;
                        pixelSize = 4;
                    }
                    else if (pixelType == Imf::HALF)
                    {
                        format = EPixelFormat::ePixelFormat_R16F;
                        pixelSize = 2;
                    }
                }
                else
                {
                    if (pixelType == Imf::FLOAT)
                    {
                        format = EPixelFormat::ePixelFormat_R32G32B32A32F;
                        pixelSize = 16;
                    }
                    else if (pixelType == Imf::HALF)
                    {
                        format = EPixelFormat::ePixelFormat_R16G16B16A16F;
                        pixelSize = 8;
                    }
                }

                if (format == ePixelFormat_Unknown)
                {
                    AZ_Error("Image Processing", false, "load exr file error: unsupported exr pixel format [%d]", pixelType);
                    return nullptr;
                }

                // Get the image size
                int width, height;
                ImfMath::Box2i dw = header.dataWindow();
                width = dw.max.x - dw.min.x + 1;
                height = dw.max.y - dw.min.y + 1;

                // Create IImageObject
                IImageObject* newImage = IImageObject::CreateImage(width, height, 1, format);

                // Setup Imf FrameBuffer for loading data
                char* pixels = new char[width * height * pixelSize];
                Imf::FrameBuffer frameBuffer;
                size_t xStride = pixelSize;
                size_t yStride = pixelSize * width;
                char* base = pixels;

                if (isSingleChannel)
                {
                    frameBuffer.insert(singleChannelName.c_str(),
                        Imf::Slice(pixelType, base, xStride, yStride));
                }
                else
                {
                    int32_t channelPixelSize = pixelSize / 4;
                    frameBuffer.insert("R",
                        Imf::Slice(pixelType, base, xStride, yStride, 1, 1, 0.0));
                    frameBuffer.insert("G",
                        Imf::Slice(pixelType, base + channelPixelSize, xStride, yStride, 1, 1, 0.0));
                    frameBuffer.insert("B",
                        Imf::Slice(pixelType, base + channelPixelSize * 2, xStride, yStride, 1, 1, 0.0));
                    frameBuffer.insert("A",
                        Imf::Slice(pixelType, base + channelPixelSize * 3, xStride, yStride, 1, 1, 1.0));
                }

                exrFile.setFrameBuffer(frameBuffer);
                exrFile.readPixels(0, height - 1);

                // Save pixel data to newImage's mipmap data buffer
                AZ::u32 pitch;
                AZ::u8* mem;
                newImage->GetImagePointer(0, mem, pitch);
                memcpy(mem, base, newImage->GetMipBufSize(0));
                delete [] pixels;

                return newImage;
            }
            catch (...)
            {
                AZ_Error("Image Processing", false, "ExrLoader: load exr file [%s] error", filename.c_str());
                return nullptr;
            }
        }

        IImageObject* LoadImageFromFile(const AZStd::string& filename)
        {
            // Get exr file feature information
            bool isTiled, isDeep, isMultiPart;
            bool isExr = Imf::isOpenExrFile(filename.c_str(), isTiled, isDeep, isMultiPart);

            if (!isExr)
            {
                AZ_Error("Image Processing", false, "ExrLoader: file [%s] is not a valid exr file", filename.c_str());
                return nullptr;
            }

            if (isDeep || isMultiPart || isTiled)
            {
                if (isTiled)
                {
                    AZ_Error("Image Processing", false, "ExrLoader doesn't support tiled exr file [%s]", filename.c_str());
                }
                else
                {
                    AZ_Error("Image Processing", false, "ExrLoader: file [%s] has unsupported deep or multi-part information", filename.c_str());
                }
                return nullptr;
            }

            return LoadImageFromScanlineFile(filename);
        }
    }// namespace ExrLoader
} //namespace ImageProcessingAtom

#undef Imf
#undef ImfMath
