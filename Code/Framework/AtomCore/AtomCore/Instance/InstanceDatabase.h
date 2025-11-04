/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceData.h>
#include <AtomCore/Instance/InstanceId.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/parallel/shared_mutex.h>


namespace AZStd
{
    class any;
}

namespace AZ
{
    namespace Data
    {
        /**
         * Provides create and delete functions for a specific InstanceData type, for use by @ref InstanceDatabase
         */
        template <typename Type>
        struct InstanceHandler
        {
            /**
             * Creation takes an asset as input and produces a new instance as output.
             * Ownership must be returned to the caller. Use this method to perform
             * both allocation and initialization using the provided asset. The returned
             * instance is assumed to be valid and usable by the client.
             *
             * Usage Examples:
             *  - The user may choose to allocate from a local pool or cache.
             *  - The concrete instance type may have a non-standard initialization path.
             *  - The user may wish to encode global context into the functor (an RHI device, for example).
             *
             *  PERFORMANCE NOTE: Creation is currently done under a lock. Initialization should be quick.
             */
            using CreateFunction = AZStd::function<Instance<Type>(AssetData*)>;

            using CreateFunctionWithParam = AZStd::function<Instance<Type>(AssetData*, const AZStd::any* param)>;

            /**
             * Deletion takes an asset as input and transfers ownership to the method.
             */
            using DeleteFunction = AZStd::function<void(Type*)>;

            /// A function to use when creating an instance.
            ///  The system will assert if both  @m_createFunction and @m_createFunctionWithParam
            ///  creation functions are invalid.
            CreateFunction m_createFunction = nullptr;

            /// A function with an additional custom param to use when creating an instance.
            ///  The system will assert if both  @m_createFunction and @m_createFunctionWithParam
            ///  creation functions are invalid.
            CreateFunctionWithParam m_createFunctionWithParam = nullptr;

            /// [Optional] The function to use when deleting an instance.
            DeleteFunction m_deleteFunction = [](Type* t) { delete t; };
        };

        //! This class exists to allow InstanceData to access parts of InstanceDatabase without having
        //! to know the instance data type, since InstanceDatabase is a template class.
        class InstanceDatabaseInterface
        {
            friend class InstanceData;
        protected:
            virtual void ReleaseInstance(InstanceData* instance) = 0;
        };

