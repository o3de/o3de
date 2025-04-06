/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/AzCoreAPI.h>

namespace AZ
{
    namespace Data
    {
        /**
         * AssetEvents
         * All parameters are passed by value because events can be queued.
         */
        class AssetEvents
            : public EBusTraits
        {
            friend class AssetManager;
        public:
            AZ_RTTI(AssetEvents, "{5EA1F709-A268-4C60-BEC0-7CFEED3BA114}");

            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            typedef AZStd::recursive_mutex  MutexType;
            typedef AssetId                 BusIdType;
            static const bool EnableEventQueue = true;

            /**
            * Custom connection policy to make sure all we are fully in sync
            */
            template <class Bus>
            struct AssetConnectionPolicy
                : public EBusConnectionPolicy<Bus>
            {
                static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
                {
                    // It's possible for users to open a level while the AP is still processing and as such not all legacy asset ids were known at the time
                    // of loading. Check now to see if the assets have been compiled and find the actual asset id to connect to.
                    typename Bus::BusIdType actualId = AssetInternal::ResolveAssetId(id);
                    EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, actualId);

                    // If the asset is ready, notify this handler manually.
                    // It is safe to do this because we know that if the asset
                    // has already been marked ready, then the event has already
                    // finished firing or otherwise we would not have been able
                    // to lock the bus mutex
                    Asset<AssetData> assetData(AssetInternal::GetAssetData(actualId, AZ::Data::AssetLoadBehavior::Default));
                    if (assetData)
                    {
                        auto isReady = assetData->GetStatus() == AssetData::AssetStatus::Ready;
                        bool isError = assetData->IsError();

                        if (isReady || isError)
                        {
                            connectLock.unlock();

                            if (isReady)
                            {
                                handler->OnAssetReady(assetData);
                            }
                            else if (isError)
                            {
                                handler->OnAssetError(assetData);
                            }

                            // Lock the mutex again since some destructors will be modifying the context afterwards
                            connectLock.lock();
                        }
                    }
                }
            };
            template<typename Bus>
            using ConnectionPolicy = AssetConnectionPolicy<Bus>;

            //////////////////////////////////////////////////////////////////////////

            virtual ~AssetEvents() {}

            /// Called when an asset is loaded, patched and ready to be used.
            virtual void OnAssetReady(Asset<AssetData> asset) { (void)asset; }

            /// Called when an asset has been moved (usually due to de-fragmentation/compaction), if possible the only data pointer is provided otherwise NULL.
            virtual void OnAssetMoved(Asset<AssetData> asset, void* oldDataPointer) { (void)asset; (void)oldDataPointer; }

            /// Called before an asset reload has started.
            virtual void OnAssetPreReload(Asset<AssetData> asset) { (void)asset; }

            /// Called when an asset has been reloaded (usually in tool mode and loose more). It should not be called in final build.
            virtual void OnAssetReloaded(Asset<AssetData> asset) { (void)asset; }

            /// Called when an asset failed to reload.
            virtual void OnAssetReloadError(Asset<AssetData> asset) { (void)asset; }

            /// Called when an asset has been saved. In general most assets can't be saved (in a game) so make sure you check the flag.
            virtual void OnAssetSaved(Asset<AssetData> asset, bool isSuccessful) { (void)asset; (void)isSuccessful; }

            /// Called when an asset is unloaded.
            virtual void OnAssetUnloaded(const AssetId assetId, const AssetType assetType) { (void)assetId; (void)assetType; }

            /**
            * Called when an error happened with an asset. When this message is received the asset should be considered broken by default.
            * Note that this can happen when the asset errors during load, but also happens when the asset is missing (not in catalog etc.)
            * in the case of an asset that is completely missing, the Asset<T> passed in here will have no hint or other information about
            * the asset since completely missing assets are not registered in the asset manager's database or the catalog.
            */
            virtual void OnAssetError(Asset<AssetData> asset) { (void)asset; }

