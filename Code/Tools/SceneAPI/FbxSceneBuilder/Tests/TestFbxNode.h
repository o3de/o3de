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
#include <SceneAPI/FbxSceneBuilder/Tests/TestFbxMesh.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        // TestFbxNode
        //      FbxNode Test Data creation
        class TestFbxNode
            : public FbxNodeWrapper
        {
        public:
            ~TestFbxNode() override = default;

            const std::shared_ptr<FbxMeshWrapper> GetMesh() const override;
            const char* GetName() const override;

            void SetMesh(std::shared_ptr<TestFbxMesh> testFbxMesh);
            void SetName(const char* name);

        protected:
            std::shared_ptr<TestFbxMesh> m_testFbxMesh;
            AZStd::string m_name;
        };
    }
}