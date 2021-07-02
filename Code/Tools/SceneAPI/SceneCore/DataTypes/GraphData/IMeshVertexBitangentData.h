/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>

namespace AZ
{
    class Vector3;
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {

            class IMeshVertexBitangentData
                : public IGraphObject
            {
            public:                                
                AZ_RTTI(IMeshVertexBitangentData, "{6C8F6109-B0BD-49D1-A998-4A4946557DF9}", IGraphObject);

                virtual ~IMeshVertexBitangentData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual size_t GetCount() const = 0;
                virtual const AZ::Vector3& GetBitangent(size_t index) const = 0;
                virtual void SetBitangent(size_t vertexIndex, const AZ::Vector3& bitangent) = 0;
                virtual void SetBitangentSetIndex(size_t setIndex) = 0;
                virtual size_t GetBitangentSetIndex() const = 0;
                virtual TangentSpace GetTangentSpace() const = 0;
                virtual void SetTangentSpace(TangentSpace space) = 0;
            };

        }  // DataTypes
    }  // SceneAPI
}  // AZ
