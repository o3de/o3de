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
                    return NULL;
                }

                const Imf::Header& header = exrFile.header();

                // Get Channel information for RGBA
                const Imf::Channel* channels[4];
                channels[0] = header.channels().findChannel("R");
                channels[1] = header.channels().findChannel("G");
                channels[2] = header.channels().findChannel("B");
                channels[3] = header.channels().findChannel("A");
                // Initialize pixel format to invalid one
                Imf::PixelType pixelType = Imf::NUM_PIXELTYPES;
                bool hasChannels = false;
                for (int32_t idx = 0; idx < 4; idx++)
                {
                    if (channels[idx])
                    {
                        if (hasChannels)
                        {
                            if (pixelType != channels[idx]->type)
                            {
                                // return null if there are different pixel types in different channels
                                AZ_Error("Image Processing", false, "load exr file error: image "
                                    "channels have different data types", filename.c_str());
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

                if (!hasChannels)
                {
                    // return null if there are no rgba channels
                    AZ_Error("Image Processing", false, "load exr file error: exr image doesn't contain "
                        "any rgba channels", filename.c_str());
                    return nullptr;
                }

                // Find the EPixelFormat to matching the format
                EPixelFormat format = ePixelFormat_Unknown;
                int32_t pixelSize = 0;
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
                else
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
                int32_t channelPixelSize = pixelSize / 4;
                char* base = pixels;
                frameBuffer.insert("R",
                    Imf::Slice(pixelType, base, xStride, yStride));
                frameBuffer.insert("G",
                    Imf::Slice(pixelType, base + channelPixelSize, xStride, yStride));
                frameBuffer.insert("B",
                    Imf::Slice(pixelType, base + channelPixelSize * 2, xStride, yStride));
                // Insert A with default value of 1
                frameBuffer.insert("A",
                    Imf::Slice(pixelType, base + channelPixelSize * 3, xStride, yStride, 1, 1, 1.0));
                exrFile.setFrameBuffer(frameBuffer);

                exrFile.readPixels(0, height - 1);
                // save pixel data to newImage's mipmap data buffer
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
            // In the current implementation it supports load one flat image with one or few of rgba channels.
            // It wont handle multi-part, deep image or some arbitrary channels. It also won't handle layers.
            // It's often the environment map wasn't saved with "envmap" header, so we are not trying get the information.

            // Get exr file feature information
            bool isTiled, isDeep, isMultiPart;
            bool isExr = Imf::isOpenExrFile(filename.c_str(), isTiled, isDeep, isMultiPart);

            if (!isExr)
            {
                AZ_Error("Image Processing", false, "ExrLoader: file [%s] is not a valid exr file", filename.c_str());
                return NULL;
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
                return NULL;
            }

            return LoadImageFromScanlineFile(filename);
        }
    }// namespace ExrLoader
} //namespace ImageProcessingAtom

#undef Imf
#undef ImfMath
