/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include<Atom/RHI/DeviceDrawItem.h>
#include<AtomCore/Instance/InstanceId.h>

namespace AZ
{
    namespace Render
    {
        //! Represents all the data needed to know if a mesh can be instanced
        struct MeshInstanceGroupKey
        {
            Data::InstanceId m_modelId = Data::InstanceId::CreateFromAssetId(Data::AssetId(Uuid::CreateNull(), 0));
            uint32_t m_lodIndex = 0;
            uint32_t m_meshIndex = 0;
            Data::InstanceId m_materialId = Data::InstanceId::CreateFromAssetId(Data::AssetId(Uuid::CreateNull(), 0));
            // If anything needs to force instancing off (e.g., if the shader it uses doesn't support instancing),
            // it can set a random uuid here to force it to get a unique key
            Uuid m_forceInstancingOff = Uuid::CreateNull();
            RHI::DrawItemSortKey m_sortKey = 0;

            bool operator<(const MeshInstanceGroupKey& rhs) const;

            bool operator==(const MeshInstanceGroupKey& rhs) const;

            bool operator!=(const MeshInstanceGroupKey& rhs) const;

        private:
            auto MakeTie() const;
        };
    } // namespace Render
} // namespace AZ

namespace AZStd
{
    // hash specialization
    template<>
    struct hash<AZ::Render::MeshInstanceGroupKey>
    {
        using argument_type = AZ::Render::MeshInstanceGroupKey;
        using result_type = size_t;

        result_type operator()(const argument_type& key) const
        {
            size_t h = 0;
            hash_combine(h, key.m_modelId);
            hash_combine(h, key.m_lodIndex);
            hash_combine(h, key.m_meshIndex);
            hash_combine(h, key.m_materialId);
            hash_combine(h, key.m_forceInstancingOff);
            hash_combine(h, key.m_sortKey);
            return h;
        }
    };
} // namespace AZStd
