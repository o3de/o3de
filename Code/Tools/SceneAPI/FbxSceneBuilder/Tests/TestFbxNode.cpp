/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
