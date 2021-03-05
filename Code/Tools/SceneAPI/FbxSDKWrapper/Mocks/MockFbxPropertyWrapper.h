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

#include <SceneAPI/FbxSDKWrapper/FbxPropertyWrapper.h>
#include <AzTest/AzTest.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class MockFbxPropertyWrapper
            : public FbxPropertyWrapper
        {
        public:
            MockFbxPropertyWrapper()
                : FbxPropertyWrapper(new FbxProperty())
            {
            }
            MOCK_CONST_METHOD0(IsValid,
                bool());
            MOCK_CONST_METHOD0(GetFbxVector4,
                Vector4());
            MOCK_CONST_METHOD0(GetFbxInt,
                FbxInt());
            MOCK_CONST_METHOD0(GetFbxString,
                AZStd::string());
            MOCK_CONST_METHOD1(GetEnumValue,
                const char*(int index));
        };
    }  // namespace FbxSDKWrapper
}  // namespace AZ