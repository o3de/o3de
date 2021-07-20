/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ImageTIF.h"

/// libTiff
#include <tiffio.h>  // TIFF library

// Function prototypes
static tsize_t libtiffDummyReadProc (thandle_t fd, tdata_t buf, tsize_t size);
static tsize_t libtiffDummyWriteProc (thandle_t fd, tdata_t buf, tsize_t size);
static toff_t libtiffDummySizeProc(thandle_t fd);
static toff_t libtiffDummySeekProc (thandle_t fd, toff_t off, int i);
//static int libtiffDummyCloseProc (thandle_t fd);

// Structure used to pass state to our in-memory TIFF file callbacks
struct MemImage
{
    uint8 *buffer;
    uint32 offset;
    uint32 size;
};


/////////////////// Callbacks to libtiff

static int libtiffDummyMapFileProc(thandle_t, tdata_t*, toff_t*)
{
    return 0;
}

static void libtiffDummyUnmapFileProc(thandle_t, tdata_t, toff_t)
{
}

static toff_t libtiffDummySizeProc(thandle_t fd)
{
    MemImage *memImage = static_cast<MemImage *>(fd);
    return memImage->size;
}

static tsize_t
libtiffDummyReadProc (thandle_t fd, tdata_t buf, tsize_t size)
{
    MemImage *memImage = static_cast<MemImage *>(fd);
    tsize_t nBytesLeft = memImage->size - memImage->offset;

    if (size > nBytesLeft)
    {
        size = nBytesLeft;
    }

    memcpy(buf, &memImage->buffer[memImage->offset], size);

    memImage->offset += size;

    // Return the amount of data read
    return size;
}

static tsize_t
libtiffDummyWriteProc ([[maybe_unused]] thandle_t fd, [[maybe_unused]] tdata_t buf, tsize_t size)
{
    return (size);
}

static toff_t
libtiffDummySeekProc (thandle_t fd, toff_t off, int i)
{
    MemImage *memImage = static_cast<MemImage *>(fd);
    switch (i)
    {
    case SEEK_SET:
        memImage->offset = off;
        break;

    case SEEK_CUR:
        memImage->offset += off;
        break;

    case SEEK_END:
        memImage->offset = memImage->size - off;
        break;

    default:
        memImage->offset = off;
        break;
    }

    // This appears to return the location that it went to
    return memImage->offset;
}

static int
libtiffDummyCloseProc ([[maybe_unused]] thandle_t fd)
{
    // Return a zero meaning all is well
    return 0;
}

bool CImageTIF::Load(const QString& fileName, CImageEx& outImage)
{
    CCryFile file;
    if (!file.Open(fileName.toUtf8().data(), "rb"))
    {
        CLogFile::FormatLine("File not found %s", fileName.toUtf8().data());
        return false;
    }

    MemImage memImage;

    std::vector<uint8> data;

    memImage.size = file.GetLength();

    data.resize(memImage.size);
    memImage.buffer = &data[0];
    memImage.offset = 0;

    file.ReadRaw(memImage.buffer, memImage.size);


    // Open the dummy document (which actually only exists in memory)
    TIFF* tif = TIFFClientOpen (fileName.toUtf8().data(), "rm", (thandle_t)&memImage, libtiffDummyReadProc,
            libtiffDummyWriteProc, libtiffDummySeekProc,
            libtiffDummyCloseProc, libtiffDummySizeProc, libtiffDummyMapFileProc, libtiffDummyUnmapFileProc);

    //  TIFF* tif = TIFFOpen(fileName,"r");

    bool bRet = false;

    if (tif)
    {
        uint32 dwWidth, dwHeight;
        size_t npixels;
        uint32* raster;
        char* dccfilename = NULL;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &dwWidth);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &dwHeight);
        TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &dccfilename);

        npixels = dwWidth * dwHeight;

        raster = (uint32*)_TIFFmalloc((tsize_t)(npixels * sizeof(uint32)));

        if (raster)
        {
            if (TIFFReadRGBAImage(tif, dwWidth, dwHeight, raster, 0))
            {
                if (outImage.Allocate(dwWidth, dwHeight))
                {
                    char* dest = (char*)outImage.GetData();
                    uint32 dwPitch = dwWidth * 4;

                    for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
                    {
                        char* src2 = (char*)&raster[(dwHeight - 1 - dwY) * dwWidth];
                        char* dest2 = &dest[dwPitch * dwY];

                        memcpy(dest2, src2, dwWidth * 4);
                    }

                    if (dccfilename)
                    {
                        outImage.SetDccFilename(dccfilename);
                    }

                    bRet = true;
                }
            }

            _TIFFfree(raster);
        }

        TIFFClose(tif);
    }

    if (!bRet)
    {
        outImage.Detach();
    }

    return bRet;
}

