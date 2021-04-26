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

namespace LmbrCentral
{
    /**
     * Bundling System Requests
     */
    class BundlingSystemRequests
        : public AZ::EBusTraits
    {
    public:

        /**
         * Load bundles with a given file extension such as .pak from a given folder
         */
        virtual void LoadBundles([[maybe_unused]] const char* baseFolder, [[maybe_unused]] const char* fileExtension) {}
        /**
        * Unload bundles loaded through the LoadBundles call
        */
        virtual void UnloadBundles() {}

        virtual size_t GetOpenedBundleCount() const { return 0; }
    };

    using BundlingSystemRequestBus = AZ::EBus<BundlingSystemRequests>;

} // namespace LmbrCentral
