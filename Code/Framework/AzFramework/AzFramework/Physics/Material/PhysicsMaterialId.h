/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

namespace Physics
{
    //! Class that is used to identify a material.
    struct MaterialId
    {
        AZ_TYPE_INFO(Physics::MaterialId, "{30DED897-0E19-40B7-9BE1-0E92CBD307F9}");

        //! Creates an instance id from an asset id. The two will share the same guid and
        //! sub id. This is an explicit create method rather than a constructor in order
        //! to make it explicit.
        static MaterialId CreateFromAssetId(const AZ::Data::AssetId& assetId);

        //! Creates an InstanceId by hashing the provided name.
        static MaterialId CreateName(const char* name);

        //! Creates an InstanceId by hashing the provided data.
        static MaterialId CreateData(const void* data, size_t dataSize);

        //! Creates a random id.
        static MaterialId CreateRandom();

        //! Create a null id by default.
        MaterialId() = default;

        explicit MaterialId(const AZ::Uuid& guid);
        explicit MaterialId(const AZ::Uuid& guid, uint32_t subId);

        bool IsValid() const;

        bool operator==(const MaterialId& rhs) const;
        bool operator!=(const MaterialId& rhs) const;

        template<class StringType>
        StringType ToString() const;

        template<class StringType>
        void ToString(StringType& result) const;

        AZ::Uuid m_guid = AZ::Uuid::CreateNull();
        uint32_t m_subId = 0;
    };

    template<class StringType>
    inline StringType MaterialId::ToString() const
    {
        StringType result;
        ToString(result);
        return result;
    }

    template<class StringType>
    inline void MaterialId::ToString(StringType& result) const
    {
        result = StringType::format("%s:%x", m_guid.ToString<StringType>().c_str(), m_subId);
    }
} // namespace Physics

namespace AZStd
{
    // hash specialization
    template<>
    struct hash<Physics::MaterialId>
    {
        typedef AZ::Uuid argument_type;
        typedef size_t result_type;
        AZ_FORCE_INLINE size_t operator()(const Physics::MaterialId& id) const
        {
            return id.m_guid.GetHash() ^ static_cast<size_t>(id.m_subId);
        }
    };
} // namespace AZStd
