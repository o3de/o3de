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
        public:
            friend typename AZStd::hash<AZ::Data::InstanceId>;

            AZ_TYPE_INFO(InstanceId, "{0E59A635-07E8-419F-A0F2-90E0CE9C0AD6}");

            /**
             * Support assigning multiple sub IDs for additional uniqueness between instances. For asset related instances, the first two
             * sub IDs are reserved for the asset id sub id and the creation token, if available. Otherwise, any unreserved or unused sub
             * IDs can be assigned according to the needs of the system managing instances.
             */
            static constexpr const int SubIdCapacity = 4;
            using SubIdContainer = AZStd::fixed_vector<uint32_t, SubIdCapacity>;

            /**
             * Creates an instance id from an asset. The GUID will be copied from the asset id. The first sub id will be the asset id sub
             * id. The second sub id will be the asset creation token, or AZ::Data::s_defaultCreationToken if it is not available. Any
             * additional sub IDs will be added after that.
             */
            static InstanceId CreateFromAsset(const Asset<AssetData>& asset, const SubIdContainer& subIds = {});

            /**
             * Creates an instance id from an asset id. The GUID will be copied from the asset id. The first sub id will be the asset id sub
             * id. The second sub id will be AZ::Data::s_defaultCreationToken so that it is uniform with CreateFromAsset. Any additional sub
             * IDs will be added after that.
             */
            static InstanceId CreateFromAssetId(const AssetId& assetId, const SubIdContainer& subIds = {});

            /**
             * Creates an InstanceId with a GUID constructed from the name string. Additional sub IDs can be provided by the system creating
             * the instances.
             */
            static InstanceId CreateName(const char* name, const SubIdContainer& subIds = {});

            /**
             * Creates an InstanceId with a GUID constructed from the data buffer. Additional sub IDs can be provided by the system creating
             * the instances.
             */
            static InstanceId CreateData(const void* data, size_t dataSize, const SubIdContainer& subIds = {});

            /**
             * Creates an InstanceId with a GUID constructed from the UUID. Additional sub IDs can be provided by the system creating
             * the instances.
             */
            static InstanceId CreateUuid(const AZ::Uuid& guid, const SubIdContainer& subIds = {});

            /**
             * Creates a random InstanceId.
             */
            static InstanceId CreateRandom();

            // Create a null id by default.
            InstanceId() = default;

            bool IsValid() const;

            bool operator<(const InstanceId& rhs) const;
            bool operator==(const InstanceId& rhs) const;
            bool operator!=(const InstanceId& rhs) const;

            template<class StringType>
            StringType ToString() const;

            template<class StringType>
            void ToString(StringType& result) const;

            const Uuid& GetGuid() const;

        private:
            explicit InstanceId(const Uuid& guid, const SubIdContainer& subIds);

            Uuid m_guid = Uuid::CreateNull();

            SubIdContainer m_subIds;
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
            StringType combinedStr = m_guid.ToString<StringType>();
            for (const auto subId : m_subIds)
            {
                combinedStr += StringType::format(":%x", subId);
            }
            result = combinedStr;
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
            size_t combinedHash = id.m_guid.GetHash();
            for (const auto subId : id.m_subIds)
            {
                hash_combine(combinedHash, subId);
            }
            return combinedHash;
        }
    };
} // namespace AZStd
