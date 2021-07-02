/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/ImageProcessing/PixelFormats.h>

namespace ImageProcessingAtom
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
}// namespace ImageProcessingAtom
