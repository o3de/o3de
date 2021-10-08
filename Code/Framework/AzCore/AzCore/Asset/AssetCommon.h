/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/typetraits/is_base_of.h>
#include <AzCore/Debug/AssetTracking.h>
#include <AzCore/IO/Streamer/FileRequest.h>

namespace AZ
{
    class AssetSerializer;
    class AssetEventHandler;
    class ReflectContext;
    class SerializeContext;

    namespace Data
    {
        class AssetContainer;
        class AssetManager;
        class AssetData;
        class AssetEntry;
        class AssetHandler;

        template<class T>
        class Asset;

        typedef Uuid AssetType;

        namespace AssetInternal
        {
            bool IsValidAssetType(const AssetType& type, AZ::SerializeContext* serializeContext = nullptr);
        }

        /**
         * Asset ID types
         */
        typedef AssetData*  AssetPtr;

        struct AssetId
        {
            AZ_TYPE_INFO(AssetId, "{652ED536-3402-439B-AEBE-4A5DBC554085}");

            /// Create asset id in invalid state
            AssetId();
            AssetId(const Uuid& guid, u32 subId = 0);

            bool IsValid() const;

            void SetInvalid(); // or just can just do m_assetId = AssetId();

            bool operator==(const AssetId& rhs) const;
            bool operator!=(const AssetId& rhs) const;
            bool operator<(const AssetId& rhs) const;

            enum class SubIdDisplayType
            {
                Hex,
                Decimal
            };

            template<class StringType>
            StringType ToString(SubIdDisplayType displayType = SubIdDisplayType::Hex) const;

            template<class StringType>
            void ToString(StringType& result, SubIdDisplayType displayType = SubIdDisplayType::Hex) const;

            static AssetId CreateString(AZStd::string_view input);
            static void Reflect(ReflectContext* context);

            Uuid m_guid;
            u32  m_subId;   ///< To allow easier and more consistent asset guid, we can provide asset sub ID. (i.e. Guid is a cubemap texture, subId is the index of the side)
            // Explicitly define and clear the set of pad bytes in this struct.
            // Currently, there are only pad bytes at the end, because of the struct alignment caused by m_guid.
            // Any changes to this structure may require more pad byte arrays to be interspersed in the struct, and/or
            // changes to the expression below to calculate the number of pad bytes occurring at the end.
            // The assumptions used here are checked by static asserts in the AssetId() constructor.
            static constexpr size_t s_maxMemberAlignment = alignof(decltype(m_guid));
            static constexpr size_t s_lastMemberSize = sizeof(decltype(m_subId));
            u8 m_padBytes[s_maxMemberAlignment - (s_lastMemberSize % s_maxMemberAlignment)]{};
        };

        /**
         * Constants
         */
        static const AssetType  s_invalidAssetType = AZ::Uuid::CreateNull();
        static const int  s_defaultCreationToken = -1;

        /**
         * Base class for all asset types.
         */
        class AssetData
        {
            template<class T>
            friend class Asset;
            friend class AssetManager;
            friend class AssetHandler;

        public:
            enum class AssetStatus : int
            {
                NotLoaded,          ///< Asset has not been loaded, and is not in the process of loading.
                Queued,             ///< Asset has a job created for loading it which has not begun processing
                StreamReady,        ///< Asset Stream loader has finished, ready for Job to begin loading
                Loading,            ///< Asset is currently in the process of loading.
                LoadedPreReady,     ///< Asset data and preload dependencies are loaded. Handler init is about to be called and onassetready will be signaled
                ReadyPreNotify,     ///< Asset is loaded and ready for use. AssetBus hasn't yet dispatched the OnAssetReady event on the main thread.
                Ready,              ///< Asset is loaded and ready for use.
                Error,              ///< Asset attempted to load, but it or a strict dependency failed.
            };

            AZ_CLASS_ALLOCATOR(AssetData, SystemAllocator, 0);
            AZ_RTTI(AssetData, "{AF3F7D32-1536-422A-89F3-A11E1F5B5A9C}");

            AssetData(const AssetId& assetId = AssetId(), AssetStatus status = AssetStatus::NotLoaded)
                : m_status(status)
                , m_assetId(assetId)
            {}

            virtual ~AssetData();

            static void Reflect(ReflectContext* context);

            void Acquire();
            void Release();

            void AcquireWeak();
            void ReleaseWeak();

            /// Asset is loaded and ready for use.
            /// Note that the asset may be ready for use before the OnAssetReady
            /// event has been dispatched by the AssetBus on the main thread.
            AZ_FORCE_INLINE bool IsReady() const
            {
                AssetStatus status = GetStatus();
                return (status == AssetStatus::Ready || status == AssetStatus::ReadyPreNotify);
            }

