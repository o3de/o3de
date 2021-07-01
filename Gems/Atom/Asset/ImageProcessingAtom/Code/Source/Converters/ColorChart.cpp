/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageProcessing_precompiled.h>

#include <Processing/ImageObjectImpl.h>
#include <Processing/ImageToProcess.h>

namespace ImageProcessingAtom
{
    const int COLORCHART_IMAGE_WIDTH = 78;
    const int COLORCHART_IMAGE_HEIGHT = 66;

    // color chart in cry engine is a special image data, with size 78x66, you may see in game screenshot which is defined by a rectangle
    // area with a yellow-black dash line boarder
    // Create color chart function is to read that block of image data and convert it to a color table then save it to another image
    // with size 256x16.

    class C3dLutColorChart
    {
    public:
        C3dLutColorChart() {}
        ~C3dLutColorChart() {};

        //generate default color chart data
        void GenerateDefault();

        //generate color chart data from input image
        bool GenerateFromInput(IImageObjectPtr image);

        //ouput the color chart data to an image object
        IImageObjectPtr GenerateChartImage();

    protected:
        //extract color chart data from specified location in an image
        void ExtractFromImageAt(IImageObjectPtr pImg, AZ::u32 x, AZ::u32 y);

        //find color chart location in an image
        static bool FindColorChart(const IImageObjectPtr pImg, AZ::u32& outLocX, AZ::u32& outLocY);

        //if there is a color chart at specified location
        static bool IsColorChartAt(AZ::u32 x, AZ::u32 y, void* pData, AZ::u32 pitch);

    private:
        enum EPrimaryShades
        {
            ePS_Red = 16,
            ePS_Green = 16,
            ePS_Blue = 16,

            ePS_NumColors = ePS_Red * ePS_Green * ePS_Blue
        };

        struct SColor
        {
            unsigned char r, g, b, _padding;
        };

        typedef AZStd::vector<SColor> ColorMapping;

        ColorMapping m_mapping;
    };

    void C3dLutColorChart::GenerateDefault()
    {
        m_mapping.reserve(ePS_NumColors);

        for (int b = 0; b < ePS_Blue; ++b)
        {
            for (int g = 0; g < ePS_Green; ++g)
            {
                for (int r = 0; r < ePS_Red; ++r)
                {
                    SColor col;
                    col.r = 255 * r / (ePS_Red);
                    col.g = 255 * g / (ePS_Green);
                    col.b = 255 * b / (ePS_Blue);
                    int l = 255 - (col.r * 3 + col.g * 6 + col.b) / 10;
                    col.r = col.g = col.b = (unsigned char)l;
                    m_mapping.push_back(col);
                }
            }
        }
    }

    //find color chart location in a image
    bool C3dLutColorChart::FindColorChart(const IImageObjectPtr pImg, AZ::u32& outLocX, AZ::u32& outLocY)
    {
        const AZ::u32 width = pImg->GetWidth(0);
        const AZ::u32 height = pImg->GetHeight(0);

        //the origin image is too small to have a color chart
        if (width < COLORCHART_IMAGE_WIDTH || height < COLORCHART_IMAGE_HEIGHT)
        {
            return false;
        }

        AZ::u8* pData;
        AZ::u32 pitch;
        pImg->GetImagePointer(0, pData, pitch);

        //check all the posible start location on whether there might be a color chart
        for (AZ::u32 y = 0; y <= height - COLORCHART_IMAGE_HEIGHT; ++y)
        {
            for (AZ::u32 x = 0; x <= width - COLORCHART_IMAGE_WIDTH; ++x)
            {
                if (IsColorChartAt(x, y, pData, pitch))
                {
                    outLocX = x;
                    outLocY = y;
                    return true;
                }
            }
        }

        return false;
    }

    bool C3dLutColorChart::GenerateFromInput(IImageObjectPtr image)
    {
        AZ::u32 outLocX, outLocY;
        if (FindColorChart(image, outLocX, outLocY))
        {
            ExtractFromImageAt(image, outLocX, outLocY);
            return true;
        }
        return false;
    }

    IImageObjectPtr C3dLutColorChart::GenerateChartImage()
    {
        const AZ::u32 mipCount = 1;
        IImageObjectPtr image(IImageObject::CreateImage(ePS_Red* ePS_Blue, ePS_Green, 1, ePixelFormat_R8G8B8A8));

        {
            AZ::u8* pData;
            AZ::u32 pitch;
            image->GetImagePointer(0, pData, pitch);

            size_t nSlicePitch = (pitch / ePS_Blue);
            AZ::u32 src = 0;
            for (int b = 0; b < ePS_Blue; ++b)
            {
                for (int g = 0; g < ePS_Green; ++g)
                {
                    AZ::u8* p = pData + g * pitch + b * nSlicePitch;
                    for (int r = 0; r < ePS_Red; ++r)
                    {
                        const SColor& c = m_mapping[src];
                        p[0] = c.r;
                        p[1] = c.g;
                        p[2] = c.b;
                        p[3] = 255;
                        ++src;
                        p += 4;
                    }
                }
            }
        }

        return image;
    }

