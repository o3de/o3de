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
#ifndef CRYINCLUDE_CRYCOMMON_IIMAGEHANDLER_H
#define CRYINCLUDE_CRYCOMMON_IIMAGEHANDLER_H
#pragma once

#include <memory>

/**
Utility for loading and saving images. only works with RGB data(no alpha), and lossless compressed tiff files for now.
*/
struct IImageHandler
{
    struct IImage
    {
        virtual ~IImage() {}

        virtual const std::vector<unsigned char>& GetData() const = 0;
        virtual int GetWidth() const = 0;
        virtual int GetHeight() const = 0;
    };

    virtual ~IImageHandler() {}

    ///data must be RGB, 3 bytes per pixel.
    virtual std::unique_ptr<IImage> CreateImage(std::vector<unsigned char>&& data, int width, int height) const = 0;
    virtual std::unique_ptr<IImage> LoadImage(const char* filename) const = 0;
    virtual bool SaveImage(IImage* image, const char* filename) const = 0;
    virtual std::unique_ptr<IImage> CreateDiffImage(IImage* image1, IImage* image2) const = 0;
    virtual float CalculatePSNR(IImage* diffIimage) const = 0;
};
#endif // CRYINCLUDE_CRYCOMMON_IIMAGEHANDLER_H
