#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMESHDATA_H_
#define AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMESHDATA_H_

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMeshData
                : public IGraphObject
            {
            public:
                AZ_RTTI(IMeshData, "{B94A59C0-F3A5-40A0-B541-7E36B6576C4A}", IGraphObject);

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
                            vertexIndex[2] != rhs.vertexIndex[2]);
                    }
                };

                virtual ~IMeshData() override = default;

                void CloneAttributesFrom(const IGraphObject* sourceObject) override
                {
                    if (const auto* typedSource = azrtti_cast<const IMeshData*>(sourceObject))
                    {
                        SetUnitSizeInMeters(typedSource->GetUnitSizeInMeters());
                        SetOriginalUnitSizeInMeters(typedSource->GetOriginalUnitSizeInMeters());
                    }
                }

                virtual unsigned int GetVertexCount() const = 0;
                virtual bool HasNormalData() const = 0;

                //1 to 1 mapping from position to normal (each corner of triangle represented)
                virtual const AZ::Vector3& GetPosition(unsigned int index) const = 0;
                virtual const AZ::Vector3& GetNormal(unsigned int index) const = 0;

                virtual unsigned int GetFaceCount() const = 0;
                virtual const Face& GetFaceInfo(unsigned int index) const = 0;
                virtual unsigned int GetFaceMaterialId(unsigned int index) const = 0;

                //  0 <= vertexIndex < GetVertexCount().
                virtual int GetControlPointIndex(int vertexIndex) const = 0;

                // Returns number of unique control points used in the mesh.  Here, "used"
                // means it is actually referenced by some polygon in the mesh.
                virtual size_t GetUsedControlPointCount() const = 0;

                // If the control point index specified is indeed used by the mesh, returns a unique value
                // in the range [0,  GetUsedControlPointCount()). Otherwise, returns -1.
                virtual int GetUsedPointIndexForControlPoint(int controlPointIndex) const = 0;

                virtual unsigned int GetVertexIndex(int faceIndex, int vertexIndexInFace) const = 0;
                static const int s_invalidMaterialId = 0;

                // Set the unit size of the mesh, from the point of the source SDK
                void SetUnitSizeInMeters(float size) { m_unitSizeInMeters = size; }
                float GetUnitSizeInMeters() const { return m_unitSizeInMeters; }

                // Set the original unit size of the mesh, from the point of the source SDK
                void SetOriginalUnitSizeInMeters(float size) { m_originalUnitSizeInMeters = size; }
                float GetOriginalUnitSizeInMeters() const { return m_originalUnitSizeInMeters; }

            private:
                float m_unitSizeInMeters = 1.f;
                float m_originalUnitSizeInMeters = 1.f;
            };
        }  //namespace DataTypes
    }  //namespace SceneAPI
}  //namespace AZ

namespace AZStd
{
    template<>
    struct hash<AZ::SceneAPI::DataTypes::IMeshData::Face>
    {
        using result_type = AZStd::size_t;

        result_type operator()(const AZ::SceneAPI::DataTypes::IMeshData::Face& value) const
        {
            result_type hash = 0;

           hash_combine(hash, value.vertexIndex[0]);
           hash_combine(hash, value.vertexIndex[1]);
           hash_combine(hash, value.vertexIndex[2]);
   
           return hash;
        }
    };
}

#endif // AZINCLUDE_TOOLS_SCENECORE_DATATYPES_IMESHDATA_H_