            /// @return True if the asset status is Error or Canceled
            AZ_FORCE_INLINE bool IsError() const { return GetStatus() == AssetStatus::Error; }
            bool IsLoading(bool includeQueued = true) const;
            AZ_FORCE_INLINE AssetStatus GetStatus() const { return m_status.load(); }
            AZ_FORCE_INLINE const AssetId& GetId() const { return m_assetId; }
            AZ_FORCE_INLINE const AssetType& GetType() const { return RTTI_GetType(); }
            AZ_FORCE_INLINE int GetUseCount() const { return m_useCount.load(); }

        protected:
            /**
             * Override this function do create a new Asset Instance for each AssetDatababse::GetAsset(),
             * Most assets should be read only and shared (thus registered in the database), make sure you understand
             * the consequences of changing behavior. When this function returns false, this asset will NOT be added to the
             * loaded assets map (which is why when we call AssetDatabase::GetAsset it will not be found a new will be create)
             */
            virtual bool IsRegisterReadonlyAndShareable() { return true; }

            /**
             * Override this function to control automatic reload behavior. 
             * By default, the asset will reload automatically.
             * Return false to disable automatic reload. Potential use cases include:
             * 1, If an asset is dependent on a parent asset(i.e.both assets need to be reloaded as a group) the parent asset can explicitly reload the child.
             * 2, The asset does not support reload.
             * Note: Disabling an auto reload means that the asset in memory will be stale compared to what triggered the reload
             * This moves the responsibility of loading the asset or editing/handling the stale asset onto the system that disabled the reload
             */
            virtual bool HandleAutoReload() { return true; }

            enum class AssetDataFlags : AZ::u32
            {
                Requeue = 0
            };

            bool GetFlag(const AssetDataFlags& checkFlag) const;
            void SetFlag(const AssetDataFlags& checkFlag, bool setValue);

            bool GetRequeue() const;
            void SetRequeue(bool requeue);

            void RegisterWithHandler(AssetHandler* assetHandler);
            void UnregisterWithHandler();

            AssetData(const AssetData&) = delete;
            AZStd::atomic_int m_useCount{ 0 };
            AZStd::atomic_int m_weakUseCount{ 0 };
            AZStd::atomic<AssetStatus> m_status;
            AssetId m_assetId;

            AssetHandler* m_registeredHandler{ nullptr };

            // This is used to identify a unique asset and should only be set by the asset manager 
            // and therefore does not need to be atomic.
            // All shared copy of an asset should have the same identifier and therefore
            // should not be modified while making copy of an existing asset. 
            int m_creationToken = s_defaultCreationToken;
            // General purpose flags that should only be accessed within the asset mutex
            AZStd::bitset<32> m_flags;
        };

        /**
         * Setting for each reference (Asset<T>) to control loading of referenced assets during serialization.
         */
        AZ_ENUM_WITH_UNDERLYING_TYPE(AssetLoadBehavior, u8,
            (PreLoad, 0),          ///< Serializer will "Pre load" dependencies, asset containers may load in parallel but will not signal AssetReady
            (QueueLoad, 1),        ///< Serializer will queue an asynchronous load of the referenced asset and return the object to the user. User code should use the \ref AZ::Data::AssetBus to monitor for when it's ready.
            (NoLoad, 2),           ///< Serializer will load reference information, but asset loading will be left to the user. User code should call Asset<T>::QueueLoad and use the \ref AZ::Data::AssetBus to monitor for when it's ready.
                                   ///< AssetContainers will skip NoLoad dependencies
            Count,
            (Default, QueueLoad)
        );

        struct AssetFilterInfo
        {
            AssetId m_assetId;
            AssetType m_assetType;
            AssetLoadBehavior m_loadBehavior;

            AssetFilterInfo(const AssetId& id, const AssetType& assetType, AssetLoadBehavior loadBehavior);
            explicit AssetFilterInfo(const Asset<AssetData>& asset);
        };

        /// Can be provided as a predicate for asset filtering or visiting.
        using AssetFilterCB = AZStd::function<bool(const AssetFilterInfo& filterInfo)>;

        enum class AssetDependencyLoadRules
        {
            // Default will store "NoLoad" dependencies in a separate list which can be loaded on demand
            UseLoadBehavior = 0,
            Default = UseLoadBehavior,
            // LoadAll will ignore "NoLoad" dependencies and load everything
            LoadAll = 1
        };

        struct AssetLoadParameters
        {
            AssetLoadParameters() : m_assetLoadFilterCB() {}

