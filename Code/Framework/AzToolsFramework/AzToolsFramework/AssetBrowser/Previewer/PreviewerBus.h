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

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class PreviewerFactory;

        //! Public requests to previewer
        class PreviewerRequests
            : public AZ::EBusTraits
        {
        public:
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;

            //! Get previewer factory that can handle provided entry if one exists
            virtual const PreviewerFactory* GetPreviewerFactory(const AssetBrowserEntry* entry) const = 0;

            //! Priority of current request handler when multiple handlers for the same entry instance exist
            //! Higher priority takes precedence
            virtual int GetPreviewerPriority() const { return 0; }

            //! Called by ebus to sort request handlers by their priority
            bool Compare(const PreviewerRequests* other) const 
            {
                return GetPreviewerPriority() > other->GetPreviewerPriority(); // higher comes first!
            }
        };

        using PreviewerRequestBus = AZ::EBus<PreviewerRequests>;
    } // namespace AssetBrowser
} // namespace AzToolsFramework