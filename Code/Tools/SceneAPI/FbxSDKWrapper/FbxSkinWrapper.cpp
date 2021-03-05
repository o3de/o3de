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

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Debug/Trace.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/FbxSDKWrapper/FbxSkinWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        FbxSkinWrapper::FbxSkinWrapper(FbxSkin* fbxSkin)
            : m_fbxSkin(fbxSkin)
        {
            AZ_Assert(fbxSkin, "Invalid FbxSkin input to initialize FbxSkinWrapper");
        }

        FbxSkinWrapper::~FbxSkinWrapper()
        {
            m_fbxSkin = nullptr;
        }

        const char* FbxSkinWrapper::GetName() const
        {
            return m_fbxSkin->GetName();
        }

        int FbxSkinWrapper::GetClusterCount() const
        {
            return m_fbxSkin->GetClusterCount();
        }

        int FbxSkinWrapper::GetClusterControlPointIndicesCount(int index) const
        {
            if (index < m_fbxSkin->GetClusterCount())
            {
                return m_fbxSkin->GetCluster(index)->GetControlPointIndicesCount();
            }
            AZ_Assert(index < m_fbxSkin->GetClusterCount(), "Invalid skin cluster index %d", index);
            return 0;
        }

        int FbxSkinWrapper::GetClusterControlPointIndex(int clusterIndex, int pointIndex) const
        {
            bool validIndex = clusterIndex < m_fbxSkin->GetClusterCount() && pointIndex < m_fbxSkin->GetCluster(clusterIndex)->GetControlPointIndicesCount();
            if (validIndex)
            {
                return m_fbxSkin->GetCluster(clusterIndex)->GetControlPointIndices()[pointIndex];
            }
            AZ_Assert(validIndex, "Invalid skin cluster control point index at cluster index %d control point index %d", clusterIndex, pointIndex);
            return -1;
        }

        double FbxSkinWrapper::GetClusterControlPointWeight(int clusterIndex, int pointIndex) const
        {
            bool validIndex = clusterIndex < m_fbxSkin->GetClusterCount() && pointIndex < m_fbxSkin->GetCluster(clusterIndex)->GetControlPointIndicesCount();      
            if (validIndex)
            {
                return m_fbxSkin->GetCluster(clusterIndex)->GetControlPointWeights()[pointIndex];
            }
            AZ_Assert(validIndex, "Invalid skin cluster control point weight at cluster index %d control point index %d", clusterIndex, pointIndex);
            return 0.0f;
        }

        AZStd::shared_ptr<const FbxNodeWrapper> FbxSkinWrapper::GetClusterLink(int index) const
        {       
            if (index < m_fbxSkin->GetClusterCount())
            {
                return AZStd::make_shared<const FbxNodeWrapper>(m_fbxSkin->GetCluster(index)->GetLink());
            }
            AZ_Assert(index < m_fbxSkin->GetClusterCount(), "Invalid skin cluster index %d", index);
            return nullptr;
        }
    } // namespace FbxSDKWrapper
} // namespace AZ