            explicit AssetLoadParameters(const AssetFilterCB& assetFilterCallback)
                : m_assetLoadFilterCB(assetFilterCallback)
            {
            }
            AssetLoadParameters(const AssetFilterCB& assetFilterCallback, const AssetDependencyLoadRules& dependencyRules)
                : m_assetLoadFilterCB(assetFilterCallback),
                m_dependencyRules(dependencyRules)
            {
            }
            AssetFilterCB m_assetLoadFilterCB{ nullptr };
            AZStd::optional<AZStd::chrono::milliseconds> m_deadline{ };
            AZStd::optional<IO::IStreamerTypes::Priority> m_priority{ };
            AssetDependencyLoadRules m_dependencyRules{ AssetDependencyLoadRules::Default };
            // If the asset we're requesting is already loaded and we don't want to check for any
            // depenencies that need loading, leave this as true.  If you wish to force a clean evaluation
            // for dependent assets set to false
            bool m_reloadMissingDependencies{ false };
            bool operator==(const AssetLoadParameters& rhs) const
            {
                // We can only check if a filter is empty, they can't be compared to each other
                // So only load parameters with empty filters can ever be equal
                const bool emptyLoadFilter = m_assetLoadFilterCB.operator!();
                const bool rhsEmptyLoadFilter = rhs.m_assetLoadFilterCB.operator!();

                return emptyLoadFilter
                    && rhsEmptyLoadFilter
                    && m_deadline == rhs.m_deadline
                    && m_priority == rhs.m_priority;
            }
        };

        /**
         * Asset is a smart pointer to an actual asset.
         * Asset users should always refer to assets using
         * this object.
         */
        template<class T>
        class Asset
        {
            typedef Asset ThisType;

            friend class AssetManager;
            friend class AZ::AssetSerializer;
            friend class AZ::AssetEventHandler;

            template<typename U>
            friend class Asset;

        public:
            /// Create asset with default params (no asset bounded)
            /// By default, referenced assets will be queued for load during serialization.
            /// Use \ref AssetLoadBehavior to control this behavior.
            Asset(AssetLoadBehavior loadBehavior = AssetLoadBehavior::Default);
            /// Create an asset from a valid asset data (created asset), might not be loaded or currently loading.
            Asset(AssetData* assetData, AssetLoadBehavior loadBehavior);
            /// Create an asset from a valid asset data (created asset) and set the asset id for both, might not be loaded or currently loading.
            Asset(const AZ::Data::AssetId& id, AssetData* assetData, AssetLoadBehavior loadBehavior);
            /// Initialize asset pointer with id, type, and hint. No data construction will occur until QueueLoad is called.
            Asset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type, const AZStd::string& hint = AZStd::string());

            Asset(const Asset& rhs);
            template<typename U>
            Asset(const Asset<U>& rhs);

            Asset(Asset&& rhs);
            Asset& operator=(Asset&& rhs);
            ~Asset();

            Asset& operator=(const Asset& rhs);

            explicit operator bool() const
            {
                return m_assetData != nullptr;
            }

            T& operator*() const
            {
                AZ_Assert(m_assetData, "Asset is not loaded");
                return *Get();
            }

            T* operator->() const
            {
                AZ_Assert(m_assetData, "Asset is not loaded");
                return Get();
            }

            bool IsReady() const;               ///< Is the asset data ready (loaded)?
            bool IsError() const;               ///< Did an error occur when loading the asset?
            bool IsLoading() const;             ///< Is the asset currently loading?
            AssetData::AssetStatus GetStatus() const; ///< What is the current load status of the asset?

            const AssetId& GetId() const;       ///< Retrieve the Id of the referenced asset.
            const AssetType& GetType() const;   ///< Retrieve the type of the referenced asset (if available).
            const AZStd::string& GetHint() const; ///< Retrieve the last known path to the asset Id this ref is bound to
            void SetHint(const AZStd::string& newHint);

            /// Returns asset data - base class
            AssetData* GetData() const;

            /// Returns pointer to the asset type, or null if asset is not loaded
            T* Get() const;

            /// Return asset data as T type, uses azrtti for the cast
            template<class U>
            U* GetAs() const;

            /**
            * Create an asset with an ID (type is inferred). If there is a current asset it will be released.
            * \param id \ref AssetId
            * \param queueLoad if true it will call "QueueLoad" to load the asset data.
            */
            bool Create(const AssetId& id, bool queueLoad = false);

            /**
            * Create an asset with an ID and loadBehavior (type is inferred). If there is a current asset it will be released.
            * \param id \ref AssetId
            * \param loadBehavior \ref AssetLoadBehavior
            * \param queueLoad if true it will call "QueueLoad" to load the asset data.
            */
            bool Create(const AssetId& id, AssetLoadBehavior loadBehavior, bool queueLoad = false);

