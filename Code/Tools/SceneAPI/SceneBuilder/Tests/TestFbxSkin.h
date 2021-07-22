#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/FbxSDKWrapper/FbxSkinWrapper.h>
#include <SceneAPI/FbxSceneBuilder/Tests/TestFbxNode.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class TestFbxSkin
            : public FbxSkinWrapper
        {
        public:
            ~TestFbxSkin() override = default;

            const char* GetName() const override;
            int GetClusterCount() const override;
            int GetClusterControlPointIndicesCount(int index) const override;
            int GetClusterControlPointIndex(int clusterIndex, int pointIndex) const override;
            double GetClusterControlPointWeight(int clusterIndex, int pointIndex) const override;
            AZStd::shared_ptr<const FbxNodeWrapper> GetClusterLink(int index) const override;

            void SetName(const char* name);
            void CreateSkinWeightData(AZStd::vector<AZStd::string>& boneNames, AZStd::vector<AZStd::vector<double>>& weights, AZStd::vector<AZStd::vector<int>>& controlPointIndices);
            void CreateExpectSkinWeightData(AZStd::vector<AZStd::vector<int>>& boneIds, AZStd::vector<AZStd::vector<float>>& weights);

            size_t GetExpectedVertexCount() const;
            size_t GetExpectedLinkCount(size_t vertexIndex) const;
            int GetExpectedSkinLinkBoneId(size_t vertexIndex, size_t linkIndex) const;
            float GetExpectedSkinLinkWeight(size_t vertexIndex, size_t linkIndex) const;

        protected:
            AZStd::string m_name;
            AZStd::vector<AZStd::shared_ptr<FbxSDKWrapper::TestFbxNode>> m_links;
            AZStd::vector<AZStd::vector<double>> m_weights;
            AZStd::vector<AZStd::vector<int>> m_controlPointIndices;

            AZStd::vector<AZStd::vector<int>> m_expectedBoneIds;
            AZStd::vector<AZStd::vector<float>> m_expectedWeights;
        };
    }
}
