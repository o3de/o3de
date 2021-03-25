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

#pragma once
#include <SceneAPI/SDKWrapper/NodeWrapper.h>

struct aiScene;

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        class AssImpNodeWrapper : public SDKNode::NodeWrapper
        {
        public:
            AZ_RTTI(AssImpNodeWrapper, "{1043260B-9076-49B7-AD38-EF62E85F7C1D}", SDKNode::NodeWrapper);
            AssImpNodeWrapper(aiNode* fbxNode);
            ~AssImpNodeWrapper() override;
            const char* GetName() const override;
            AZ::u64 GetUniqueId() const override;
            int GetChildCount() const override;
            const std::shared_ptr<NodeWrapper> GetChild(int childIndex) const override;
            const bool ContainsMesh();
            bool ContainsBones(const aiScene& scene) const;
            int GetMaterialCount() const override;
        };
    } // namespace AssImpSDKWrapper
}// namespace AZ
