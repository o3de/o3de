/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "ImageASC.h"

// Editor
#include "Util/Image.h"

//---------------------------------------------------------------------------

bool CImageASC::Save(const QString& fileName, const CFloatImage& image)
{
    // There are two types of ARCGrid file formats - binary (ADF) and ASCII (ASC).
    // See here:  https://en.wikipedia.org/wiki/Esri_grid

    uint32 width = image.GetWidth();
    uint32 height = image.GetHeight();
    float* pixels = image.GetData();

    AZStd::string fileHeader = AZStd::string::format(
        // Number of columns and rows in the data
        "ncols %d\n"
        "nrows %d\n"

        // The coordinates of the bottom-left corner.
        // These numbers represent coordinates on a globe, so this choice of values is arbitrary.
        "xllcorner 0.0\n"
        "yllcorner 0.0\n"

        // The size of each grid square.
        // The problem is that cellsize represents the size of a square on a grid being projected onto a globe.
        // This number can be used to convert size to degrees, based on where on the globe it appears.
        // We don't have a real-world location associated with our data, so this size choice is arbitrary.
        "cellsize 0.0003\n"

        // The value used for missing data.  Since we shouldn't have any missing data, we'll choose a value that can't appear below.
        "nodata_value -1\n"
        , width, height);

    FILE* file = nullptr;
    azfopen(&file, fileName.toUtf8().data(), "wt");
    if (!file)
    {
        return false;
    }

    // First print the file header
    fprintf(file, "%s", fileHeader.c_str());

    // Then print all the pixels.
    for (uint32 y = 0; y < height; y++)
    {
        for (uint32 x = 0; x < width; x++)
        {
            fprintf(file, "%.7f ", pixels[x + y * width]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return true;
}

//---------------------------------------------------------------------------

bool CImageASC::Load(const QString& fileName, CFloatImage& image)
{
    FILE* file = nullptr;
    azfopen(&file, fileName.toUtf8().data(), "rt");
    if (!file)
    {
        return false;
    }

    const char seps[] = " \r\n\t";
    char* token;

    int32 width = 0;
    int32 height = 0;
    float nodataValue = 0.0f;

    bool validData = true;

    // Read the file into memory

    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* str = new char[fileSize];
    fread(str, fileSize, 1, file);

    // Break all of the values in the file apart into tokens.

    [[maybe_unused]] char* nextToken = nullptr;
    token = azstrtok(str, 0, seps, &nextToken);

    // ncols = grid width
    validData = validData && (azstricmp(token, "ncols") == 0);
    token = azstrtok(nullptr, 0, seps, &nextToken);
    width = atoi(token);

    // nrows = grid height
    token = azstrtok(nullptr, 0, seps, &nextToken);
    validData = validData && (azstricmp(token, "nrows") == 0);
    token = azstrtok(nullptr, 0, seps, &nextToken);
    height = atoi(token);

    // xllcorner = leftmost coordinate.  (Skip, we don't care about it)
    token = azstrtok(nullptr, 0, seps, &nextToken);
    validData = validData && (azstricmp(token, "xllcorner") == 0);
    token = azstrtok(nullptr, 0, seps, &nextToken);

    // yllcorner = bottommost coordinate.  (Skip, we don't care about it)
    token = azstrtok(nullptr, 0, seps, &nextToken);
    validData = validData && (azstricmp(token, "yllcorner") == 0);
    token = azstrtok(nullptr, 0, seps, &nextToken);

    // cellsize = size of each grid cell.  (Skip, we don't care about it)
    token = azstrtok(nullptr, 0, seps, &nextToken);
    validData = validData && (azstricmp(token, "cellsize") == 0);
    token = azstrtok(nullptr, 0, seps, &nextToken);

    // nodata_value = the value used for missing data.  We'll replace these with 0 height.
    token = azstrtok(nullptr, 0, seps, &nextToken);
    validData = validData && (azstricmp(token, "nodata_value") == 0);
    token = azstrtok(nullptr, 0, seps, &nextToken);
    nodataValue = static_cast<float>(atof(token));

    if (!validData)
    {
        // Bad file. not supported asc.
        delete[]str;
        fclose(file);
        return false;
    }

    image.Allocate(width, height);

    // Read in the pixel data

    float* p = image.GetData();
    int size = width * height;
    int i = 0;
    float pixelValue;
    float maxPixel = 0.0f;
    while (token != nullptr && i < size)
    {
        token = azstrtok(nullptr, 0, seps, &nextToken);
        if (token != nullptr)
        {
            // Negative heights aren't supported, clamp to 0.
            pixelValue = max<float>(0.0f, static_cast<float>(atof(token)));

            // If this is a location we specifically don't have data for, set it to 0.
            if (pixelValue == nodataValue)
            {
                pixelValue = 0.0f;
            }

            *p++ = pixelValue;
            maxPixel = max(maxPixel, pixelValue);
            i++;
        }
    }

    if (maxPixel > 0.0f)
    {
        // Scale our range down to 0 - 1
        float* pp = image.GetData();
        for (i = 0; i < size; i++)
        {
            pp[i] = clamp_tpl(pp[i] / maxPixel, 0.0f, 1.0f);
        }
    }

    delete[]str;

    fclose(file);

    return true;
}

