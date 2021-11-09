/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/PixelFormats.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <Compressors/Compressor.h>
#include <ImageBuilderBaseType.h>

namespace ImageProcessingAtom
{
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

        //convert various original cubemap layouts to new layout
        bool ConvertCubemapLayout(CubemapLayoutType newLayout);
    };
}// namespace ImageProcessingAtom