        /**
         * This class is a simple database of typed instances. An 'instance' in this context is any class
         * which inherits from InstanceData, is created at runtime from an asset, and has a unique instance
         * id. The purpose of this system is to control de-duplication of instances at runtime, and to 
         * associate instance types with their originating asset types.
         *
         * The database has itself singleton access, but it should be owned by the corresponding system (which
         * is in charge of creation / destruction of the database). To use the database, you may instantiate it
         * using one of the following approaches:
         *  1) Instantiate one InstanceDatabase for each concrente instance class. Use this approach if all
         *     concrete instance classes are known at compile time.
         *  2) Instantiate one InstanceDatabase for a known instance base class, and then register multiple
         *     InstanceHandlers for each concrete instance class. Use this approach if only the instance base
         *     class is known at compile time and the concrete instance classes are only known at runtime.
         *     For example, Atom provides abstract StreamingImageControllerAsset and StreamingImageController
         *     classes, and a game-specific gem can provide custom implementations by adding a handler to
         *     InstanceDatabase<StreamingImageController>.
         *
         * The database allows you to find an instance from its corresponding @ref InstanceId. Alternatively, you
         * can 'find or create' an instance, which will create the instance if it doesn't already exist, or return you the
         * existing instance. The 'find or create' operation takes an asset as input. Instances are designed to be trivially
         * created from their parent asset.
         *
         * The database does NOT own instances. Ownership is returned to you in the form of a smart pointer (Data::Instance<>).
         * This is the same ownership model used by the asset manager.
         *
         * The system is thread-safe. You can create / destroy instances from any thread, however Instances should not be
         * copied between threads, they should always be retrieved from the InstanceDatabase directly.
         *
         * Example Usage (using instantiation approach #1 described above):
         * @code{.cpp}
         *
         * // Create the database.
         * Data::InstanceHandler<MyInstanceType> handler;
         *
         * // Provide your own creator (and optional deleter) to control allocation / initialization of your object.
         * handler.m_createFunction = [] (Data::AssetData* assetData) { return aznew MyInstanceType(assetData); };
         *
         * Data::InstanceDatabase<MyInstanceType>::Create(azrtti_typeid<MyAssetType>(), handler);
         *
         * Data::Asset<MyAssetType> myAsset{ASSETID_1};
         *
         * // Create an instance id from the asset id (1-to-1 mapping).
         * Data::InstanceId instanceId = Data::InstanceId::CreateFromAssetId(myAsset.GetId());
         *
         * // Find or create an instance from an asset.
         * Data::Instance<MyInstanceType> instance = Data::InstanceDatabase<MyInstanceType>::Instance().FindOrCreate(instanceId, myAsset);
         *
         * // Create an instance by name.
         * Data::InstanceId instanceIdName = Data::InstanceId::CreateName("HelloWorld");
         *
         * // Creates a new instance from the same asset (the old instance is de-ref'd).
         * instance = Data::InstanceDatabase<MyInstanceType>::Instance().FindOrCreate(instanceIdName, myAsset);
         *
         * // Finds an existing instance.
         * Data::Instance<MyInstanceType> instance2 = Data::InstanceDatabase<MyInstanceType>::Instance().Find(instanceIdName);
         *
         * instance == instance2; // true
         *
         * // Find or create an existing instance.
         * Data::Instance<MyInstanceType> instance3 = Data::InstanceDatabase<MyInstanceType>::Instance().FindOrCreate(instanceIdName, myAsset);
         *
         * instance == instance2 == instance3; // true
         *
         * // INVALID: Create an instance using a different asset.
         * Data::Asset<MyAssetType> myAsset2{ASSETID_2};
         *
         * // This will assert. You can only request an instance using the SAME asset each time. If the system detects a mismatch it
         * // will throw an error.
         * Data::Instance<MyInstanceType> instance3 = Data::InstanceDatabase<MyInstanceType>::Instance().FindOrCreate(instanceIdName, myAsset2);
         *
         * // After all objects are out of scope! The system will report an error if objects are still active on destruction.
         * Data::InstanceDatabase<MyInstanceType>::Destroy();
         *
         * @endcode
         */
        template <typename Type>
        class InstanceDatabase final : public InstanceDatabaseInterface
        {
            static_assert(AZStd::is_base_of<InstanceData, Type>::value, "Type must inherit from Data::Instance to be used in Data::InstanceDatabase.");
        public:
            AZ_CLASS_ALLOCATOR(InstanceDatabase, AZ::SystemAllocator);

            /**
             * Create the InstanceDatabase with a single handler.
             * Use this function when creating an InstanceDatabase that will handle concrete classes of @ref Type.
             * \param assetType  - All instances will be based on subclasses of this asset type.
             * \param handler    - An InstanceHandler that creates instances of @ref assetType assets.
             * \param checkAssetIds - If true, it will be validated that "instance->m_assetId == asset.GetId()"
             */
            static void Create(const AssetType& assetType, const InstanceHandler<Type>& handler, bool checkAssetIds = true);

            static void Destroy();
            static bool IsReady();
            static InstanceDatabase& Instance();

            /**
             * Attempts to find an instance associated with the provided id. If the instance exists, it
             * is returned. If no instance is found, nullptr is returned. It is safe to call this from
             * multiple threads.
             *
             * @param id The id used to find an instance in the database.
             */
            Data::Instance<Type> Find(const InstanceId& id) const;

            /**
             * Attempts to find an instance associated with the provided id. If it exists, it is returned.
             * Otherwise, it is created using the provided asset data and then returned. It is safe to call
             * this method from multiple threads, even with the same id. The call is synchronous and other threads
             * will block until creation is complete.
             *
             * PERFORMANCE NOTE: If the asset data is not loaded and creation is required, the system will
             * perform a BLOCKING load on the asset. If this behavior is not desired, the user should either
             * ensure the asset is loaded prior to calling this method, or call @ref Find instead.
             *
             * @param id The id used to find or create an instance in the database.
             * @param asset The asset used to initialize the instance, if it does NOT already exist.
             *      If the instance exists, the asset id is checked against the existing instance. If
             *      validation is enabled, the system will error if the created asset id does not match
             *      the provided asset id. It is required that you consistently provide the same asset
             *      when acquiring an instance.
             * @return Returns a smart pointer to the instance, which was either found or created.
             */
            Data::Instance<Type> FindOrCreate(const InstanceId& id, const Asset<AssetData>& asset, const AZStd::any* param = nullptr);

            //! Calls the above FindOrCreate using an InstanceId created from the asset
            Data::Instance<Type> FindOrCreate(const Asset<AssetData>& asset, const AZStd::any* param = nullptr);

