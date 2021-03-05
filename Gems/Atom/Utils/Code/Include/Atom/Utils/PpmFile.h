/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AtomCore/std/containers/array_view.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/Format.h>

namespace AZ
{
    namespace Utils
    {
        //! Ppm is a portable pixel format, one of the simplest image formats out there.
        //! See https://en.wikipedia.org/wiki/Netpbm#PPM_example
        class PpmFile final
        {
        public:
            //! Creates a binary ppm buffer from an image buffer. Assumes @buffer is RBGA.
            //! @param buffer image data
            //! @param size image dimensions
            //! @param format only R8G8B8A8_UNORM and B8G8R8A8_UNORM are supported at this time
            //! @return the buffer is ppm binary with RGB payload (alpha is omitted as it is not supported by .ppm format)
            static AZStd::vector<uint8_t> CreatePpmFromImageBuffer(AZStd::array_view<uint8_t> buffer, const RHI::Size& size, RHI::Format format);

            //! Fills an image buffer with data from ppm file contents.
            //! @param ppmData the data loaded from a ppm file
            //! @param buffer output image data
            //! @param size output image dimensions
            //! @param format output image format
            //! @return true if the data was parsed successfully
            static bool CreateImageBufferFromPpm(AZStd::array_view<uint8_t> ppmData, AZStd::vector<uint8_t>& buffer, RHI::Size& size, RHI::Format& format);
        };
    }
} // namespace AZ
