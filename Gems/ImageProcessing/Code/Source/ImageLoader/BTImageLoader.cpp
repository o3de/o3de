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

#include "ImageProcessing_precompiled.h"

#include "ImageLoaders.h"

#include <ImageProcessing/ImageObject.h>
#include <AzCore/IO/FileIO.h>

#include <limits>

namespace
{
    //---------------------------------------------------------------------------
    // Load and save the VTP Binary Terrain (BT) format, documented here:
    // http://vterrain.org/Implementation/Formats/BT.html

    // This structure represents a binary layout in the file.  To direct load & save it, we need to remove all structure memory padding
#pragma pack(push,1)
    struct BtHeader
    {
        char headerTag[7];                  // Should be "binterr"
        char headerTagVersion[3];           // Should be "1.3"
        AZ::s32 columns;                    // # of columns in the heightfield
        AZ::s32 rows;                       // # of rows in the heightfield
        AZ::s16 bytesPerPoint;              // bytes per height value, either 2 for signed ints or 4 for floats
        AZ::s16 isFloatingPointData;        // 1 if height values are floats, 0 for 16-bit signed ints
        AZ::s16 horizUnits;                 // 0 if degrees, 1 if meters, 2 if international feet, 3 if US survey feet
        AZ::s16 utmZone;                    // UTM projection zone 1 to 60 or -1 to -60 (see https://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system )
        AZ::s16 datum;                      // Datum value (6001 to 6094), see http://www.epsg.org/
        double leftExtent;                  // left coordinate projection of the file
        double rightExtent;                 // right coordinate projection of the file
        double bottomExtent;                // bottom coordinate projection of the file
        double topExtent;                   // top coordinate projection of the file
        AZ::s16 externalProjection;         // 1 if projection is in an external .prj file, 0 if it's contained in the header
        float scale;                        // vertical units in meters.  0.0 should be treated as 1.0
        char unused[190];
    };
#pragma pack(pop)

    AZStd::vector<char> LoadFile(const AZStd::string& fileName)
    {
        AZ::IO::FileIOBase* fileReader = AZ::IO::FileIOBase::GetInstance();

        if (!fileReader)
        {
            return {};
        }

        // an engine compatible file reader has been attached, so use that.
        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::u64 fileSize = 0;

        if (!fileReader->Open(fileName.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, fileHandle))
        {
            return {};
        }

        if ((!fileReader->Size(fileHandle, fileSize)) || (fileSize == 0))
        {
            fileReader->Close(fileHandle);
            return {};
        }

        AZStd::vector<char> fileBuf(fileSize);

        if (!fileReader->Read(fileHandle, fileBuf.data(), fileSize, true))
        {
            fileReader->Close(fileHandle);
            return {};
        }

        fileReader->Close(fileHandle);

        return fileBuf;
    }

    bool IsHeaderValid(const BtHeader* header, std::size_t fileSize)
    {
        bool validData = true;

        // Do some quick error-checking on the header to make sure it meets our expectations

        // Does the header have the right header tag? (binterr1.0 - binterr1.3)
        validData = validData && (memcmp(header->headerTag, "binterr", sizeof(header->headerTag)) == 0);
        validData = validData && (header->headerTagVersion[0] == '1') && (header->headerTagVersion[1] == '.') && (header->headerTagVersion[2] >= '0') && (header->headerTagVersion[2] <= '3');

        // Will the grid fit into a reasonable image size?
        validData = validData && (header->columns >= 0) && (header->columns < 65536);
        validData = validData && (header->rows >= 0) && (header->rows < 65536);

        // Do we either have 32-bit floats or 16-bit ints?
        validData = validData && (((header->isFloatingPointData == 1) && (header->bytesPerPoint == 4)) || ((header->isFloatingPointData == 0) && (header->bytesPerPoint == 2)));

        // Is the remaining data exactly the size needed to fill our image?
        AZ::s32 total = header->columns * header->rows * header->bytesPerPoint;
        validData = validData && ((fileSize - sizeof(BtHeader)) == total);

        return validData;
    }
}