            //! Calls FindOrCreate using a random InstanceId
            Data::Instance<Type> Create(const Asset<AssetData>& asset, const AZStd::any* param = nullptr);

            /**
             * Removes the instance data from the database. Does not release it.
             * References to existing instances will remain valid, but new calls to Create/FindOrCreate will create a new instance
             * This function is temporary, to provide functionality needed for Model hot-reloading, but will be removed
             * once the Model class does not need it anymore.
             *
             * @param id The id of the instance to remove
             */
            void TEMPOrphan(const InstanceId& id);

            //! A helper function to visit every instance in the database and calls the provided callback method.
            //! Note: this function can be slow depending on how many instances in the database
            void ForEach(AZStd::function<void(Type&)> callback);
            void ForEach(AZStd::function<void(const Type&)> callback) const;

        private:
            InstanceDatabase(const AssetType& assetType);
            ~InstanceDatabase();

            bool m_checkAssetIds = true;
            //useAssetTypeAsKeyForHandlers;
            static const char* GetEnvironmentName();
            
            Data::Asset<Data::AssetData> LoadAsset(const Data::Asset<AssetData>& asset) const;
            Data::Instance<Type> EmplaceInstance(const InstanceId& id, const Data::Asset<AssetData>& asset, const AZStd::any* param);

            // Utility function called by InstanceData to remove the instance from the database.
            void ReleaseInstance(InstanceData* instance) override;

            void ValidateSameAsset(InstanceData* instance, const Data::Asset<AssetData>& asset) const;

            InstanceHandler<Type> m_instanceHandler;

            // m_database uses a recursive_mutex instead of a shared_mutex because it's possible to recursively
            // create or destroy instances on the same thread while in the midst of creating or destroying an instance.
            mutable AZStd::recursive_mutex m_databaseMutex;
            AZStd::unordered_map<InstanceId, Type*> m_database;

            // There are several classes in Atom, like ShaderResourceGroup, that are not threadsafe
            // because they share the same ShaderResourceGroupPool, so it is important that for each
            // InstanceType there's a mutex that prevents several of those classes from being instantiated
            // simultaneously.
            AZStd::recursive_mutex m_instanceCreationMutex;

            // All instances created by this InstanceDatabase will be for assets derived from this type.
            AssetType m_baseAssetType;

            static EnvironmentVariable<InstanceDatabase*> ms_instance;
        };

        template<typename Type>
        EnvironmentVariable<InstanceDatabase<Type>*>
            InstanceDatabase<Type>::ms_instance = nullptr;

        template<typename Type>
        InstanceDatabase<Type>::~InstanceDatabase()
        {
#ifdef AZ_DEBUG_BUILD
            for (const auto& keyValue : m_database)
            {
                const InstanceId& instanceId = keyValue.first;
                const AZStd::string& stringValue = instanceId.ToString<AZStd::string>();
                AZ_Printf("InstanceDatabase", "\tLeaked Instance: %s\n", stringValue.c_str());
            }
#endif

            AZ_Error(
                "InstanceDatabase", m_database.empty(),
                "AZ::Data::%s still has active references.", Type::GetDatabaseName());
        }

