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

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/string/string.h>

#include <QImage>

class QImage;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserModel;

        //! Sends requests to output preview image for texture assets. Used for internal only!
        class AssetBrowserTexturePreviewRequests
            : public AZ::EBusTraits
        {
        public:

            // Only a single handler is allowed
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

            //! Request to get a preview image for texture product
            //@return whether the output image is valid or not
            virtual bool GetProductTexturePreview(const char* /*fullProductFileName*/, QImage& /*previewImage*/, AZStd::string& /*productInfo*/, AZStd::string& /*productAlphaInfo*/) { return false; }
        };

        using AssetBrowserTexturePreviewRequestsBus = AZ::EBus<AssetBrowserTexturePreviewRequests>;
    } // namespace AssetBrowser
} // namespace AzToolsFramework
