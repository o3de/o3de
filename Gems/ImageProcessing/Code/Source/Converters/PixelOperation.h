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

namespace ImageProcessing
{
    class IPixelOperation
    {
    public:
        virtual ~IPixelOperation() {}

        virtual void GetRGBA(const uint8* buf, float& r, float& g, float& b, float& a) = 0;
        virtual void SetRGBA(uint8* buf, const float& r, const float& g, const float& b, const float& a) = 0;
    };

    typedef AZStd::shared_ptr<IPixelOperation> IPixelOperationPtr;
    IPixelOperationPtr CreatePixelOperation(EPixelFormat pixelFmt);

}// namespace ImageProcessing
