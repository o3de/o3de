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

#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>
#include <AzTest/AzTest.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class MockFbxNodeWrapper
            : public FbxNodeWrapper
        {
        public:
            MOCK_CONST_METHOD0(GetMaterialCount,
                int());
            MOCK_CONST_METHOD1(GetMaterialName,
                const char*(int index));
            MOCK_CONST_METHOD1(GetMaterial,
                const std::shared_ptr<FbxMaterialWrapper>(int index));
            MOCK_CONST_METHOD0(GetMesh,
                const std::shared_ptr<FbxMeshWrapper>());
            MOCK_CONST_METHOD1(FindProperty,
                const std::shared_ptr<FbxPropertyWrapper>(const char* name));
            MOCK_CONST_METHOD0(IsBone,
                bool());
            MOCK_CONST_METHOD0(GetName,
                const char*());
            MOCK_METHOD0(EvaluateGlobalTransform,
                Transform());
            MOCK_METHOD0(EvaluateLocalTranslation,
                Vector3());
            MOCK_METHOD1(EvaluateLocalTranslation,
                Vector3(FbxTimeWrapper& time));
            MOCK_METHOD0(EvaluateLocalTransform,
                Transform());
            MOCK_METHOD1(EvaluateLocalTransform,
                Transform(FbxTimeWrapper& time));
            MOCK_CONST_METHOD0(GetGeometricTranslation,
                Vector3());
            MOCK_CONST_METHOD0(GetGeometricScaling,
                Vector3());
            MOCK_CONST_METHOD0(GetGeometricRotation,
                Vector3());
            MOCK_CONST_METHOD0(GetGeometricTransform,
                Transform());
            MOCK_CONST_METHOD0(GetChildCount,
                int());
            MOCK_CONST_METHOD1(GetChild,
                const std::shared_ptr<FbxNodeWrapper>(int childIndex));
            MOCK_CONST_METHOD0(IsAnimated,
                bool());
        };
    }  // namespace FbxSDKWrapper
}  // namespace AZ