    void C3dLutColorChart::ExtractFromImageAt(IImageObjectPtr image, AZ::u32 x, AZ::u32 y)
    {
        int ox = x + 1;
        int oy = y + 1;

        AZ::u8* pData;
        AZ::u32 pitch;
        image->GetImagePointer(0, pData, pitch);

        m_mapping.reserve(ePS_NumColors);

        for (int b = 0; b < ePS_Blue; ++b)
        {
            int px = ox + ePS_Red * (b % 4);
            int py = oy + ePS_Green * (b / 4);

            for (int g = 0; g < ePS_Green; ++g)
            {
                for (int r = 0; r < ePS_Red; ++r)
                {
                    AZ::u8* p = pData + pitch * (py + g) + (px + r) * 4;

                    SColor col;
                    col.r = p[0];
                    col.g = p[1];
                    col.b = p[2];
                    m_mapping.push_back(col);
                }
            }
        }
    }

    //check if image data at location x and y could be a color chart
    //based on if the boarder is dash lines with two pixel each segement
    //the idea and implementation are both coming from CryEngine.
    bool C3dLutColorChart::IsColorChartAt(AZ::u32 x, AZ::u32 y, void* pData, AZ::u32 pitch)
    {
        struct Color
        {
        private:
            int c[3];

        public:
            Color(AZ::u32 x, AZ::u32 y, void* pPixels, AZ::u32 pitch)
            {
                const uint8* p = (const uint8*)pPixels + pitch * y + x * 4;
                c[0] = p[0];
                c[1] = p[1];
                c[2] = p[2];
            }

            bool isSimilar(const Color& a, int maxDiff) const
            {
                return
                    abs(a.c[0] - c[0]) <= maxDiff &&
                    abs(a.c[1] - c[1]) <= maxDiff &&
                    abs(a.c[2] - c[2]) <= maxDiff;
            }
        };

        const Color colorRef[2] =
        {
            Color(x, y, pData, pitch),
            Color(x + 2, y, pData, pitch)
        };

        // We require two colors of the border to be at least a bit different
        if (colorRef[0].isSimilar(colorRef[1], 15))
        {
            return false;
        }

        static const int kMaxDiff = 3;

        int refIdx = 0;
        //rectangle's top
        for (int i = 0; i < COLORCHART_IMAGE_WIDTH; i += 2)
        {
            if (!colorRef[refIdx].isSimilar(Color(x + i, y, pData, pitch), kMaxDiff) ||
                !colorRef[refIdx].isSimilar(Color(x + i + 1, y, pData, pitch), kMaxDiff))
            {
                return false;
            }
            refIdx ^= 1;
        }

        refIdx = 0;
        //left
        for (int i = 0; i < COLORCHART_IMAGE_HEIGHT; i += 2)
        {
            if (!colorRef[refIdx].isSimilar(Color(x, y + i, pData, pitch), kMaxDiff) ||
                !colorRef[refIdx].isSimilar(Color(x, y + i + 1, pData, pitch), kMaxDiff))
            {
                return false;
            }
            refIdx ^= 1;
        }

        refIdx = 0;
        //right
        for (int i = 0; i < COLORCHART_IMAGE_HEIGHT; i += 2)
        {
            if (!colorRef[refIdx].isSimilar(Color(x + COLORCHART_IMAGE_WIDTH - 1, y + i, pData, pitch), kMaxDiff) ||
                !colorRef[refIdx].isSimilar(Color(x + COLORCHART_IMAGE_WIDTH - 1, y + i + 1, pData, pitch), kMaxDiff))
            {
                return false;
            }
            refIdx ^= 1;
        }

        refIdx = 0;
        //bottom
        for (int i = 0; i < COLORCHART_IMAGE_WIDTH; i += 2)
        {
            if (!colorRef[refIdx].isSimilar(Color(x + i, y + COLORCHART_IMAGE_HEIGHT - 1, pData, pitch), kMaxDiff) ||
                !colorRef[refIdx].isSimilar(Color(x + i + 1, y + COLORCHART_IMAGE_HEIGHT - 1, pData, pitch), kMaxDiff))
            {
                return false;
            }
            refIdx ^= 1;
        }

        return true;
    }


    void ImageToProcess::CreateColorChart()
    {
        C3dLutColorChart colorChart;

        //get color chart data from source image.
        if (!colorChart.GenerateFromInput(m_img))
        {
            //if load from image failed then generate default color data
            colorChart.GenerateDefault();
        }

        //save color chart data to an image and save as current
        m_img = colorChart.GenerateChartImage();
    }
}
