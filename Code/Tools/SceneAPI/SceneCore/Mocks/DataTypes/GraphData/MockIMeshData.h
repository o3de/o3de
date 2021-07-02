#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class MockIMeshData
                : public IMeshData
            {
            public:
                AZ_RTTI(MockIMeshData, "{83BB2FDC-5B57-4334-B8D1-3D328F31AE7F}", IMeshData)

                virtual ~MockIMeshData() = default;

                MOCK_CONST_METHOD0(GetVertexCount,
                    unsigned int());
                MOCK_CONST_METHOD0(HasNormalData,
                    bool());
                MOCK_CONST_METHOD0(HasTextureCoordinates,
                    bool());
                MOCK_CONST_METHOD1(GetPosition,
                    const AZ::Vector3 & (unsigned int index));
                MOCK_CONST_METHOD1(GetNormal,
                    const AZ::Vector3 & (unsigned int index));
                MOCK_CONST_METHOD1(GetTextureCoordinates,
                    const AZ::Vector2 & (unsigned int index));
                MOCK_CONST_METHOD0(GetFaceCount,
                    unsigned int());
                MOCK_CONST_METHOD1(GetFaceInfo,
                    const Face&(unsigned int index));
                MOCK_CONST_METHOD1(GetFaceMaterialId,
                    unsigned int(unsigned int index));
                MOCK_CONST_METHOD1(GetControlPointIndex,
                     int(int index));
                MOCK_CONST_METHOD0(GetUsedControlPointCount,
                    size_t());
                MOCK_CONST_METHOD2(GetVertexIndex,
                    unsigned int(int faceIndex, int vertexIndexInFace));
                MOCK_CONST_METHOD1(GetUsedPointIndexForControlPoint,
                    int(int controlPointIndex));
            };
        }  // namespace DataTypes
    }  //namespace SceneAPI
}  // namespace AZ
