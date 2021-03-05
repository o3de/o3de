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

#include <SceneAPI/FbxSDKWrapper/FbxSceneWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxTimeWrapper.h>
#include <AzTest/AzTest.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class MockFbxSceneWrapper
            : public FbxSceneWrapper
        {
        public:
            virtual ~MockFbxSceneWrapper() = default;

            MOCK_METHOD1(LoadSceneFromFile,
                bool(const char* fileName));
            MOCK_METHOD1(LoadSceneFromFile,
                bool(const std::string& fileName));
            MOCK_CONST_METHOD0(GetSystemUnit,
                AZStd::shared_ptr<FbxSystemUnitWrapper>());
            MOCK_CONST_METHOD0(GetAxisSystem,
                AZStd::shared_ptr<FbxAxisSystemWrapper>());
            MOCK_CONST_METHOD0(GetTimeLineDefaultDuration,
                FbxTimeWrapper());
            MOCK_CONST_METHOD0(GetLastSavedApplicationName,
                const char*());
            MOCK_CONST_METHOD0(GetLastSavedApplicationVersion,
                const char*());
            MOCK_CONST_METHOD0(GetRootNode,
                const std::shared_ptr<FbxNodeWrapper>());
            MOCK_METHOD0(GetRootNode,
                std::shared_ptr<FbxNodeWrapper>());
            MOCK_CONST_METHOD0(GetAnimationStackCount,
                int());
            MOCK_CONST_METHOD1(GetAnimationStackAt,
                const std::shared_ptr<FbxAnimStackWrapper>(int index));
            MOCK_METHOD0(Clear,
                void());
        };
    }  // namespace FbxSDKWrapper
}  // namespace AZ