namespace ImageProcessing
{
    bool BTLoader::IsExtensionSupported(const char* extension)
    {
        return strcmp(extension, "bt") == 0;
    }

    /*
        Most of the logic here was taken from ImageBT.cpp. Please make sure
        any changes are kept in sync :)
    */
    IImageObject* BTLoader::LoadImageFromBT(const AZStd::string& fileName)
    {
        auto fileData = LoadFile(fileName);

        if (fileData.size() < sizeof(BtHeader))
        {
            return nullptr;
        }

        auto header = reinterpret_cast<BtHeader*>(fileData.data());

        if (!header || !IsHeaderValid(header, fileData.size()))
        {
            return nullptr;
        }

        if (header->scale == 0.0f)
        {
            header->scale = 1.0f;
        }

        // The BT format defines the data as stored in column-first order, from bottom to top.  
        // However, some BT files store the data in row-first order, from top to bottom.
        // There isn't anything that clearly specifies which type of file it is.  If you load it the wrong way, 
        // the data will look like a bunch of wavy stripes.
        // The only difference I've found in test files is datum values above 8000, which appears to be an invalid value for datum
        // (it should be 6001-6904 according to the BT definition)
        constexpr AZ::s32 invalidDatumValueDenotingColumnFirstData = 8000;
        bool isColumnFirstData = (header->datum >= invalidDatumValueDenotingColumnFirstData) ? true : false;
        AZ::s32 imageWidth = 0;
        AZ::s32 imageHeight = 0;

        if (isColumnFirstData)
        {
            imageWidth = header->rows;
            imageHeight = header->columns;
        }
        else
        {
            imageWidth = header->columns;
            imageHeight = header->rows;
        }

        IImageObject* image = IImageObject::CreateImage(imageWidth, imageHeight, 1, EPixelFormat::ePixelFormat_R32F);

        AZ::u8* p = nullptr;
        AZ::u32 dwPitch = 0;
        image->GetImagePointer(0, p, dwPitch);

        auto dst = reinterpret_cast<float*>(p);
        auto maxPixel = std::numeric_limits<float>::lowest();
        auto minPixel = std::numeric_limits<float>::max();
        auto terrainData = reinterpret_cast<const AZ::u8*>(header + 1);

        // Read in the pixel data
        if (header->isFloatingPointData)
        {
            for (AZ::s32 y = 0; y < imageHeight; ++y)
            {
                for (AZ::s32 x = 0; x < imageWidth; ++x)
                {
                    float height = *reinterpret_cast<const float*>(terrainData);
                    terrainData += sizeof(float);

                    // Scale based on what our header defines
                    float setVal = dst[(y * imageWidth) + x] = height * header->scale;
                    maxPixel = AZStd::max(maxPixel, setVal);
                    minPixel = AZStd::min(minPixel, setVal);
                }
            }
        }
        else
        {
            for (AZ::s32 y = 0; y < imageHeight; ++y)
            {
                for (AZ::s32 x = 0; x < imageWidth; ++x)
                {
                    float height = static_cast<float>(*reinterpret_cast<const AZ::u16*>(terrainData));
                    terrainData += sizeof(AZ::s16);

                    // Scale based on what our header defines
                    float setVal = dst[(y * imageWidth) + x] = height * header->scale;
                    maxPixel = AZStd::max(maxPixel, setVal);
                    minPixel = AZStd::min(minPixel, setVal);
                }
            }
        }

        // Scale our range down to 0 - 1
        auto diff = maxPixel - minPixel;
        if (AZ::GetAbs(diff) < std::numeric_limits<float>::epsilon())
        {
            diff = 1.0f;
        }

        auto imagePixels = imageWidth * imageHeight;
        for (AZ::s32 i = 0; i < imagePixels; ++i)
        {
            dst[i] = (dst[i] - minPixel) / diff;
        }

        return image;
    }
}
