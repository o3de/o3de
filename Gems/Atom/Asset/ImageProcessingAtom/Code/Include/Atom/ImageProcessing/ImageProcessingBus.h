/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
*or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/EBus/EBus.h>
#include <Atom/ImageProcessing/ImageObject.h>

namespace ImageProcessingAtom
{
    class ImageProcessingRequests
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;
        //////////////////////////////////////////////////////////////////////////

        // Loads an image from a source file path
        virtual IImageObjectPtr LoadImage(const AZStd::string& filePath) = 0;

        // Loads an image from a source file path and converts it to a format suitable for previewing in tools
        virtual IImageObjectPtr LoadImagePreview(const AZStd::string& filePath) = 0;
    };
    using ImageProcessingRequestBus = AZ::EBus<ImageProcessingRequests>;
} // namespace ImageProcessingAtom
