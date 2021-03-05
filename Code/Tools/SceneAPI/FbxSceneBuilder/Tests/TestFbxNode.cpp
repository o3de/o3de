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
#include <SceneAPI/FbxSceneBuilder/Tests/TestFbxNode.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        const std::shared_ptr<FbxMeshWrapper> TestFbxNode::GetMesh() const
        {
            return m_testFbxMesh;
        }

        const char* TestFbxNode::GetName() const
        {
            return m_name.c_str();
        }

        void TestFbxNode::SetMesh(std::shared_ptr<TestFbxMesh> testFbxMesh)
        {
            m_testFbxMesh = testFbxMesh;
        }

        void TestFbxNode::SetName(const char* name)
        {
            m_name = name;
        }
    }
}