/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ImageBT.h"

//---------------------------------------------------------------------------
// Load and save the VTP Binary Terrain (BT) format, documented here:
// http://vterrain.org/Implementation/Formats/BT.html

// This structure represents a binary layout in the file.  To direct load & save it, we need to remove all structure memory padding
#pragma pack(push,1)
struct BtHeader
{
    char headerTag[7];                  // Should be "binterr"
    char headerTagVersion[3];           // Should be "1.3"
    int32 columns;                      // # of columns in the heightfield
    int32 rows;                         // # of rows in the heightfield
    int16 bytesPerPoint;                // bytes per height value, either 2 for signed ints or 4 for floats
    int16 isFloatingPointData;          // 1 if height values are floats, 0 for 16-bit signed ints
    int16 horizUnits;                   // 0 if degrees, 1 if meters, 2 if international feet, 3 if US survey feet
    int16 utmZone;                      // UTM projection zone 1 to 60 or -1 to -60 (see https://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system )
    int16 datum;                        // Datum value (6001 to 6094), see http://www.epsg.org/
    double leftExtent;                  // left coordinate projection of the file
    double rightExtent;                 // right coordinate projection of the file
    double bottomExtent;                // bottom coordinate projection of the file
    double topExtent;                   // top coordinate projection of the file
    int16 externalProjection;           // 1 if projection is in an external .prj file, 0 if it's contained in the header
    float scale;                        // vertical units in meters.  0.0 should be treated as 1.0
    char unused[190];
};
#pragma pack(pop)

bool CImageBT::Save(const QString& fileName, const CFloatImage& image)
{
    int width = image.GetWidth();
    int height = image.GetHeight();

    float *pixels = image.GetData();

    // Create a header with reasonable default values.
    BtHeader header =
    {
        {'b', 'i', 'n', 't', 'e', 'r', 'r'},
        {'1', '.', '3' },
        width,
        height,
        sizeof(float),          // we'll use floats to make sure we can capture the full potential range of heightfield values
        1,                      // use floats
        1,                      // units are meters
        0,                      // no UTM projection zone
        6326,                   // WGS84 Datum value.  Recommended by VTP as the default if you don't care about Datum values.
        0.0,                    // set the left extent to 0?
        double(width),                  // set the right extent to the width? (assumes 1 m per pixel)
        double(height),                 // set the bottom extent to the height? (assumes 1 m per pixel)
        0.0,                    // set the top extent to 0?
        0,                      // no external prj file
        1.0f
    };

    memset(header.unused, 0, sizeof(header.unused));

    FILE* file = nullptr;
    azfopen(&file, fileName.toUtf8().data(), "wb");
    if (!file)
    {
        return false;
    }

    fwrite(&header, sizeof(header), 1, file);
    for (int32 y = 0; y < height; y++)
    {
        for (int32 x = 0; x < width; x++)
        {
            float heightmapValue = pixels[(y * width) + x];
            fwrite(&heightmapValue, sizeof(float), 1, file);
        }
    }

    fclose(file);
    return true;
}

//---------------------------------------------------------------------------

bool CImageBT::Load(const QString& fileName, CFloatImage& image)
{
    FILE* file = nullptr;
    azfopen(&file, fileName.toUtf8().data(), "rb");
    if (!file)
    {
        return false;
    }

    // Get the file size

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Our file needs to be at least as big as the BT file header.
    if (fileSize < sizeof(BtHeader))
    {
        return false;
    }

    // Get the BT header data
    BtHeader header;
    memset(&header, 0, sizeof(BtHeader)); // C4701 potentially uninitialized local variable 'header' used
    bool validData = true;
    validData = validData && (fread(&header, sizeof(BtHeader), 1, file) != 0);

    // Do some quick error-checking on the header to make sure it meets our expectations

    // Does the header have the right header tag? (binterr1.0 - binterr1.3)
    validData = validData && (memcmp(header.headerTag, "binterr", sizeof(header.headerTag)) == 0);
    validData = validData && (header.headerTagVersion[0] == '1') && (header.headerTagVersion[1] == '.') && (header.headerTagVersion[2] >= '0') && (header.headerTagVersion[2] <= '3');

    // Will the grid fit into a reasonable image size?
    validData = validData && (header.columns >= 0) && (header.columns < 65536);
    validData = validData && (header.rows >= 0) && (header.rows < 65536);

    // Do we either have 32-bit floats or 16-bit ints?
    validData = validData && (((header.isFloatingPointData == 1) && (header.bytesPerPoint == 4)) || ((header.isFloatingPointData == 0) && (header.bytesPerPoint == 2)));

    // Is the remaining data exactly the size needed to fill our image?
    validData = validData && ((fileSize - sizeof(BtHeader)) == (header.columns * header.rows * header.bytesPerPoint));

    if (!validData)
    {
        fclose(file);
        return false;
    }

    if (header.scale == 0.0f)
    {
        header.scale = 1.0f;
    }

    // The BT format defines the data as stored in column-first order, from bottom to top.  
    // However, some BT files store the data in row-first order, from top to bottom.
    // There isn't anything that clearly specifies which type of file it is.  If you load it the wrong way, 
    // the data will look like a bunch of wavy stripes.
    // The only difference I've found in test files is datum values above 8000, which appears to be an invalid value for datum
    // (it should be 6001-6904 according to the BT definition)
    const int invalidDatumValueDenotingColumnFirstData = 8000;
    bool isColumnFirstData = (header.datum >= invalidDatumValueDenotingColumnFirstData) ? true : false;
    float height = 0.0f;
    int imageWidth, imageHeight;

    if (isColumnFirstData)
    {
        imageWidth = header.rows;
        imageHeight = header.columns;
    }
    else
    {
        imageWidth = header.columns;
        imageHeight = header.rows;
    }


    image.Allocate(imageWidth, imageHeight);
    float* p = image.GetData();
    float maxPixel = 0.0f;

    // Read in the pixel data
    for (int32 y = 0; y < imageHeight; y++)
    {
        for (int32 x = 0; x < imageWidth; x++)
        {
            if (header.isFloatingPointData)
            {
                fread(&height, sizeof(float), 1, file);
            }
            else
            {
                int16 intHeight = 0;
                fread(&intHeight, sizeof(int16), 1, file);
                height = static_cast<float>(intHeight);
            }
            // Scale based on what our header defines, and clamp the values to positive range, negatives not supported
            p[(y * imageWidth) + x] = max((height * header.scale), 0.0f);
            maxPixel = max(maxPixel, p[(y * imageWidth) + x]);
        }
    }

    // Scale our range down to 0 - 1
    if (maxPixel > 0.0f)
    {
        p = image.GetData();
        for (int32 i = 0; i < (imageWidth * imageHeight); i++)
        {
            p[i] = clamp_tpl(p[i] / maxPixel, 0.0f, 1.0f);
        }
    }

    fclose(file);
    return true;
}

