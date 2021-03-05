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
#ifndef CRYINCLUDE_CRYSYSTEM_IMAGEHANDLER_H
#define CRYINCLUDE_CRYSYSTEM_IMAGEHANDLER_H

#include "IImageHandler.h"

class ImageHandler
    : public IImageHandler
{
public:
    static const int c_BytesPerPixel = 3; //This only deals with RGB data for now, no alpha
private:
    virtual std::unique_ptr<IImageHandler::IImage> CreateImage(std::vector<unsigned char>&& rgbData, int width, int height) const override;
    virtual std::unique_ptr<IImageHandler::IImage> LoadImage(const char* filename) const override;
    virtual bool SaveImage(IImageHandler::IImage* image, const char* filename) const override;
    virtual std::unique_ptr<IImageHandler::IImage> CreateDiffImage(IImageHandler::IImage* image1, IImageHandler::IImage* image2) const override;
    virtual float CalculatePSNR(IImageHandler::IImage* diffIimage) const override;
};
#endif // CRYINCLUDE_CRYSYSTEM_IMAGEHANDLER_H
