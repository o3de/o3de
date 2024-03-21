/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#if defined(CARBONATED)
        /**
         * Loads a specific bundle
         */
        virtual void LoadBundle([[maybe_unused]] const char* bundlePath, [[maybe_unused]] bool reload) {}
        /**
         * Generates a list of bundles to load
         */
        virtual AZStd::vector<AZStd::string> GenerateBundlesToLoadList([[maybe_unused]] const char* baseFolder, [[maybe_unused]] const char* fileExtension) { return AZStd::vector<AZStd::string>(); }
#endif

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