        template<typename Type>
        Data::Instance<Type> InstanceDatabase<Type>::Find(const InstanceId& id) const
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_databaseMutex);
            auto iter = m_database.find(id);
            if (iter != m_database.end())
            {
                return iter->second;
            }
            return nullptr;
        }

        template<typename Type>
        Data::Instance<Type> InstanceDatabase<Type>::FindOrCreate(
            const InstanceId& id, const Asset<AssetData>& asset, const AZStd::any* param)
        {
            if (!id.IsValid())
            {
                return nullptr;
            }

            // Try to find the entry
            {
                AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_databaseMutex);
                auto iter = m_database.find(id);
                if (iter != m_database.end())
                {
                    InstanceData* data = static_cast<InstanceData*>(iter->second);
                    ValidateSameAsset(data, asset);
                    
                    return iter->second;
                }
            }

            // Take a reference so we can mutate it.
            Data::Asset<Data::AssetData> assetLocal = LoadAsset(asset);

            // Failed to load the asset
            if (!assetLocal.IsReady())
            {
                return nullptr;
            }

            return EmplaceInstance(id, assetLocal, param);
        }

        template<typename Type>
        Data::Instance<Type> InstanceDatabase<Type>::FindOrCreate(const Asset<AssetData>& asset, const AZStd::any* param)
        {
            return FindOrCreate(Data::InstanceId::CreateFromAsset(asset), asset, param);
        }

        template<typename Type>
        Data::Instance<Type> InstanceDatabase<Type>::Create(const Asset<AssetData>& asset, const AZStd::any* param)
        {
            const InstanceId id = InstanceId::CreateRandom();

            Data::Asset<AssetData> assetLocal = LoadAsset(asset);

            // Failed to load the asset
            if (!assetLocal.IsReady())
            {
                return nullptr;
            }

            return EmplaceInstance(id, assetLocal, param);
        }

        template<typename Type>
        void InstanceDatabase<Type>::TEMPOrphan(const InstanceId& id)
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_databaseMutex);
            // Check if the instance is still in the database, in case it was orphaned twice
            auto instanceItr = m_database.find(id);
            if (instanceItr != m_database.end())
            {
                // Mark the instance as orphaned, and remove it from the database
                instanceItr->second->m_isOrphaned = true;
                m_database.erase(instanceItr);
            }
        }

        template<typename Type>
        Data::Asset<Data::AssetData> InstanceDatabase<Type>::LoadAsset(const Data::Asset<AssetData>& asset) const
        {
            // Take a reference so we can mutate it.
            Data::Asset<Data::AssetData> assetLocal = asset;
            if (!assetLocal.IsReady())
            {
                assetLocal.QueueLoad();

                if (assetLocal.IsLoading())
                {
                    assetLocal.BlockUntilLoadComplete();
                }
            }
            return assetLocal;
        }

        template<typename Type>
        Data::Instance<Type> InstanceDatabase<Type>::EmplaceInstance(
            const InstanceId& id, const Data::Asset<AssetData>& asset, const AZStd::any* param)
        {
            // It's very important to have m_databaseMutex unlocked while an instance is being created because
            // there can be cases like in StreamingImage(s), where multiple threads are involved and some of those threads
            // attempt to release a StreamingImage, which in turn will lock m_databaseMutex and it could incurr
            // in potential deadlocks.

            // If the instance was created redundantly, it will be temporarily stored here for destruction
            // before this function returns.
            Data::Instance<Type> redundantInstance = nullptr;

            // It's possible for the m_createFunction call to recursively trigger another FindOrCreate call, so be aware that
            // the contents of m_database may change within this call.
            Data::Instance<Type> instance = nullptr;

            {
                AZStd::scoped_lock<decltype(m_instanceCreationMutex)> lock(m_instanceCreationMutex);
                if (!param)
                {
                    instance = m_instanceHandler.m_createFunction(asset.Get());
                }
                else
                {
                    instance = m_instanceHandler.m_createFunctionWithParam(asset.Get(), param);
                }
            }

            // Lock the database. There's still a chance that the same instance was created in parallel.
            // in such case we return the first one that made it into the database and gracefully release the
            // redundant one.
            if (instance)
            {
                AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_databaseMutex);
                auto iter = m_database.find(id);
                if (iter != m_database.end())
                {
                    InstanceData* data = static_cast<InstanceData*>(iter->second);
                    ValidateSameAsset(data, asset);
                    redundantInstance = instance; // Will be destroyed as soon as we return from this function.
                    instance = iter->second;
                }
                else
                {
                    instance->m_id = id;
                    instance->m_parentDatabase = this;
                    instance->m_assetId = asset.GetId();
                    instance->m_assetType = asset.GetType();
                    m_database.emplace(id, instance.get());
                }
            }

            return instance;
        }

        template<typename Type>
        void InstanceDatabase<Type>::ReleaseInstance(InstanceData* instance)
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_databaseMutex);
            
            const int prevUseCount = instance->m_useCount.fetch_sub(1);
            AZ_Assert(prevUseCount >= 1, "m_useCount is negative");
            if (prevUseCount > 1)
            {
                // This instance is still being used.
                return;
            }
        
            // If instanceId doesn't exist in m_database that means the instance was already deleted on another thread.
            // We check and make sure the pointers match before erasing, just in case some other InstanceData was created with the same ID.
            // We re-check the m_useCount in case some other thread requested an instance from the database after we decremented m_useCount.
            // We change m_useCount to -1 to be sure another thread doesn't try to clean up the instance (though the other checks
            // probably cover that).
            auto instanceId = instance->GetId();
            auto instanceItr = m_database.find(instanceId);
            int32_t expectedRefCount = 0;
            if (instanceItr != m_database.end() &&
                instanceItr->second == instance &&
                instance->m_useCount.compare_exchange_strong(expectedRefCount, -1))
            {
                m_database.erase(instanceId);
                m_instanceHandler.m_deleteFunction(static_cast<Type*>(instance));
            }
            else if (instance->m_isOrphaned && instance->m_useCount.compare_exchange_strong(expectedRefCount, -1))
            {
                // If the instance was orphaned, it has already been removed from the database,
                // but still needs to be deleted when the refcount drops to 0
                m_instanceHandler.m_deleteFunction(static_cast<Type*>(instance));
            }
        }

        template<typename Type>
        void InstanceDatabase<Type>::ValidateSameAsset(
            InstanceData* instance, const Data::Asset<AssetData>& asset) const
        {
            /**
             * The following validation layer is disabled in release, but is designed to catch a couple related edge cases
             * that might result in difficult to track bugs.
             *  - The user provides an id that collides with a different id.
             *  - The user attempts to provide a different asset when requesting the same instance id.
             *
             * In either case, the probable result is that an instance is returned that does not match the asset id provided
             * by the caller, which is not valid and probably not what the user expected. The validation layer will throw an
             * error to alert them.
             */

        #if defined (AZ_DEBUG_BUILD)
            if (m_checkAssetIds)
            {
                AZ_Error(
                    "InstanceDatabase", (instance->m_assetId == asset.GetId()),
                    "InstanceDatabase::FindOrCreate found the requested instance, but a different asset was used to create it. "
                    "Instances of a specific id should be acquired using the same asset. Either make sure the instance id "
                    "is actually unique, or that you are using the same asset each time for that particular id.");
            }
        #else
            AZ_UNUSED(instance);
            AZ_UNUSED(asset);
        #endif
        }

        template<typename Type>
        InstanceDatabase<Type>::InstanceDatabase(const AssetType& assetType) 
            : m_baseAssetType(assetType)
        {
        }

        template<typename Type>
        void InstanceDatabase<Type>::Create(const AssetType& assetType, const InstanceHandler<Type>& handler, bool checkAssetIds)
        {
            AZ_Assert(!ms_instance || !ms_instance.Get(), "InstanceDatabase already created!");

            if (!ms_instance)
            {
                ms_instance = Environment::CreateVariable<InstanceDatabase<Type>*>(GetEnvironmentName());
            }

            if (!ms_instance.Get())
            {
                ms_instance.Set(aznew InstanceDatabase<Type>(assetType));
            }

            AZ_Assert(handler.m_createFunction || handler.m_createFunctionWithParam, "At least one create function must be valid");
            ms_instance.Get()->m_instanceHandler = handler;
            ms_instance.Get()->m_checkAssetIds = checkAssetIds;
        }

        template<typename Type>
        void InstanceDatabase<Type>::Destroy()
        {
            AZ_Assert(ms_instance, "InstanceDatabase not created!");
            delete (*ms_instance);
            *ms_instance = nullptr;
        }

        template<typename Type>
        bool InstanceDatabase<Type>::IsReady()
        {
            if (!ms_instance)
            {
                ms_instance = Environment::FindVariable<InstanceDatabase*>(GetEnvironmentName());
            }

            return ms_instance && *ms_instance;
        }

        template<typename Type>
        InstanceDatabase<Type>& InstanceDatabase<Type>::Instance()
        {
            if (!ms_instance)
            {
                ms_instance = Environment::FindVariable<InstanceDatabase*>(GetEnvironmentName());
            }

            AZ_Assert(ms_instance && *ms_instance, "InstanceDatabase<%s> has not been initialized yet.", AzTypeInfo<Type>::Name());
            return *(*ms_instance);
        }

        template<typename Type>
        const char* InstanceDatabase<Type>::GetEnvironmentName()
        {
            static_assert(
                HasInstanceDatabaseName<Type>::value,
                "All classes used as instances in an InstanceDatabase need to define AZ_INSTANCE_DATA in the class.");
            return Type::GetDatabaseName();
        }
                
        template <typename Type>
        void InstanceDatabase<Type>::ForEach(AZStd::function<void(Type&)> callback)
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_databaseMutex);
            for (auto element : m_database)
            {
                callback(*element.second);
            }
        }
        
        template <typename Type>
        void InstanceDatabase<Type>::ForEach(AZStd::function<void(const Type&)> callback) const
        {
            AZStd::scoped_lock<AZStd::recursive_mutex> lock(m_databaseMutex);
            for (auto element : m_database)
            {
                callback(*element.second);
            }
        }
    }
}