            /**
             * If the asset is not loaded and has a valid id, it will be queued for load.
             * \returns true if the load was queued (you can listen on the AssetBus or check with IsReady)
             * and false if the queue of loading did not happen. There are multiple reasons for this and you can
             * test in case of error. Current failure conditions:
             * - Invalid id
             */
            bool QueueLoad(const AssetLoadParameters& loadParams = {});

            AssetData::AssetStatus BlockUntilLoadComplete();

            /**
             * Fully resets the Asset to a clean initialized state.
             * The asset data is released, the asset ID is reset, the hint is cleared, etc.
             */
            bool Reset();

            /**
             * Releases reference on asset data, if one is held.
             * The asset ID, hint, etc are all still preserved.
             * \return true if a reference was held, and therefore released.
             */
            bool Release();

            /**
             * Reloads an asset if an asset is create.
             * \returns true if reload is triggered, otherwise false if an asset is not created (ie. We don't have asset ID to reload)
             */
            bool Reload();

            /**
             * Save asset, triggers a SaveAsset of possible \ref AssetDatabase::SaveAsset.
             */
            bool Save();

            /**
             * Sets the auto load behavior for the asset reference.
             * \param \ref AssetLoadBehavior
             */
            void SetAutoLoadBehavior(AssetLoadBehavior loadBehavior);

            /**
             * \return auto load behavior for the asset reference.
             */
            AssetLoadBehavior GetAutoLoadBehavior() const;

            /// Returns asset instance flags
            /// Deprecated - use GetAutoLoadBehavior
            u8 GetFlags() const;

            /// You can change the flags only when we don't have an asset bound (ie. GetData() == null)
            /// Deprecated - use SetAutoLoadBehavior
            bool SetFlags(u8 flags);

            /**
            * Upgrades legacy Ids/hints with the new canonical Ids/hints.
            * This will search for the assetId and hint in the catalog
            * and if found, will update it to be the new, canonical one(s)
            */
            void UpgradeAssetInfo();

            /** 
            * for debugging purposes - creates a string that represents the assets id, subid, hint, and name.
            * You should use this function for any time you want to show the full details of an asset in a log message
            * as it will always produce a consistent output string.  By convention, don't surround the output of this call
            * with any kind of decorator, (for example, don't format like [%s]) because decorators are built-in.
            */
            template<class StringType>
            StringType ToString() const;

            /**
            * for debugging purposes only - updates a string that represents the asset's id, subid, hint and name.
            * see comment details on StringType ToString() above.
            */
            template<class StringType>
            void ToString(StringType& result) const;

        protected:

            void SetData(AssetData* assetData);

            void swap(Asset& rhs);

            AssetId m_assetId;                      ///< Id of asset the pointer currently references. QueueLoad will acquire/load the actual asset.
            AssetType m_assetType;                  ///< Referenced asset type.
            AssetData* m_assetData = nullptr;       ///< Pointer to the asset data, it's always present when we have bound an asset (it doesn't have to be loaded)
            AssetLoadBehavior m_loadBehavior;       ///< Load behavior for the asset reference
            AZStd::string m_assetHint;              ///< Last known path to the asset m_assetId refers to
        private:
            static void ResetAsset(Asset& asset);
        };

        template<typename T>
        void Asset<T>::ResetAsset(Asset<T>& asset)
        {
            AZ_Assert(
                asset.m_assetData == nullptr, "m_assetData is not null. "
                "Ensure either Release or Reset are called first to free the AssetData reference");

            asset.m_assetId = AssetId();
            asset.m_assetType = AssetType::CreateNull();
            asset.m_assetData = nullptr;
            asset.m_loadBehavior = AssetLoadBehavior::Default;
            asset.m_assetHint = AZStd::string();
        }

        template<typename T, typename U>
        Asset<T> static_pointer_cast(const Asset<U>& p) { return Asset<T>(p); }

        //=========================================================================
        template<class T>
        template<class StringType>
        inline StringType Asset<T>::ToString() const
        {
            StringType result;
            ToString(result);
            return result;
        }

        //=========================================================================
        template<class T>
        template<class StringType>
        inline void Asset<T>::ToString(StringType& result) const
        {
            if (m_assetHint.empty())
            {
                result = StringType::format("[AssetId=%s Type=%s]", m_assetId.ToString<StringType>().c_str(), GetType().template ToString<StringType>().c_str());
            }

            result = StringType::format("['%s' AssetId=%s Type=%s]", m_assetHint.c_str(), m_assetId.ToString<StringType>().c_str(), GetType().template ToString<StringType>().c_str());
        }

