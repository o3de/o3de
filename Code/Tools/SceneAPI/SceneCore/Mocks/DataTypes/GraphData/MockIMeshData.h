#pragma once

/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
