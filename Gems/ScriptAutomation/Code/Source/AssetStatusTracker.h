/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Asset/AssetSystemBus.h>

namespace ScriptAutomation
{
    //! Utility for tracking status of assets being built by the asset processor so scripts can insert delays
    class AssetStatusTracker final
       : public AzFramework::AssetSystemInfoBus::Handler
    {
    public:
        ~AssetStatusTracker();

        //! Starts tracking asset status updates from the Asset Processor.
        //! Clears any asset status information already collected.
        //! Clears any asset expectations that were added by ExpectAsset().
        void StartTracking();

        //! Sets the AssetStatusTracker to expect a particular asset with specific expected results.
        //! Note this can be called multiple times with the same assetPath, in which case the expected counts will be added.
        //! @param sourceAssetPath the source asset path, relative to the watch folder. Will be normalized and matched case-insensitive.
        //! @param expectedCount number of completed jobs expected for this asset.
        void ExpectAsset(AZStd::string sourceAssetPath, uint32_t expectedCount = 1);

        //! Returns whether all of the expected assets have finished.
        bool DidExpectedAssetsFinish() const;

        //! Return a list of assets that have not completed expected processing.
        AZStd::vector<AZStd::string> GetIncompleteAssetList() const;

        //! Stops tracking asset status updates from the Asset Processor. Clears any asset status information already collected.
        void StopTracking();

    private:

        // Tracks the number of times various events occur
        struct AssetStatusEvents
        {
            uint32_t m_started = 0;
            uint32_t m_succeeded = 0;
            uint32_t m_failed = 0;
            uint32_t m_expectedCount = 0;
        };

        // AssetSystemInfoBus overrides...
        void AssetCompilationStarted(const AZStd::string& assetPath) override;
        void AssetCompilationSuccess(const AZStd::string& assetPath) override;
        void AssetCompilationFailed(const AZStd::string& assetPath) override;

        bool m_isTracking = false;

        AZStd::unordered_map<AZStd::string /*asset path*/, AssetStatusEvents> m_allAssetStatusData;
        mutable AZStd::mutex m_mutex;
    };
} // namespace ScriptAutomation