        namespace AssetInternal
        {
            Asset<AssetData> FindOrCreateAsset(const AssetId& id, const AssetType& type, AssetLoadBehavior assetReferenceLoadBehavior);
            Asset<AssetData> GetAsset(const AssetId& id, const AssetType& type, AssetLoadBehavior assetReferenceLoadBehavior,
                const AZ::Data::AssetLoadParameters& assetLoadFilterCB = AssetLoadParameters{});
            AssetData::AssetStatus BlockUntilLoadComplete(const Asset<AssetData>& asset);
            void UpdateAssetInfo(AssetId& id, AZStd::string& assetHint);
            bool ReloadAsset(AssetData* assetData, AssetLoadBehavior assetReferenceLoadBehavior);
            bool SaveAsset(AssetData* assetData, AssetLoadBehavior assetReferenceLoadBehavior);
            Asset<AssetData> GetAssetData(const AssetId& id, AssetLoadBehavior assetReferenceLoadBehavior);
            AssetId ResolveAssetId(const AssetId& id);
        }

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
                        auto curStatus = assetData->GetStatus();
                        bool isError = assetData->IsError();
                        connectLock.unlock();
                        if (curStatus == AssetData::AssetStatus::Ready)
                        {
                            handler->OnAssetReady(assetData);
                        }
                        else if (isError)
                        {
                            handler->OnAssetError(assetData);
                        }
                    }
                }
            };
            template<typename Bus>
            using ConnectionPolicy = AssetConnectionPolicy<Bus>;

            using EventProcessingPolicy = Debug::AssetTrackingEventProcessingPolicy<>;
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

        typedef EBus<AssetEvents> AssetBus;

        /*
         * AssetBusCallbacks is a utility class that maps AssetBus events to user callbacks
         */
        class AssetBusCallbacks
            : public AssetBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(AssetBusCallbacks, AZ::SystemAllocator, 0);

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

        /**
        * Implementations
        */

        /**
        * AssetID
        */

        //=========================================================================
        inline AssetId::AssetId()
            : m_guid(AZ::Uuid::CreateNull())
            , m_subId(0)
        {
            // Verify that the correct number of pad bytes have been added to the structure.
            // i.e. The structure should be the exact size of all the members added together, including
            // our choice of padding.  If it's ever different, it means we need to adjust the amount of padding
            // that's manually added into the structure.
            static_assert(sizeof(AssetId) == (sizeof(m_guid) + sizeof(m_subId) + sizeof(m_padBytes)));
        }

        //=========================================================================
        inline AssetId::AssetId(const Uuid& guid, u32 subId)
            : m_guid(guid)
            , m_subId(subId)
        {
        }

        //=========================================================================
        template<class StringType>
        inline StringType AssetId::ToString(SubIdDisplayType displayType) const
        {
            StringType result;
            ToString(result, displayType);
            return result;
        }

        //=========================================================================
        template<class StringType>
        inline void AssetId::ToString(StringType& result, SubIdDisplayType displayType) const
        {
            switch (displayType)
            {
            case SubIdDisplayType::Hex:
                result = StringType::format("%s:%x", m_guid.ToString<StringType>().c_str(), m_subId);
                break;
            case SubIdDisplayType::Decimal:
                result = StringType::format("%s:%d", m_guid.ToString<StringType>().c_str(), m_subId);
                break;
            }
        }

        //=========================================================================
        inline bool AssetId::IsValid() const
        {
            return m_guid != AZ::Uuid::CreateNull();
        }

        //=========================================================================
        inline void AssetId::SetInvalid()
        {
            m_guid = AZ::Uuid::CreateNull();
            m_subId = 0;
        }

        //=========================================================================
        inline bool AssetId::operator == (const AssetId& rhs) const
        {
            return m_guid == rhs.m_guid && m_subId == rhs.m_subId;
        }

        //=========================================================================
        inline bool AssetId::operator != (const AssetId& rhs) const
        {
            return m_guid != rhs.m_guid || m_subId != rhs.m_subId;
        }

        //=========================================================================
        inline bool AssetId::operator < (const AssetId& rhs) const
        {
            if (m_guid == rhs.m_guid)
            {
                return m_subId < rhs.m_subId;
            }
            return m_guid < rhs.m_guid;
        }

        //=========================================================================
        template<class T, class U>
        inline bool operator==(const Asset<T>& lhs, const Asset<U>& rhs)
        {
            return lhs.GetId() == rhs.GetId(); /// Id is enough considering all ids are globally unique
        }

        //=========================================================================
        template<class T, class U>
        inline bool operator!=(const Asset<T>& lhs, const Asset<U>& rhs)
        {
            return lhs.GetId() != rhs.GetId(); /// Id is enough considering all ids are globally unique
        }

        /**
         * Asset<T>
         */
        //=========================================================================
        template<class T>
        Asset<T>::Asset(AssetLoadBehavior loadBehavior)
            : m_assetType(azrtti_typeid<T>())
            , m_loadBehavior(loadBehavior)
        {
            static_assert((AZStd::is_base_of<AssetData, T>::value), "Can only specify desired type if asset type is AssetData");
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(AssetData* assetData, AssetLoadBehavior loadBehavior)
            : m_assetType(azrtti_typeid<T>())
            , m_loadBehavior(loadBehavior)
        {
            SetData(assetData);
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(const AssetId& id, AssetData* assetData, AssetLoadBehavior loadBehavior)
            : m_assetId(id)
            , m_assetType(azrtti_typeid<T>())
            , m_loadBehavior(loadBehavior)
        {
            AZ_Assert(!assetData->m_assetId.IsValid(), "Asset data already has an ID set.");
            assetData->m_assetId = id;
            SetData(assetData);
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(const AssetId& id, const AZ::Data::AssetType& type, const AZStd::string& hint)
            : m_assetId(id)
            , m_assetType(type)
            , m_loadBehavior(AssetLoadBehavior::Default)
            , m_assetHint(hint)
        {
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(const Asset& rhs)
            : m_assetId(rhs.m_assetId)
            , m_assetType(rhs.m_assetType)
            , m_loadBehavior(rhs.m_loadBehavior)
            , m_assetHint(rhs.m_assetHint)
        {
            SetData(rhs.m_assetData);
        }

        //=========================================================================
        template<class T>
        template<typename U>
        Asset<T>::Asset(const Asset<U>& rhs)
            : m_assetId(rhs.m_assetId)
            , m_assetType(rhs.m_assetType)
            , m_loadBehavior(rhs.m_loadBehavior)
            , m_assetHint(rhs.m_assetHint)
        {
            SetData(rhs.m_assetData);
        }

        //=========================================================================
        template<class T>
        Asset<T>::Asset(Asset&& rhs)
        {
            if (this != &rhs)
            {
                *this = AZStd::move(rhs);
            }
        }

        //=========================================================================
        template<class T>
        Asset<T>& Asset<T>::operator=(Asset&& rhs)
        {
            if (this != &rhs)
            {
                Release();

                // there is an edge case here, where the RHS refers to the same valid asset (id) but is missing the asset hint
                // in that specific case we don't want to wipe away the hint we might have.
                bool sameAsset = m_assetId.IsValid() && (rhs.m_assetId == m_assetId);
                bool preserveAssetHint = (sameAsset && rhs.m_assetHint.empty());

                m_assetId = AZStd::move(rhs.m_assetId);
                m_assetType = AZStd::move(rhs.m_assetType);
                m_assetData = AZStd::exchange(rhs.m_assetData, nullptr);
                m_loadBehavior = rhs.m_loadBehavior;

                if (!preserveAssetHint)
                {
                    // if we're not preserving the asset hint, go ahead and move from the rhs.
                    m_assetHint = AZStd::move(rhs.m_assetHint);
                }

                ResetAsset(rhs);
            }
            return *this;
        }

        //=========================================================================
        template<class T>
        Asset<T>::~Asset()
        {
            if (m_assetData)
            {
                m_assetData->Release();
            }
        }

        //=========================================================================
        template<class T>
        Asset<T>& Asset<T>::operator=(const Asset& rhs)
        {
            ThisType(rhs).swap(*this);
            return *this;
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Reset()
        {
            const bool released = Release();
            ResetAsset(*this);
            return released;
        }

        //=========================================================================
        template<class T>
        bool  Asset<T>::IsReady() const
        {
            return m_assetData && m_assetData->IsReady();
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::IsError() const
        {
            return m_assetData && m_assetData->IsError();
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::IsLoading() const
        {
            return m_assetData && m_assetData->IsLoading();
        }

        //=========================================================================
        template<class T>
        AssetData::AssetStatus Asset<T>::GetStatus() const
        {
            return m_assetData ? m_assetData->GetStatus() : AssetData::AssetStatus::NotLoaded;
        }

        //=========================================================================
        template<class T>
        const AssetId& Asset<T>::GetId() const
        {
            return m_assetId;
        }

        //=========================================================================
        template<class T>
        const AssetType& Asset<T>::GetType() const
        {
            return m_assetType;
        }

        //=========================================================================
        template<class T>
        AssetData* Asset<T>::GetData() const
        {
            return m_assetData;
        }

        template <class T>
        const AZStd::string& Asset<T>::GetHint() const
        {
            return m_assetHint;
        }

        template <class T>
        void Asset<T>::SetHint(const AZStd::string& newHint)
        {
            m_assetHint = newHint;
        }

        //=========================================================================
        template<class T>
        T* Asset<T>::Get() const
        {
            return static_cast<T*>(m_assetData);
        }

        //=========================================================================
        template<class T>
        template<class U>
        U* Asset<T>::GetAs() const
        {
            return azrtti_cast<U*>(m_assetData);
        }

        //=========================================================================
        template<class T>
        void Asset<T>::SetAutoLoadBehavior(AssetLoadBehavior loadBehavior)
        {
            m_loadBehavior = loadBehavior;
        }

        //=========================================================================
        template<class T>
        AssetLoadBehavior Asset<T>::GetAutoLoadBehavior() const
        {
            return m_loadBehavior;
        }

        //=========================================================================
        template<class T>
        u8 Asset<T>::GetFlags() const
        {
            AZ_Warning("Asset", false, "Deprecated - replaced by GetAutoLoadBehavior");
            return static_cast<u8>(m_loadBehavior);
        }

        //=========================================================================
        template<class T>
        void Asset<T>::UpgradeAssetInfo()
        {
            if (!m_assetId.IsValid())
            {
                return;
            }

            // called Internal to de-templatize and also avoid inclusion of AssetManager
            // into this header.
            AssetInternal::UpdateAssetInfo(m_assetId, m_assetHint);
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::SetFlags(u8 flags)
        {
            AZ_Warning("Asset", false, "Deprecated - replaced by SetAutoLoadBehavior");
            if (!m_assetData)
            {
                AZ_Assert(flags < static_cast<u8>(AssetLoadBehavior::Count), "Flags value is out of range");
                m_loadBehavior = static_cast<AssetLoadBehavior>(flags);
                return true;
            }
            return false;
        }

        //=========================================================================
        template<class T>
        void Asset<T>::SetData(AssetData* assetData)
        {
            // Validate the data type matches or derives from T, or bail
            if (assetData && !assetData->RTTI_IsTypeOf(AzTypeInfo<T>::Uuid()))
            {
#ifdef AZ_ENABLE_TRACING
                char assetDataIdGUIDStr[Uuid::MaxStringBuffer];
                char assetTypeIdGUIDStr[Uuid::MaxStringBuffer];
                assetData->GetId().m_guid.ToString(assetDataIdGUIDStr, AZ_ARRAY_SIZE(assetDataIdGUIDStr));
                AzTypeInfo<T>::Uuid().ToString(assetTypeIdGUIDStr, AZ_ARRAY_SIZE(assetTypeIdGUIDStr));
                AZ_Error("AssetDatabase", false, "Asset of type %s:%x (%s) is not related to %s (%s)!",
                    assetData->GetType().ToString<AZStd::string>().c_str(), assetData->GetId().m_subId, assetDataIdGUIDStr,
                    AzTypeInfo<T>::Name(), assetTypeIdGUIDStr);
#endif // AZ_ENABLE_TRACING
                m_assetId = AssetId();
                m_assetType = azrtti_typeid<T>();
                m_assetHint.clear();
                return;
            }

            // Acquire new data first, then release old data
            if (assetData)
            {
                assetData->Acquire();
                m_assetId = assetData->GetId();
                m_assetType = assetData->RTTI_GetType();
                UpgradeAssetInfo();
            }
            if (m_assetData)
            {
                m_assetData->Release();
            }
            m_assetData = assetData;
        }

        //=========================================================================
        template<class T>
        void Asset<T>::swap(Asset& rhs)
        {
            bool sameAsset = m_assetId.IsValid() && (rhs.m_assetId == m_assetId);

            AZStd::swap(m_assetId, rhs.m_assetId);
            AZStd::swap(m_assetType, rhs.m_assetType);
            AZStd::swap(m_assetData, rhs.m_assetData);
            AZStd::swap(m_loadBehavior, rhs.m_loadBehavior);

            // the asset hint is useful forensically - its more of a cached value than
            // part of the data that we are swapping
            // if we are referring to the exact same actual asset, we'd prefer to preserve the asset hint
            // on both sides of the swap.  We want as many Asset<T> hint fields to be filled in with useful
            // forensic data.
            if ((sameAsset) && (rhs.m_assetHint.empty()))
            {
                rhs.m_assetHint = m_assetHint;
            }
            else if ((sameAsset) && (m_assetHint.empty()))
            {
                m_assetHint = rhs.m_assetHint;
            }
            else
            {
                // if we are a different asset (or being swapped with a empty) then we just swap as usual.
                AZStd::swap(m_assetHint, rhs.m_assetHint);
            }
            
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::QueueLoad(const AssetLoadParameters& loadParams)
        {
            const AssetLoadBehavior loadBehavior = GetAutoLoadBehavior();
            bool queueLoadSuccessful = false;

            // If we have a valid asset ID but no asset, either find the existing asset or create a new structure to load it into.
            if (!m_assetData && m_assetId.IsValid())
            {
                auto asset = AssetInternal::FindOrCreateAsset(m_assetId, m_assetType, loadBehavior);

                // Only assign on success so we don't lose the assetId/assetType if the above failed
                if(asset)
                {
                    *this = asset;
                }
            }

            // If we either already had valid asset data, or just created it via FindOrCreateAsset, try to queue the load.
            if (m_assetData && m_assetData->GetId().IsValid())
            {
                // Try to queue if the asset isn't already loading or loaded.
                // Also try to queue if the asset *is* already loading or loaded, but we're the only one with a strong reference
                // (i.e. use count == 1), because that means it was in the process of being garbage-collected.
                if ((m_assetData->GetStatus() == AZ::Data::AssetData::AssetStatus::NotLoaded) ||
                    (m_assetData->GetUseCount() == 1))
                {
                    *this = AssetInternal::GetAsset(m_assetData->GetId(), m_assetData->GetType(), loadBehavior, loadParams);
                }

                // If the asset made it to a loading or ready state, consider the request successful.
                if (m_assetData->IsLoading() || m_assetData->IsReady())
                {
                    queueLoadSuccessful = true;
                }
            }

            return queueLoadSuccessful;
        }

        template <class T>
        AssetData::AssetStatus Asset<T>::BlockUntilLoadComplete()
        {
            return AssetInternal::BlockUntilLoadComplete(*this);
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Release()
        {
            if (m_assetData)
            {
                SetData(nullptr);
                return true;
            }

            return false;
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Create(const AssetId& id, bool queueLoad)
        {
            if (id != GetId())
            {
                const AssetLoadBehavior loadBehavior = GetAutoLoadBehavior();
                return Create(id, loadBehavior, queueLoad);
            }

            return true; // already created
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Create(const AssetId& id, AssetLoadBehavior loadBehavior, bool queueLoad)
        {
            if (id != GetId())
            {
                // Acquire new asset. This will release the current asset.
                *this = AssetInternal::FindOrCreateAsset(id, m_assetType, loadBehavior);

                if (queueLoad)
                {
                    QueueLoad(AssetLoadParameters(nullptr));
                }
            }

            return true; // already created
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Reload()
        {
            if (m_assetData && m_assetData->GetId().IsValid())
            {
                return AssetInternal::ReloadAsset(m_assetData, m_loadBehavior);
            }

            return false;
        }

        //=========================================================================
        template<class T>
        bool Asset<T>::Save()
        {
            if (m_assetData && m_assetData->GetId().IsValid())
            {
                return AssetInternal::SaveAsset(m_assetData, m_loadBehavior);
            }

            return false;
        }
        //=========================================================================
        // Built-in asset helper filters.
        //=========================================================================

        /// Indiscriminately skips all asset references.
        bool AssetFilterNoAssetLoading(const AssetFilterInfo& filterInfo);

        // Shared ProductDependency concepts between AP and LY 
        namespace ProductDependencyInfo
        {
            //! Corresponds to all ProductDependencyFlags, not just LoadBehaviors
            enum class ProductDependencyFlagBits : AZ::u8
            {
                // LoadBehavior bits correspond to AssetLoadBehavior which has values 0-3
                // so uses the first two bits here
                LoadBehaviorLow = 0,
                LoadBehaviorHigh = 1,
                // use additional bits starting here
                Unused
            };
            using ProductDependencyFlags = AZStd::bitset<64>;
            AZ::Data::AssetLoadBehavior LoadBehaviorFromFlags(const ProductDependencyFlags& dependencyFlags);
            AZ::Data::ProductDependencyInfo::ProductDependencyFlags CreateFlags(AZ::Data::AssetLoadBehavior autoLoadBehavior);
        } // namespace ProductDependencyInfo
    }  // namespace Data

    AZ_TYPE_INFO_SPECIALIZE(Data::AssetLoadBehavior, "{DAF9ECED-FEF3-4D7A-A220-8CFD6A5E6DA1}");
    AZ_TYPE_INFO_TEMPLATE_WITH_NAME(AZ::Data::Asset, "Asset", "{C891BF19-B60C-45E2-BFD0-027D15DDC939}", AZ_TYPE_INFO_CLASS);

}   // namespace AZ

namespace AZStd
{
    // hash specialization
    template <>
    struct hash<AZ::Data::AssetId>
    {
        typedef AZ::Uuid    argument_type;
        typedef size_t      result_type;
        AZ_FORCE_INLINE size_t operator()(const AZ::Data::AssetId& id) const
        {
            // use the subId here because otherwise you suffer performance problems if one source has a lot of products (same guid, varying subid)
            return id.m_guid.GetHash() ^ static_cast<size_t>(id.m_subId);
        }
    };
}

