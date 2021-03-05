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

#pragma once

#include <ImageProcessing/PixelFormats.h>
#include <ImageProcessing/ImageObject.h>
#include <Compressors/Compressor.h>

namespace ImageProcessing
{

    //cubemap layouts
    enum CubemapLayoutType
    {
        CubemapLayoutHorizontal = 0, //6x1 strip. with rotations. 
        CubemapLayoutHorizontalCross,   //4x3. 
        CubemapLayoutVerticalCross,     //3x4
        CubemapLayoutVertical,     //1x6 strip. new output format. it's better because the memory is continuous for each face
        CubemapLayoutTypeCount,
        CubemapLayoutNone = CubemapLayoutTypeCount
    };

    class ImageToProcess
    {
    private:
        IImageObjectPtr m_img;
        ICompressor::CompressOption m_compressOption;

    private:
        ImageToProcess(const ImageToProcess&);

    public:
        ImageToProcess(IImageObjectPtr img)
        {
            m_img = img;
        }

        ~ImageToProcess()
        {
        }

        void Set(IImageObjectPtr img)
        {
            m_img = img;
        }

        IImageObjectPtr Get() const
        {
            return m_img;
        }

        ICompressor::CompressOption& GetCompressOption()
        {
            return m_compressOption;
        }

        void SetCompressOption(const ICompressor::CompressOption& compressOption)
        {
            m_compressOption = compressOption;
        }
                
    public:
        // ---------------------------------------------------------------------------------
        //! can be used to compress, requires a preset
        void ConvertFormat(EPixelFormat fmtTo);
        void ConvertFormatUncompressed(EPixelFormat fmtTo);
        
        // ---------------------------------------------------------------------------------
        // Arguments:
        //   bDeGamma - apply de-gamma correction
        bool GammaToLinearRGBA32F(bool bDeGamma);
        void LinearToGamma();

        // ---------------------------------------------------------------------------------
        // Resizers for A32B32G32R32F

        // Prerequisites: image width is even, ARGB32F only, no mips.
        void DownscaleTwiceHorizontally();

        // Prerequisites: height is even, ARGB32F only, no mips.
        void DownscaleTwiceVertically();

        // Prerequisites: width is pow of 2, ARGB32F only, no mips.
        void UpscalePow2TwiceHorizontally();

        // Prerequisites: height is pow of 2, ARGB32F only, no mips.
        void UpscalePow2TwiceVertically();

        // ---------------------------------------------------------------------------------
        // Tools for A32B32G32R32F

        // input needs to be in range 0..1
        void AddNormalMap(const IImageObject* pAddBump);

        void CreateHighPass(uint32 dwMipDown);

        void CreateColorChart();

        //convert various original cubemap layouts to new layout
        bool ConvertCubemapLayout(CubemapLayoutType newLayout);

    };
}// namespace ImageProcessing

