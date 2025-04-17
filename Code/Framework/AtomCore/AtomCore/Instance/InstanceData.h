/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomCore/Instance/InstanceId.h>
#include <AtomCore/Instance/Instance.h>

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    namespace Data
    {
        class InstanceDatabaseInterface;

        /**
         * InstanceData is an intrusive smart pointer base class for any class utilizing an InstanceDatabase.
         * To use a class in an InstanceDatabase, you must inherit from InstanceData, and in your concrete class,
         * define the AZ_INSTANCE_DATA() macro.
         *
         * InstanceData is compatible with AZStd::intrusive_ptr. The pointer is also typedef'd to AZ::Data::Instance<>
         * to mirror AZ::Data::Asset<>.
         *
         * Each instance data is associated with an instance id and an asset id. These id's are only valid if the instance
         * is created from an InstanceDatabase, otherwise they are null. It is valid to create a derived instance data class
         * without using the InstanceDatabase, but the ids will all be null.
         *
         * By default, if the instance database did not create the instance, InstanceData will call 'delete' on itself when
         * the reference count hits zero. If the instance database was the creator, it will assign a custom deleter. You
         * do not have access to this deleter.
         */
        class InstanceData
        {
        public:
            AZ_RTTI(InstanceData, "{3B728818-A765-4749-A3A6-0C960E4DD65E}");

            virtual ~InstanceData() = default;

            /**
             * Returns the id which uniquely identifies the instance in the instance database. If
             * the concrete class was created outside of the database, the id is null.
             */
            const InstanceId& GetId() const;

            /**
             * Returns the asset id used to create the instance.
             */
            const AssetId& GetAssetId() const;
            
            /**
             * Returns the asset type used to create the instance.
             */
            const AssetType& GetAssetType() const;

        protected:
            InstanceData() = default;

        private:
            ///////////////////////////////////////////////////////////////////
            // IntrusivePtrCountPolicy template overrides
            void add_ref();
            void release();
            ///////////////////////////////////////////////////////////////////

            template <typename Type>
            friend struct AZStd::IntrusivePtrCountPolicy;

            template<typename Type>
            friend class InstanceDatabase;

            // Pointer to the InstanceDatabase that owns this instance. Will be null if the InstanceData object
            // is not held in an InstanceDatabase.
            InstanceDatabaseInterface* m_parentDatabase = nullptr;
            
            AZStd::atomic_int m_useCount = {0};
            
            // The id which uniquely identifies the instance.
            InstanceId m_id;

            // Tracks the asset id used to create the instance.
            AssetId m_assetId;

            // Tracks the asset type used to create the instance.
            AssetType m_assetType;
            
            // Boolean to indicate if the instance has been orphaned from the instance database
            bool m_isOrphaned = false;
        };

        /// @cond EXCLUDE_DOCS
        AZ_HAS_STATIC_MEMBER(InstanceDatabaseName, GetDatabaseName, const char*, ());
        /// @endcond

        /**
         * Declares a concrete instance class. This macro is required if the instance is used in an InstanceDatabase.
         * The class must derive from AZ::Data::InstanceData. The class may not be templated.
         *
         * AZ_INSTANCE_DATA(_InstanceClass, _ClassGUID, OtherBaseClasses...) AZ::Data::InstanceData is included automatically.
         */
    #define AZ_INSTANCE_DATA(_InstanceClass, ...)                               \
        AZ_RTTI(_InstanceClass, __VA_ARGS__, AZ::Data::InstanceData)            \
        static const char* GetDatabaseName()                                    \
        {                                                                       \
            return "InstanceDatabase<" #_InstanceClass ">";                     \
        }
    }
}
