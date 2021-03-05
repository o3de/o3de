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
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class MockFbxMeshWrapper
            : public FbxMeshWrapper
        {
        public:
            MOCK_CONST_METHOD0(GetDeformerCount,
                int());
            MOCK_CONST_METHOD1(GetSkin,
                AZStd::shared_ptr<const FbxSkinWrapper>(int index));
            MOCK_CONST_METHOD1(GetMaterialIndices,
                bool(FbxLayerElementArrayTemplate<int>**));
            MOCK_CONST_METHOD0(GetControlPointsCount,
                int());
            MOCK_CONST_METHOD0(GetControlPoints,
                AZStd::vector<Vector3>());
            MOCK_CONST_METHOD0(GetPolygonCount,
                int());
            MOCK_CONST_METHOD1(GetPolygonSize,
                int(int));
            MOCK_CONST_METHOD1(GetPolygonVertics,
                int(int index));
            MOCK_CONST_METHOD1(GetPolygonVertexIndex,
                int(int));
            MOCK_CONST_METHOD1(GetElementUV,
                FbxUVWrapper(int));
            MOCK_CONST_METHOD0(GetElementUVCount,
                int());
            MOCK_CONST_METHOD1(GetElementVertexColor,
                FbxVertexColorWrapper(int));
            MOCK_CONST_METHOD0(GetElementVertexColorCount,
                int());
            MOCK_CONST_METHOD3(GetPolygonVertexNormal,
                bool(int, int, Vector3&));
        };
    }  // namespace FbxSDKWrapper
}  // namespace AZ
