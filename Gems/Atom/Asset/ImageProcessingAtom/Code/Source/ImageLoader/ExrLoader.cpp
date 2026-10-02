/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ImageLoader/ImageLoaders.h>
#include <Atom/ImageProcessing/ImageObject.h>

#include <AzCore/std/containers/vector.h>

#include <QString>

// From OpenEXR third party library
#include <OpenEXR/ImfArray.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
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
                const Imf::ChannelList& channelList = header.channels();

                int32_t channelCount = 0;
                for (Imf::ChannelList::ConstIterator channel = channelList.begin(); channel != channelList.end(); ++channel)
                {
                    ++channelCount;
                }

                if (channelCount == 0)
                {
                    AZ_Error("Image Processing", false, "ExrLoader: file [%s] contains no channels", filename.c_str());
                    return nullptr;
                }

                const Imf::Channel* rgbaChannels[] = {
                    channelList.findChannel("R"),
                    channelList.findChannel("G"),
                    channelList.findChannel("B"),
                    channelList.findChannel("A"),
                };

                int32_t rgbaChannelCount = 0;
                for (const Imf::Channel* channel : rgbaChannels)
                {
                    if (channel)
                    {
                        ++rgbaChannelCount;
                    }
                }

                const bool isSingleChannel = channelCount == 1;
                const bool hasOnlyRgbaChannels = rgbaChannelCount == channelCount;

                // Arbitrary channel names are supported only for single-channel images.
                if (!isSingleChannel && !hasOnlyRgbaChannels)
                {
                    AZ_Error("Image Processing", false, "ExrLoader: unsupported channel layout in file [%s]", filename.c_str());
                    return nullptr;
                }

                Imf::PixelType pixelType = Imf::NUM_PIXELTYPES;
                const char* singleChannelName = nullptr;
                if (isSingleChannel)
                {
                    const Imf::ChannelList::ConstIterator channel = channelList.begin();
                    singleChannelName = channel.name();
                    pixelType = channel.channel().type;
                }
                else
                {
                    for (const Imf::Channel* channel : rgbaChannels)
                    {
                        if (!channel)
                        {
                            continue;
                        }

                        if (pixelType == Imf::NUM_PIXELTYPES)
                        {
                            pixelType = channel->type;
                        }
                        else if (pixelType != channel->type)
                        {
                            AZ_Error("Image Processing", false, "ExrLoader: mismatched channel pixel types in file [%s]", filename.c_str());
                            return nullptr;
                        }
                    }
                }

                EPixelFormat format;
                size_t pixelSize;
                if (pixelType == Imf::FLOAT)
                {
                    if (isSingleChannel)
                    {
                        format = EPixelFormat::ePixelFormat_R32F;
                        pixelSize = 4;
                    }
                    else
                    {
                        format = EPixelFormat::ePixelFormat_R32G32B32A32F;
                        pixelSize = 16;
                    }
                }
                else if (pixelType == Imf::HALF)
                {
                    if (isSingleChannel)
                    {
                        format = EPixelFormat::ePixelFormat_R16F;
                        pixelSize = 2;
                    }
                    else
                    {
                        format = EPixelFormat::ePixelFormat_R16G16B16A16F;
                        pixelSize = 8;
                    }
                }
                else
                {
                    AZ_Error("Image Processing", false, "ExrLoader: unsupported pixel type [%d] in file [%s]", pixelType, filename.c_str());
                    return nullptr;
                }

                const ImfMath::Box2i dataWindow = header.dataWindow();
                const int32_t width = dataWindow.max.x - dataWindow.min.x + 1;
                const int32_t height = dataWindow.max.y - dataWindow.min.y + 1;

                AZStd::vector<char> pixels(static_cast<size_t>(width) * height * pixelSize);
                Imf::FrameBuffer frameBuffer;
                const size_t xStride = pixelSize;
                const size_t yStride = pixelSize * width;
                char* base = pixels.data();

                if (isSingleChannel)
                {
                    frameBuffer.insert(singleChannelName, Imf::Slice(pixelType, base, xStride, yStride));
                }
                else
                {
                    const size_t channelPixelSize = pixelSize / 4;
                    frameBuffer.insert("R", Imf::Slice(pixelType, base, xStride, yStride));
                    frameBuffer.insert("G", Imf::Slice(pixelType, base + channelPixelSize, xStride, yStride));
                    frameBuffer.insert("B", Imf::Slice(pixelType, base + channelPixelSize * 2, xStride, yStride));
                    frameBuffer.insert("A", Imf::Slice(pixelType, base + channelPixelSize * 3, xStride, yStride, 1, 1, 1.0));
                }

                exrFile.setFrameBuffer(frameBuffer);
                exrFile.readPixels(0, height - 1);

                IImageObject* newImage = IImageObject::CreateImage(width, height, 1, format);
                AZ::u32 pitch;
                AZ::u8* mem;
                newImage->GetImagePointer(0, mem, pitch);
                memcpy(mem, pixels.data(), newImage->GetMipBufSize(0));

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
    } // namespace ExrLoader
} // namespace ImageProcessingAtom

#undef Imf
#undef ImfMath
