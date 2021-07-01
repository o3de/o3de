/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
             * Creates an instance id from an asset id. The two will share the same guid and
             * sub id. This is an explicit create method rather than a constructor in order
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

            bool IsValid() const;

            bool operator==(const InstanceId& rhs) const;
            bool operator!=(const InstanceId& rhs) const;

            template<class StringType>
            StringType ToString() const;

            template<class StringType>
            void ToString(StringType& result) const;

            Uuid m_guid = Uuid::CreateNull();
            uint32_t  m_subId = 0;
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
            result = StringType::format("%s:%x", m_guid.ToString<StringType>().c_str(), m_subId);
        }
    }
}

namespace AZStd
{
    // hash specialization
    template <>
    struct hash<AZ::Data::InstanceId>
    {
        typedef AZ::Uuid    argument_type;
        typedef size_t      result_type;
        AZ_FORCE_INLINE size_t operator()(const AZ::Data::InstanceId& id) const
        {
            return id.m_guid.GetHash() ^ static_cast<size_t>(id.m_subId);
        }
    };
}
