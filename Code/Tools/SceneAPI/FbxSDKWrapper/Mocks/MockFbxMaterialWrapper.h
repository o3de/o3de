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
#include <SceneAPI/FbxSDKWrapper/FbxMaterialWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class MockFbxMaterialWrapper
            : public FbxMaterialWrapper
        {
        public:
            MOCK_CONST_METHOD0(GetName,
                std::string());
            MOCK_CONST_METHOD1(GetTextureFileName,
                std::string(const char* textureType));
            MOCK_CONST_METHOD1(GetTextureFileName,
                std::string(const std::string& textureType));
            MOCK_CONST_METHOD1(GetTextureFileName,
                std::string(MaterialMapType textureType));
            MOCK_CONST_METHOD0(IsNoDraw,
                bool());
        };
    }  // namespace FbxSDKWrapper
}  // namespace AZ