            /// Called when an asset load is canceled.  This can happen when there are no remaining references to an asset which hasn't finished loading
            virtual void OnAssetCanceled(AssetId assetId) { (void)assetId; }

            /// When an asset is loaded as part of a container this signal is sent when all assets within the container are ready
            virtual void OnAssetContainerReady([[maybe_unused]] Asset<AssetData> asset) { }
        };

        using AssetBus = EBus<AssetEvents>;

        /*
         * AssetBusCallbacks is a utility class that maps AssetBus events to user callbacks
         */
        class AssetBusCallbacks
            : public AssetBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetBusCallbacks, AZ::SystemAllocator);

            using AssetReadyCB = AZStd::function<void (Asset<AssetData> /*asset*/, AssetBusCallbacks& /*callbacks*/)>;
            using AssetMovedCB = AZStd::function<void (Asset<AssetData> /*asset*/, void* /*oldDataPointer*/, AssetBusCallbacks& /*callbacks*/)>;
            using AssetReloadedCB = AZStd::function<void (Asset<AssetData> /*asset*/, AssetBusCallbacks& /*callbacks*/)>;
            using AssetSavedCB = AZStd::function<void (Asset<AssetData> /*asset*/, bool /*isSuccessful*/, AssetBusCallbacks& /*callbacks*/)>;
            using AssetUnloadedCB = AZStd::function<void (const AssetId& /*assetId*/, const AssetType& /*assetType*/, AssetBusCallbacks& /*callbacks*/)>;
            using AssetErrorCB = AZStd::function<void (Asset<AssetData> /*asset*/, AssetBusCallbacks& /*callbacks*/)>;
            using AssetCanceledCB = AZStd::function<void(const AssetId& /*assetId*/, AssetBusCallbacks& /*callbacks*/)>;

            void SetCallbacks(const AssetReadyCB& readyCB, const AssetMovedCB& movedCB, const AssetReloadedCB& reloadedCB,
                              const AssetSavedCB& savedCB, const AssetUnloadedCB& unloadedCB, const AssetErrorCB& errorCB,
                              const AssetCanceledCB& canceledCB);
            void ClearCallbacks();

            void SetOnAssetReadyCallback(const AssetReadyCB& readyCB);
            void SetOnAssetMovedCallback(const AssetMovedCB& movedCB);
            void SetOnAssetReloadedCallback(const AssetReloadedCB& reloadedCB);
            void SetOnAssetSavedCallback(const AssetSavedCB& savedCB);
            void SetOnAssetUnloadedCallback(const AssetUnloadedCB& unloadedCB);
            void SetOnAssetErrorCallback(const AssetErrorCB& errorCB);
            void SetOnAssetCanceledCallback(const AssetCanceledCB& canceledCB);

            void OnAssetReady(Asset<AssetData> asset) override;
            void OnAssetMoved(Asset<AssetData> asset, void* oldDataPointer) override;
            void OnAssetReloaded(Asset<AssetData> asset) override;
            void OnAssetSaved(Asset<AssetData> asset, bool isSuccessful) override;
            void OnAssetUnloaded(const AssetId assetId, const AssetType assetType) override;
            void OnAssetError(Asset<AssetData> asset) override;
            void OnAssetCanceled(const AssetId assetId) override;

        protected:
            AssetReadyCB    m_onAssetReadyCB;
            AssetMovedCB    m_onAssetMovedCB;
            AssetReloadedCB m_onAssetReloadedCB;
            AssetSavedCB    m_onAssetSavedCB;
            AssetUnloadedCB m_onAssetUnloadedCB;
            AssetErrorCB    m_onAssetErrorCB;
            AssetCanceledCB m_onAssetCanceledCB;
        };

    }  // namespace Data
}   // namespace AZ

AZ_DECLARE_EBUS_EXTERN_MULTI_ADDRESS(AZ::Data::AssetEvents);