bool CImageTIF::Load(const QString& fileName, CFloatImage& outImage)
{
    // Defined in GeoTIFF format - http://web.archive.org/web/20160403164508/http://www.remotesensing.org/geotiff/spec/geotiffhome.html
    // Used to get the X, Y, Z scales from a GeoTIFF file
    static const int GEOTIFF_MODELPIXELSCALE_TAG = 33550;

    
    CCryFile file;
    if (!file.Open(fileName.toUtf8().data(), "rb"))
    {
        CLogFile::FormatLine("File not found %s", fileName.toUtf8().data());
        return false;
    }

    MemImage memImage;

    std::vector<uint8> data;

    memImage.size = file.GetLength();

    data.resize(memImage.size);
    memImage.buffer = &data[0];
    memImage.offset = 0;

    file.ReadRaw(memImage.buffer, memImage.size);


    // Open the dummy document (which actually only exists in memory)
    TIFF* tif = TIFFClientOpen(fileName.toUtf8().data(), "rm", (thandle_t)&memImage, libtiffDummyReadProc,
        libtiffDummyWriteProc, libtiffDummySeekProc,
        libtiffDummyCloseProc, libtiffDummySizeProc, libtiffDummyMapFileProc, libtiffDummyUnmapFileProc);

    //  TIFF* tif = TIFFOpen(fileName,"r");

    bool bRet = false;

    if (tif)
    {
        uint32 width = 0, height = 0;
        uint16 spp = 0, bpp = 0, format = 0;
        char* dccfilename = NULL;

        TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &dccfilename);

        TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetFieldDefaulted(tif, TIFFTAG_IMAGELENGTH, &height);

        TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bpp);     // how many bits each color component is.  typically 8-bit, but could be 16-bit.
        TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &spp);   // how many color components per pixel?  1=greyscale, 3=RGB, 4=RGBA
        TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLEFORMAT, &format);   // format of the pixel data - int, uint, float.  

        // There are two types of 32-bit floating point TIF semantics.  Paint programs tend to use values in the 0.0 - 1.0 range.
        // GeoTIFF files use values where 1.0 = 1 meter by default, but also have an optional ZScale parameter to provide additional
        // scaling control.

        // By default, we'll assume this is a regular TIFF that we want to leave in the 0.0 - 1.0 range.
        float pixelValueScale = 1.0f;

        // Check to see if it's a GeoTIFF, and if so, whether or not it has the ZScale parameter.
        uint32 tagCount = 0;
        double *pixelScales = NULL;
        if (TIFFGetField(tif, GEOTIFF_MODELPIXELSCALE_TAG, &tagCount, &pixelScales) == 1)
        {
            // if there's an xyz scale, and the Z scale isn't 0, let's use it.
            if ((tagCount == 3) && (pixelScales != NULL) && (pixelScales[2] != 0.0f))
            {
                pixelValueScale = static_cast<float>(pixelScales[2]);
            }
        }

        uint32 linesize = TIFFScanlineSize(tif);
        uint8* linebuf = static_cast<uint8*>(_TIFFmalloc(linesize));

        // We assume that a scanline has all of the samples in it.  Validate the assumption.
        assert(linesize == (width * (bpp / 8) * spp));

        // Aliases for linebuf to make it easier to pull different types out of the scanline.
        uint16* linebufUint16 = reinterpret_cast<uint16*>(linebuf);
        uint32* linebufUint32 = reinterpret_cast<uint32*>(linebuf);
        float* linebufFloat = reinterpret_cast<float*>(linebuf);

        if (linebuf)
        {
            if (outImage.Allocate(width, height))
            {
                float* dest = outImage.GetData();
                bRet = true;

                float maxPixelValue = 0.0f;

                for (uint32 y = 0; y < height; y++)
                {
                    TIFFReadScanline(tif, linebuf, y);

                    // For each pixel, we either scale or clamp the values to a 16-bit range.  It is asymmetric behaviour, but based
                    // on assumptions about the input data:
                    // 8-bit values are scaled up because 8-bit textures used as heightmaps are usually scaled-down 16-bit values.
                    // 32-bit values may or may not need to scale down, depending on the intended authoring range.  Our assumption
                    // is that they were most likely authored with the intent of 1:1 value translations.

                    for (uint32 x = 0; x < width; x++)
                    {
                        switch (bpp)
                        {
                            case 8:
                                // Scale 0-255 to 0.0 - 1.0
                                dest[(y * width) + x] = static_cast<float>(linebuf[x * spp]) / static_cast<float>(std::numeric_limits<uint8>::max());
                                break;
                            case 16:
                                // Scale 0-65535 to 0.0 - 1.0
                                dest[(y * width) + x] = static_cast<float>(linebufUint16[x * spp]) / static_cast<float>(std::numeric_limits<uint16>::max());
                                break;
                            case 32:
                                // 32-bit values could be ints or floats.

                                if (format == SAMPLEFORMAT_INT)
                                {
                                    // Scale 0-max int32 to 0.0 - 1.0
                                    dest[(y * width) + x] = clamp_tpl(static_cast<float>(linebufUint32[x * spp]) / static_cast<float>(std::numeric_limits<int32>::max()), 0.0f, 1.0f);
                                }
                                else if (format == SAMPLEFORMAT_UINT)
                                {
                                    // Scale 0-max uint32 to 0.0 - 1.0
                                    dest[(y * width) + x] = clamp_tpl(static_cast<float>(linebufUint32[x * spp]) / static_cast<float>(std::numeric_limits<uint32>::max()), 0.0f, 1.0f);
                                }
                                else if (format == SAMPLEFORMAT_IEEEFP)
                                {
                                    dest[(y * width) + x] = linebufFloat[x * spp] * pixelValueScale;
                                }
                                else
                                {
                                    // Unknown / unsupported format.
                                    bRet = false;
                                }
                                break;
                            default:
                                // Unknown / unsupported format.
                                bRet = false;
                                break;
                        }

                        maxPixelValue = max(maxPixelValue, dest[(y * width) + x]);
                    }
                }

                if (dccfilename)
                {
                    outImage.SetDccFilename(dccfilename);
                }

                // If this is a GeoTIFF using 32-bit floats, we will end up outside the 0.0 - 1.0 range.  Let's scale it back down to 0.0 - 1.0.
                if (maxPixelValue > 1.0f)
                {
                    for (uint32 y = 0; y < height; y++)
                    {
                        for (uint32 x = 0; x < width; x++)
                        {
                            dest[(y * width) + x] = dest[(y * width) + x] / maxPixelValue;
                        }
                    }
                }
            }

            _TIFFfree(linebuf);
        }

        TIFFClose(tif);
    }

    if (!bRet)
    {
        outImage.Detach();
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
bool CImageTIF::SaveRAW(const QString& fileName, const void* pData, int width, int height, int bytesPerChannel, int numChannels, bool bFloat, const char* preset)
{
    if (bFloat && (bytesPerChannel != 2 && bytesPerChannel != 4))
    {
        bFloat = false;
    }

    bool bRet = false;

    CFileUtil::OverwriteFile(fileName);
    TIFF* tif = TIFFOpen(fileName.toUtf8().data(), "wb");
    if (tif)
    {
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, numChannels);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bytesPerChannel * 8);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, (numChannels == 1) ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        if (bFloat)
        {
            TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
        }

        if (preset && preset[0])
        {
            string tiffphotoshopdata, valueheader;
            string presetkeyvalue = string("/preset=") + string(preset);

            valueheader.push_back('\x1C');
            valueheader.push_back('\x02');
            valueheader.push_back('\x28');
            valueheader.push_back((presetkeyvalue.size() >> 8) & 0xFF);
            valueheader.push_back((presetkeyvalue.size()) & 0xFF);
            valueheader.append(presetkeyvalue);

            tiffphotoshopdata.push_back('8');
            tiffphotoshopdata.push_back('B');
            tiffphotoshopdata.push_back('I');
            tiffphotoshopdata.push_back('M');
            tiffphotoshopdata.push_back('\x04');
            tiffphotoshopdata.push_back('\x04');
            tiffphotoshopdata.push_back('\x00');
            tiffphotoshopdata.push_back('\x00');

            tiffphotoshopdata.push_back((valueheader.size() >> 24) & 0xFF);
            tiffphotoshopdata.push_back((valueheader.size() >> 16) & 0xFF);
            tiffphotoshopdata.push_back((valueheader.size() >> 8) & 0xFF);
            tiffphotoshopdata.push_back((valueheader.size()) & 0xFF);
            tiffphotoshopdata.append(valueheader);

            TIFFSetField(tif, TIFFTAG_PHOTOSHOP, tiffphotoshopdata.size(), tiffphotoshopdata.c_str());
        }

        size_t pitch = width * bytesPerChannel * numChannels;
        char* raster = (char*) _TIFFmalloc((tsize_t)(pitch * height));
        memcpy(raster, pData, pitch * height);

        bRet = true;
        for (int h = 0; h < height; ++h)
        {
            size_t offset = h * pitch;
            int err = TIFFWriteScanline(tif, raster + offset, h, 0);
            if (err < 0)
            {
                bRet = false;
                break;
            }
        }
        _TIFFfree(raster);
        TIFFClose(tif);
    }
    return bRet;
}

