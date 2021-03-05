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
#include <SceneAPI/FbxSDKWrapper/FbxVertexColorWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class MockFbxVertexColorWrapper 
            : public FbxVertexColorWrapper
        {
        public:
            MOCK_CONST_METHOD3(GetElementAt,
                FbxColorWrapper(int, int, int));
            MOCK_CONST_METHOD0(GetName,
                const char*());
            MOCK_CONST_METHOD0(IsValid,
                bool());
        };

    }  // namespace FbxSDKWrapper
}  // namespace AZ
