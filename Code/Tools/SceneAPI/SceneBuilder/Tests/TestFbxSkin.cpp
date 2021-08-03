/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/FbxSceneBuilder/Tests/TestFbxSkin.h>
#include <SceneAPI/FbxSceneBuilder/Tests/TestFbxNode.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        const char* TestFbxSkin::GetName() const
        {
            return m_name.c_str();
        }

        int TestFbxSkin::GetClusterCount() const
        {
            return aznumeric_caster(m_links.size());
        }

        int TestFbxSkin::GetClusterControlPointIndicesCount(int index) const
        {
            return aznumeric_caster(m_controlPointIndices[index].size());
        }

        int TestFbxSkin::GetClusterControlPointIndex(int clusterIndex, int pointIndex) const
        {
            return m_controlPointIndices[clusterIndex][pointIndex];
        }

        double TestFbxSkin::GetClusterControlPointWeight(int clusterIndex, int pointIndex) const
        {
            return m_weights[clusterIndex][pointIndex];
        }

        AZStd::shared_ptr<const FbxNodeWrapper> TestFbxSkin::GetClusterLink(int index) const
        {
            return m_links[index];
        }

        void TestFbxSkin::SetName(const char* name)
        {
            m_name = name;
        }

        void TestFbxSkin::CreateSkinWeightData(AZStd::vector<AZStd::string>& boneNames, AZStd::vector<AZStd::vector<double>>& weights, AZStd::vector<AZStd::vector<int>>& controlPointIndices)
        {
            m_links.resize(boneNames.size());
            for (size_t linkIndex = 0; linkIndex < boneNames.size(); ++linkIndex)
            {
                m_links[linkIndex] = AZStd::make_shared<FbxSDKWrapper::TestFbxNode>();
                m_links[linkIndex]->SetName(boneNames[linkIndex].c_str());
            }
            m_weights = weights;
            m_controlPointIndices = controlPointIndices;
        }

        void TestFbxSkin::CreateExpectSkinWeightData(AZStd::vector<AZStd::vector<int>>& boneIds, AZStd::vector<AZStd::vector<float>>& weights)
        {
            m_expectedBoneIds = boneIds;
            m_expectedWeights = weights;
        }

        size_t TestFbxSkin::GetExpectedVertexCount() const
        {
            return m_expectedBoneIds.size();
        }

        size_t TestFbxSkin::GetExpectedLinkCount(size_t vertexIndex) const
        {
            return m_expectedBoneIds[vertexIndex].size();
        }

        int TestFbxSkin::GetExpectedSkinLinkBoneId(size_t vertexIndex, size_t linkIndex) const
        {
            return m_expectedBoneIds[vertexIndex][linkIndex];
        }

        float TestFbxSkin::GetExpectedSkinLinkWeight(size_t vertextIndex, size_t linkIndex) const
        {
            return m_expectedWeights[vertextIndex][linkIndex];
        }
    }
}
