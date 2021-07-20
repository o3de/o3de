#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IBlendShapeData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IBlendShapeData, "{55E7384D-9333-4C51-BC91-E90CAC2C30E2}", IGraphObject);

                struct Face
                {
                    unsigned int vertexIndex[3];
                    inline bool operator==(const Face& rhs) const
                    {
                        return (vertexIndex[0] == rhs.vertexIndex[0] && vertexIndex[1] == rhs.vertexIndex[1] &&
                            vertexIndex[2] == rhs.vertexIndex[2]);
                    }

                    inline bool operator!=(const Face& rhs) const
                    {
                        return (vertexIndex[0] != rhs.vertexIndex[0] || vertexIndex[1] != rhs.vertexIndex[1] ||
                            vertexIndex[2] != rhs.vertexIndex[2] );
                    }
                };

                virtual ~IBlendShapeData() override = default;

                void CloneAttributesFrom([[maybe_unused]] const IGraphObject* sourceObject) override {}

                virtual size_t GetUsedControlPointCount() const = 0;
                virtual int GetControlPointIndex(int vertexIndex) const = 0;
                virtual int GetUsedPointIndexForControlPoint(int controlPointIndex) const = 0;

                virtual unsigned int GetVertexCount() const = 0;
                virtual unsigned int GetFaceCount() const = 0;
                virtual const Face& GetFaceInfo(unsigned int index) const = 0;

                virtual const AZ::Vector3& GetPosition(unsigned int index) const = 0;
                virtual const AZ::Vector3& GetNormal(unsigned int index) const = 0;

                virtual unsigned int GetFaceVertexIndex(unsigned int face, unsigned int vertexIndex) const = 0;
            };
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::SceneAPI::DataTypes::IBlendShapeData::Face>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::SceneAPI::DataTypes::IBlendShapeData::Face& value) const
        {
            result_type hash = 0;
        
            hash_combine(hash, value.vertexIndex[0]);
            hash_combine(hash, value.vertexIndex[1]);
            hash_combine(hash, value.vertexIndex[2]);
            
            return hash;
        }
    };
}