const char* CImageTIF::GetPreset(const QString& fileName)
{
    std::vector<uint8> data;
    CCryFile file;
    if (!file.Open(fileName.toUtf8().data(), "rb"))
    {
        CLogFile::FormatLine("File not found %s", fileName.toUtf8().data());
        return NULL;
    }

    MemImage memImage;

    memImage.size = file.GetLength();

    data.resize(memImage.size);
    memImage.buffer = &data[0];
    memImage.offset = 0;

    file.ReadRaw(memImage.buffer, memImage.size);

    TIFF* tif = TIFFClientOpen (fileName.toUtf8().data(), "rm", (thandle_t)&memImage, libtiffDummyReadProc,
            libtiffDummyWriteProc, libtiffDummySeekProc,
            libtiffDummyCloseProc, libtiffDummySizeProc, libtiffDummyMapFileProc, libtiffDummyUnmapFileProc);

    string strReturn;
    char* preset = NULL;
    int size;
    if (tif)
    {
        TIFFGetField(tif, TIFFTAG_PHOTOSHOP, &size, &preset);
        for (int i = 0; i < size; ++i)
        {
            if (!strncmp((preset + i), "preset", 6))
            {
                char* presetoffset = preset + i;
                strReturn = presetoffset;
                if (strReturn.find('/') != -1)
                {
                    strReturn = strReturn.substr(0, strReturn.find('/'));
                }

                break;
            }
        }
        TIFFClose(tif);
    }
    return strReturn.c_str();
}
