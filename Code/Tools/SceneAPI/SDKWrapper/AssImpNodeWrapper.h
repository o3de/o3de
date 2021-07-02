/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AssImpNodeWrapper(aiNode* sourceNode);
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
