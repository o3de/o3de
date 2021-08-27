#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISkinWeightData
                : public IGraphObject
            {
            public:
                AZ_RTTI(ISkinWeightData, "{F7A6CC37-5904-4D25-B1A9-B25C192A4C64}", IGraphObject);

                struct Link
                {
                    int boneId;
                    float weight;
                };

                virtual ~ISkinWeightData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual size_t GetVertexCount() const = 0;
                virtual size_t GetLinkCount(size_t vertexIndex) const = 0;
                virtual const Link& GetLink(size_t vertexIndex, size_t linkIndex) const = 0;
                virtual size_t GetBoneCount() const = 0;
                virtual const AZStd::string& GetBoneName(int boneId) const = 0;
            };
        }  // DataTypes
    }  // SceneAPI
}  // AZ

namespace AZStd
{
    template<>
    struct hash<AZ::SceneAPI::DataTypes::ISkinWeightData::Link>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::SceneAPI::DataTypes::ISkinWeightData::Link& value) const
        {
            result_type hash = 0;

            hash_combine(hash, value.boneId);
            hash_combine(hash, value.weight);

            return hash;
        }
    };
}
