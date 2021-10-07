/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Image utilities implementation.


#include "EditorDefs.h"

#include "ImageUtil.h"

// Editor
#include "Util/ImageGif.h"
#include "Util/ImageTIF.h"

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::Save(const QString& strFileName, CImageEx& inImage)
{
    QImage imgBitmap;

    ImageToQImage(inImage, imgBitmap);

    // Explicitly set the pixels per meter in our images to a consistent default.
    // The normal default is 96 pixels per inch, or 3780 pixels per meter.
    // However, the Windows scaling display setting can cause these numbers to vary
    // on different machines, producing output files that have slightly different
    // headers from machine to machine, which often isn't desirable.
    const int defaultPixelsPerMeter = 3780;
    imgBitmap.setDotsPerMeterX(defaultPixelsPerMeter);
    imgBitmap.setDotsPerMeterY(defaultPixelsPerMeter);
    
    return imgBitmap.save(strFileName);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SaveBitmap(const QString& szFileName, CImageEx& inImage)
{
    return Save(szFileName, inImage);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SaveJPEG(const QString& strFileName, CImageEx& inImage)
{
    return Save(strFileName, inImage);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::Load(const QString& fileName, CImageEx& image)
{
    QImage imgBitmap(fileName);

    if (imgBitmap.isNull())
    {
        CLogFile::FormatLine("Invalid file:  %s", fileName.toUtf8().data());
        return false;
    }

    return QImageToImage(imgBitmap, image);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::LoadJPEG(const QString& strFileName, CImageEx& outImage)
{
    return CImageUtil::Load(strFileName, outImage);
}

//===========================================================================
bool CImageUtil::LoadBmp(const QString& fileName, CImageEx& image)
{
    return CImageUtil::Load(fileName, image);
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SavePGM(const QString& fileName, const CImageEx& image)
{
    // There are two types of PGM ("Portable Grey Map") files - "raw" (binary) and "plain" (ASCII).  This function supports the "plain PGM"  format.
    // See http://netpbm.sourceforge.net/doc/pgm.html or https://en.wikipedia.org/wiki/Netpbm_format for the definition.

    uint32 width = image.GetWidth();
    uint32 height = image.GetHeight();
    uint32* pixels = image.GetData();

    // Create the file header.
    AZStd::string fileHeader = AZStd::string::format(
        // P2 = PGM header for ASCII output.  (P5 is PGM header for binary output)
        "P2\n"
        // width and height of the image
        "%d %d\n"
        // The maximum grey value in the file.  (i.e. the max value for any given pixel below)
        "65535\n"
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
            fprintf(file, "%d ", pixels[x + (y * width)]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::LoadPGM(const QString& fileName, CImageEx& image)
{
    FILE* file = nullptr;
    azfopen(&file, fileName.toUtf8().data(), "rt");
    if (!file)
    {
        return false;
    }

    const char seps[] = " \n\t\r";
    char* token;


    int32 width = 0;
    int32 height = 0;
    int32 numColors = 1;


    fseek(file, 0, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* str = new char[fileSize];
    fread(str, fileSize, 1, file);

    [[maybe_unused]] char* nextToken = nullptr;
    token = azstrtok(str, 0, seps, &nextToken);

    while (token != nullptr && token[0] == '#')
    {
        if (token != nullptr && token[0] == '#')
        {
            azstrtok(nullptr, 0, "\n", &nextToken);
        }
        token = azstrtok(nullptr, 0, seps, &nextToken);
    }
    if (azstricmp(token, "P2") != 0)
    {
        // Bad file. not supported pgm.
        delete[]str;
        fclose(file);
        return false;
    }

    do
    {
        token = azstrtok(nullptr, 0, seps, &nextToken);
        if (token != nullptr && token[0] == '#')
        {
            azstrtok(nullptr, 0, "\n", &nextToken);
        }
    } while (token != nullptr && token[0] == '#');
    width = atoi(token);

    do
    {
        token = azstrtok(nullptr, 0, seps, &nextToken);
        if (token != nullptr && token[0] == '#')
        {
            azstrtok(nullptr, 0, "\n", &nextToken);
        }
    } while (token != nullptr && token[0] == '#');
    height = atoi(token);

    do
    {
        token = azstrtok(nullptr, 0, seps, &nextToken);
        if (token != nullptr && token[0] == '#')
        {
            azstrtok(nullptr, 0, "\n", &nextToken);
        }
    } while (token != nullptr && token[0] == '#');
    numColors = atoi(token);

    image.Allocate(width, height);

    uint32* p = image.GetData();
    int size = width * height;
    int i = 0;
    while (token != nullptr && i < size)
    {
        do
        {
            token = azstrtok(nullptr, 0, seps, &nextToken);
        } while (token != nullptr && token[0] == '#');
        *p++ = atoi(token);
        i++;
    }

    delete[]str;

    fclose(file);

    // If we have 16-bit greyscale values that we're storing into 32-bit pixels, denote it with an appropriate texture type.
    if (numColors > 255)
    {
        image.SetFormat(eTF_R16G16);
    }

    return true;
}


//////////////////////////////////////////////////////////////////////////
bool CImageUtil::LoadImage(const QString& fileName, CImageEx& image, bool* pQualityLoss)
{
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    if (pQualityLoss)
    {
        *pQualityLoss = false;
    }

    _splitpath_s(fileName.toUtf8().data(), drive, dir, fname, ext);

    // Only DDS has explicit sRGB flag - we'll assume by default all formats are stored in gamma space
    image.SetSRGB(true);

    if (azstricmp(ext, ".bmp") == 0)
    {
        return LoadBmp(fileName, image);
    }
    else if (azstricmp(ext, ".tif") == 0)
    {
        return CImageTIF().Load(fileName, image);
    }
    else if (azstricmp(ext, ".jpg") == 0)
    {
        if (pQualityLoss)
        {
            *pQualityLoss = true;     // we assume JPG has quality loss
        }
        return LoadJPEG(fileName, image);
    }
    else if (azstricmp(ext, ".gif") == 0)
    {
        return CImageGif().Load(fileName, image);
    }
    else if (azstricmp(ext, ".pgm") == 0)
    {
        return LoadPGM(fileName, image);
    }
    else if (azstricmp(ext, ".png") == 0)
    {
        return CImageUtil::Load(fileName, image);
    }
    else
    {
        return CImageUtil::Load(fileName, image);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CImageUtil::SaveImage(const QString& fileName, CImageEx& image)
{
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    // Remove the read-only attribute so the file can be overwritten.
    QFile(fileName).setPermissions(QFile::ReadUser | QFile::WriteUser);

    _splitpath_s(fileName.toUtf8().data(), drive, dir, fname, ext);
    if (azstricmp(ext, ".bmp") == 0)
    {
        return SaveBitmap(fileName, image);
    }
    else if (azstricmp(ext, ".jpg") == 0)
    {
        return SaveJPEG(fileName, image);
    }
    else if (azstricmp(ext, ".pgm") == 0)
    {
        return SavePGM(fileName, image);
    }
    else
    {
        return Save(fileName, image);
    }
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::ScaleToFit(const CByteImage& srcImage, CByteImage& trgImage)
{
    trgImage.ScaleToFit(srcImage);
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::DownScaleSquareTextureTwice(const CImageEx& srcImage, CImageEx& trgImage, IImageUtil::_EAddrMode eAddressingMode)
{
    uint32* pSrcData = srcImage.GetData();
    int nSrcWidth = srcImage.GetWidth();
    int nSrcHeight = srcImage.GetHeight();
    int nTrgWidth = srcImage.GetWidth() >> 1;
    int nTrgHeight = srcImage.GetHeight() >> 1;

    // reallocate target
    trgImage.Release();
    trgImage.Allocate(nTrgWidth, nTrgHeight);
    uint32* pDstData = trgImage.GetData();

    // values in this filter are the log2 of the actual multiplicative values .. see DXCFILTER_BLUR3X3 for the used 3x3 filter
    static int filter[3][3] =
    {
        {0, 1, 0},
        {1, 2, 1},
        {0, 1, 0}
    };

    for (int i = 0; i < nTrgHeight; i++)
    {
        for (int j = 0; j < nTrgWidth; j++)
        {
            // filter3x3
            int x = j << 1;
            int y = i << 1;

            int r, g, b, a;
            r = b = g = a = 0;
            uint32 col;

            if (eAddressingMode == IImageUtil::WRAP) // TODO: this condition could be compile-time static by making it a template arg
            {
                for (int ii = 0; ii < 3; ii++)
                {
                    for (int jj = 0; jj < 3; jj++)
                    {
                        col = pSrcData[((y + nSrcHeight + ii - 1) % nSrcHeight) * nSrcWidth + ((x + nSrcWidth + jj - 1) % nSrcWidth)];

                        r += (col        & 0xff) << filter[ii][jj];
                        g += ((col >> 8)  & 0xff) << filter[ii][jj];
                        b += ((col >> 16) & 0xff) << filter[ii][jj];
                        a += ((col >> 24) & 0xff) << filter[ii][jj];
                    }
                }
            }
            else
            {
                assert(eAddressingMode == IImageUtil::CLAMP);
                for (int ii = 0; ii < 3; ii++)
                {
                    for (int jj = 0; jj < 3; jj++)
                    {
                        int x1 = clamp_tpl<int>((x + jj), 0, nSrcWidth - 1);
                        int y1 = clamp_tpl<int>((y + ii), 0, nSrcHeight - 1);
                        col = pSrcData[ y1 * nSrcWidth + x1];

                        r += (col        & 0xff) << filter[ii][jj];
                        g += ((col >> 8)  & 0xff) << filter[ii][jj];
                        b += ((col >> 16) & 0xff) << filter[ii][jj];
                        a += ((col >> 24) & 0xff) << filter[ii][jj];
                    }
                }
            }

            // the sum of the multiplicative values here is 16 so we shift by 4 bits
            r >>= 4;
            g >>= 4;
            b >>= 4;
            a >>= 4;

            uint32 res = r + (g << 8) + (b << 16) + (a << 24);

            *pDstData++ = res;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::ScaleToFit(const CImageEx& srcImage, CImageEx& trgImage)
{
    trgImage.ScaleToFit(srcImage);
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::ScaleToDoubleFit(const CImageEx& srcImage, CImageEx& trgImage)
{
    uint32 x, y, u, v;
    unsigned int* destRow, * dest, * src, * sourceRow;

    uint32 srcW = srcImage.GetWidth();
    uint32 srcH = srcImage.GetHeight();

    uint32 trgHalfW = trgImage.GetWidth() / 2;
    uint32 trgH = trgImage.GetHeight();

    uint32 xratio = trgHalfW > 0 ? (srcW << 16) / trgHalfW : 1;
    uint32 yratio = trgH > 0 ? (srcH << 16) / trgH : 1;

    src = srcImage.GetData();
    destRow = trgImage.GetData();

    v = 0;
    for (y = 0; y < trgH; y++)
    {
        u = 0;
        sourceRow = src + (v >> 16) * srcW;
        dest = destRow;
        for (x = 0; x < trgHalfW; x++)
        {
            *(dest + trgHalfW) = sourceRow[u >> 16];
            *dest++ = sourceRow[u >> 16];
            u += xratio;
        }
        v += yratio;
        destRow += trgHalfW * 2;
    }
}

//////////////////////////////////////////////////////////////////////////
void CImageUtil::SmoothImage(CByteImage& image, int numSteps)
{
    assert(numSteps > 0);
    uint8* buf = image.GetData();
    int w = image.GetWidth();
    int h = image.GetHeight();

    for (int steps = 0; steps < numSteps; steps++)
    {
        // Smooth the image.
        for (int y = 1; y < h - 1; y++)
        {
            // Precalculate for better speed
            uint8* ptr = &buf[y * w + 1];

            for (int x = 1; x < w - 1; x++)
            {
                // Smooth it out
                *ptr =
                    (
                        (uint32)ptr[1] +
                        ptr[w] +
                        ptr[-1] +
                        ptr[-w] +
                        ptr[w + 1] +
                        ptr[w - 1] +
                        ptr[-w + 1] +
                        ptr[-w - 1]
                    ) >> 3;

                // Next pixel
                ptr++;
            }
        }
    }
}

unsigned char CImageUtil::GetBilinearFilteredAt(const int iniX256, const int iniY256, const CByteImage& image)
{
    //  assert(image.IsValid());        if(!image.IsValid())return(0);      // this shouldn't be

    DWORD x = (DWORD)(iniX256) >> 8;
    DWORD y = (DWORD)(iniY256) >> 8;

    if (x >= static_cast<DWORD>(image.GetWidth() - 1) || y >= static_cast<DWORD>(image.GetHeight() - 1))
    {
        return image.ValueAt(x, y);                                                          // border is not filtered, 255 to get in range 0..1
    }
    DWORD rx = (DWORD)(iniX256) & 0xff;       // fractional aprt
    DWORD ry = (DWORD)(iniY256) & 0xff;       // fractional aprt

    DWORD top = (DWORD)image.ValueAt((int)x, (int)y) * (256 - rx)                      // left top
        + (DWORD)image.ValueAt((int)x + 1, (int)y) * rx;                                            // right top

    DWORD bottom = (DWORD)image.ValueAt((int)x, (int)y + 1) * (256 - rx)              // left bottom
        + (DWORD)image.ValueAt((int)x + 1, (int)y + 1) * rx;                                        // right bottom

    return (unsigned char)((top * (256 - ry) + bottom * ry) >> 16);
}

bool CImageUtil::QImageToImage(const QImage& bitmap, CImageEx& image)
{

    QImage convertedBitmap;
    const QImage *srcBitmap = &bitmap;
    
    if (bitmap.format() != QImage::Format_RGBA8888)
    {
        convertedBitmap = bitmap.convertToFormat(QImage::Format_RGBA8888);
        srcBitmap = &convertedBitmap;
    }

    if (image.Allocate(srcBitmap->width(), srcBitmap->height()) == false)
    {
        return false;
    }

    AZStd::copy(srcBitmap->bits(), srcBitmap->bits() + (srcBitmap->width() * srcBitmap->height() * sizeof(uint32)), reinterpret_cast<uint8*>(image.GetData()));

    return true;
}

bool CImageUtil::ImageToQImage(const CImageEx& image, QImage& bitmapObj)
{
    bitmapObj = QImage(image.GetWidth(), image.GetHeight(), QImage::Format_RGBA8888);
    AZStd::copy(image.GetData(), image.GetData() + image.GetWidth() * image.GetHeight(), reinterpret_cast<uint32*>(bitmapObj.bits()));

    return true;
}
