#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
