/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string_view.h>
#include <AzFramework/Spawnable/Spawnable.h>

namespace AzFramework
{
    //! A utility class to make it easier to work with individual spawnables.
    //! This class will monitor a spawnable for changes and provides a simplified interface to
    //! respond to.
    class SpawnableMonitor
        : public AZ::Data::AssetBus::MultiHandler
    {
    public:
        enum IssueType
        {
            Error,
            Cancel,
            ReloadError
        };

        SpawnableMonitor() = default;
        //! Constructs the monitor to immediately start monitoring a specific spawnable. Depending on the state of the
        //! spawnable this can result in immediate calls to OnSpawnable* functions.
        explicit SpawnableMonitor(AZ::Data::AssetId spawnableAssetId);
        //! Destroys the monitor, including disconnecting from the spawnable if connected.
        virtual ~SpawnableMonitor();

        //! Connects the monitor to start monitoring a specific spawnable. Depending on the state of the spawnable this
        //! can result in immediate calls to OnSpawnable* functions. If the monitor is already connected this will do
        //! nothing and return false.
        virtual bool Connect(AZ::Data::AssetId spawnableAssetId);
        //! Disconnects from the monitor if connected and otherwise does nothing and returns false.
        virtual bool Disconnect();

        //! Returns true if the monitor is connected and otherwise false.
        [[nodiscard]] virtual bool IsConnected() const;
        //! Returns true if the monitor tracked the spawnable able being ready for use, otherwise false.
        [[nodiscard]] virtual bool IsLoaded() const;

    protected:
        //! Called when a spawnable has been loaded and is ready for used.
        virtual void OnSpawnableLoaded();
        //! Called when an spawnable has been unloaded and should no longer be used.
        virtual void OnSpawnableUnloaded();
        //! Called when a spawnable has been reloaded. Note that if the replacement isn't used to replace
        //! the original spawnable then the replacement asset will eventually run out of references and send
        //! an OnSpawnableUnloaded event.
        virtual void OnSpawnableReloaded(AZ::Data::Asset<Spawnable>&& replacementAsset);
        //! Called when an error occurred while the Asset Manager did an operation on the spawnable.
        virtual void OnSpawnableIssue(IssueType issueType, AZStd::string_view message);

    private:
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetUnloaded(const AZ::Data::AssetId assetId, const AZ::Data::AssetType assetType) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetCanceled(AZ::Data::AssetId assetId) override;
        void OnAssetReloadError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        bool m_isConnected{ false };
        bool m_isLoaded{ false };
    };
} // namespace AzFramework
