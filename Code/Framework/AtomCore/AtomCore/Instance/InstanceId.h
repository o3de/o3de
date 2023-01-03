/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace Data
    {
        /**
         * InstanceId is a unique identifier for an Instance in an InstanceDatabase. Instances
         * are used primarily to control de-duplication of 'instances' created from 'assets'.
         * As a result, this class mirrors the structure of asset id (by including the sub-id)
         * in order to make translation easy. However, the types are not related in order to add
         * some type safety to the system.
         */
        struct InstanceId
        {
            AZ_TYPE_INFO(InstanceId, "{0E59A635-07E8-419F-A0F2-90E0CE9C0AD6}");

            /**
             * Creates an instance id from an asset. The instance id will share the same guid,
             * sub id, and an opaque pointer value to identify a specific version of an asset. This is
             * a create method rather than a constructor in order to make it explicit.
             */
            static InstanceId CreateFromAsset(const Asset<AssetData>& asset);

            /**
             * Creates an instance id from an asset id. The two will share the same guid and
             * sub id. This is a create method rather than a constructor in order
             * to make it explicit.
             */
            static InstanceId CreateFromAssetId(const AssetId& assetId);

            /**
             * Creates an InstanceId by hashing the provided name.
             */
            static InstanceId CreateName(const char* name);

            /**
             * Creates an InstanceId by hashing the provided data.
             */
            static InstanceId CreateData(const void* data, size_t dataSize);

            /**
             * Creates a random InstanceId.
             */
            static InstanceId CreateRandom();

            // Create a null id by default.
            InstanceId() = default;

            explicit InstanceId(const Uuid& guid);
            explicit InstanceId(const Uuid& guid, uint32_t subId);
            explicit InstanceId(const Uuid& guid, uint32_t subId, void* versionId);

            bool IsValid() const;

            bool operator==(const InstanceId& rhs) const;
            bool operator!=(const InstanceId& rhs) const;

            template<class StringType>
            StringType ToString() const;

            template<class StringType>
            void ToString(StringType& result) const;

            Uuid m_guid = Uuid::CreateNull();
            uint32_t m_subId = 0;

            // Pointer used to uniquely identify a version of the data, usually an asset. It is not intended to be
            // dereferenced or used to access asset data.
            void* m_versionId = nullptr; 
        };

        template<class StringType>
        inline StringType InstanceId::ToString() const
        {
            StringType result;
            ToString(result);
            return result;
        }

        template<class StringType>
        inline void InstanceId::ToString(StringType& result) const
        {
            result = StringType::format("%s:%x:%p", m_guid.ToString<StringType>().c_str(), m_subId, m_versionId);
        }
    } // namespace Data
} // namespace AZ

namespace AZStd
{
    // hash specialization
    template<>
    struct hash<AZ::Data::InstanceId>
    {
        using argument_type = AZ::Data::InstanceId;
        using result_type = size_t;

        result_type operator()(const argument_type& id) const
        {
            size_t h = id.m_guid.GetHash();
            hash_combine(h, id.m_subId);
            hash_combine(h, id.m_versionId);
            return h;
        }
    };
} // namespace AZStd
